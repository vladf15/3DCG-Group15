#include "mesh.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <iostream>
#include <vector>

GPUMaterial::GPUMaterial(const Material& material) :
    kd(material.kd),
    ks(material.ks),
    shininess(material.shininess),
    opacity(material.transparency)
{}

GPUMesh::GPUMesh(const Mesh& cpuMesh)
{
    // Create uniform buffer to store mesh material (https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL)
    GPUMaterial gpuMaterial(cpuMesh.material);
    glGenBuffers(1, &m_uboMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, m_uboMaterial);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GPUMaterial), &gpuMaterial, GL_STATIC_READ);

    // Figure out if this mesh has texture coordinates
    m_hasTextureCoords = static_cast<bool>(cpuMesh.material.kdTexture);

    // Make tangents and bitangents for each vertex https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    std::vector<GPUVertex> gpuVertices;
    for (int i = 0; i < cpuMesh.vertices.size(); i++) {
        glm::vec3 e1{ 0.0f };
        glm::vec3 e2{ 0.0f };
        glm::vec2 duv1{ 0.0f };
        glm::vec2 duv2{ 0.0f };
        Vertex p = cpuMesh.vertices[i];
        if (m_hasTextureCoords) {
            int i1, i2;
            for (int j = 0; j < cpuMesh.triangles.size(); j++) {
                if (cpuMesh.triangles[j].x == i) {
                    i1 = cpuMesh.triangles[j].y;
                    i2 = cpuMesh.triangles[j].z;
                }
                else if (cpuMesh.triangles[j].y == i) {
                    i1 = cpuMesh.triangles[j].x;
                    i2 = cpuMesh.triangles[j].z;
                }
                else if (cpuMesh.triangles[j].z == i) {
                    i1 = cpuMesh.triangles[j].x;
                    i2 = cpuMesh.triangles[j].y;
                }
                else continue;
                break;
            }
            e1 = cpuMesh.vertices[i1].position - p.position;
            e2 = cpuMesh.vertices[i2].position - p.position;

            duv1 = cpuMesh.vertices[i1].texCoord - p.texCoord;
            duv2 = cpuMesh.vertices[i2].texCoord - p.texCoord;

        }
        
        gpuVertices.emplace_back(p.position, p.normal, p.texCoord, e1, e2, duv1, duv2);
    }

    // Create VAO and bind it so subsequent creations of VBO and IBO are bound to this VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // Create vertex buffer object (VBO)
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(gpuVertices.size() * sizeof(decltype(gpuVertices)::value_type)), gpuVertices.data(), GL_STATIC_DRAW);

    // Create index buffer object (IBO)
    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(cpuMesh.triangles.size() * sizeof(decltype(cpuMesh.triangles)::value_type)), cpuMesh.triangles.data(), GL_STATIC_DRAW);

    // Tell OpenGL that we will be using vertex attributes 0, 1 and 2.
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);
    // We tell OpenGL what each vertex looks like and how they are mapped to the shader (location = ...).
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, normal));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, texCoord));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, e1));
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, e2));
    glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, duv1));
    glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, duv2));
    // Reuse all attributes for each instance
    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 0);
    glVertexAttribDivisor(2, 0);
    glVertexAttribDivisor(3, 0);
    glVertexAttribDivisor(4, 0);
    glVertexAttribDivisor(5, 0);
    glVertexAttribDivisor(6, 0);

    // Each triangle has 3 vertices.
    m_numIndices = static_cast<GLsizei>(3 * cpuMesh.triangles.size());
}

GPUMesh::GPUMesh(GPUMesh&& other)
{
    moveInto(std::move(other));
}

GPUMesh::~GPUMesh()
{
    freeGpuMemory();
}

GPUMesh& GPUMesh::operator=(GPUMesh&& other)
{
    moveInto(std::move(other));
    return *this;
}

std::vector<GPUMesh> GPUMesh::loadMeshGPU(std::filesystem::path filePath, bool normalize) {
    if (!std::filesystem::exists(filePath))
        throw MeshLoadingException(fmt::format("File {} does not exist", filePath.string().c_str()));

    // Generate GPU-side meshes for all sub-meshes
    std::vector<Mesh> subMeshes = loadMesh(filePath, normalize);
    std::vector<GPUMesh> gpuMeshes;
    for (const Mesh& mesh : subMeshes) { gpuMeshes.emplace_back(mesh); }
    
    return gpuMeshes;
}

bool GPUMesh::hasTextureCoords() const
{
    return m_hasTextureCoords;
}

void GPUMesh::draw(const Shader& drawingShader)
{
    // Bind material data uniform (we assume that the uniform buffer objects is always called 'Material')
    // Yes, we could define the binding inside the shader itself, but that would break on OpenGL versions below 4.2
    drawingShader.bindUniformBlock("Material", 0, m_uboMaterial);
    
    // Draw the mesh's triangles
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);
}
void GPUMesh::draw_no_mat(const Shader& drawingShader) {
    // Draw the mesh's triangles
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);
}

void GPUMesh::moveInto(GPUMesh&& other)
{
    freeGpuMemory();
    m_numIndices = other.m_numIndices;
    m_hasTextureCoords = other.m_hasTextureCoords;
    m_ibo = other.m_ibo;
    m_vbo = other.m_vbo;
    m_vao = other.m_vao;
    m_uboMaterial = other.m_uboMaterial;

    other.m_numIndices = 0;
    other.m_hasTextureCoords = other.m_hasTextureCoords;
    other.m_ibo = INVALID;
    other.m_vbo = INVALID;
    other.m_vao = INVALID;
    other.m_uboMaterial = INVALID;
}

void GPUMesh::freeGpuMemory()
{
    if (m_vao != INVALID)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != INVALID)
        glDeleteBuffers(1, &m_vbo);
    if (m_ibo != INVALID)
        glDeleteBuffers(1, &m_ibo);
    if (m_uboMaterial != INVALID)
        glDeleteBuffers(1, &m_uboMaterial);
}
