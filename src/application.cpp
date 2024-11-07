//#include "Image.h"
#include "mesh.h"
#include "texture.h"
// Always include window first (because it includes glfw, which includes GL which needs to be included AFTER glew).
// Can't wait for modules to fix this stuff...
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>
#include <framework/window.h>
#include <functional>
#include <iostream>
#include <vector>
#include "bezieranims.h"
#include"Planet.h"
#include"Planet.cpp"
#include<random>


class Application {
public:

    Application()
        : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL41)
        , m_texture(RESOURCE_ROOT "resources/checkerboard.png")
        // Load textures for PBR shading 
        , kaTexture(RESOURCE_ROOT "resources/Stone/PavingStones142_2K-PNG_AmbientOcclusion.png")
        , kdTexture(RESOURCE_ROOT "resources/Stone/PavingStones142_2K-PNG_Color.png")
        , displacementTexture(RESOURCE_ROOT "resources/Stone/PavingStones142_2K-PNG_Displacement.png")
        , normalTexture(RESOURCE_ROOT "resources/Stone/PavingStones142_2K-PNG_NormalGL.png")
        , roughnessTexture(RESOURCE_ROOT "resources/Stone/PavingStones142_2K-PNG_Roughness.png")
    {
        m_window.registerKeyCallback([this](int key, int scancode, int action, int mods) {
            if (action == GLFW_PRESS)
                onKeyPressed(key, mods);
            else if (action == GLFW_RELEASE)
                onKeyReleased(key, mods);
        });
        m_window.registerMouseMoveCallback(std::bind(&Application::onMouseMove, this, std::placeholders::_1));
        m_window.registerMouseButtonCallback([this](int button, int action, int mods) {
            if (action == GLFW_PRESS)
                onMouseClicked(button, mods);
            else if (action == GLFW_RELEASE)
                onMouseReleased(button, mods);
        });

        m_meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/dragon.obj");
        ss_mesh = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/sphere.obj");
        solar_system_ts = 0.0f;
        pbrMeshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/Stone/surface.obj");
        //pbrMeshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/bread/3DBread012_HQ-2K-PNG.obj");
        
        try {
            ShaderBuilder defaultBuilder;
            defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
            m_defaultShader = defaultBuilder.build();

            ShaderBuilder shadowBuilder;
            shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
            shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "Shaders/shadow_frag.glsl");
            m_shadowShader = shadowBuilder.build();

            ShaderBuilder ssBuilder;
            ssBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/ss_vert.glsl");
            ssBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/ss_frag.glsl");
            m_solarShader = ssBuilder.build();

            ShaderBuilder pbrBuilder; 
            pbrBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/pbr_vert.glsl");
            pbrBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/pbr_frag.glsl");
            m_pbrShader = pbrBuilder.build();


            // Any new shaders can be added below in similar fashion.
            // ==> Don't forget to reconfigure CMake when you do!
            //     Visual Studio: PROJECT => Generate Cache for ComputerGraphics
            //     VS Code: ctrl + shift + p => CMake: Configure => enter
            // ....
        } catch (ShaderLoadingException e) {
            std::cerr << e.what() << std::endl;
        }
    }
    glm::vec3 gen_rand_clr() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution dis(0.0f, 1.0f);
        return glm::vec3(dis(gen), dis(gen), dis(gen));
    }
    void create_planet() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> pRadiusDistr(0.4f, 0.75f);
        std::uniform_real_distribution<> thetaInitDistr(0.0f, 2 * acos(-1.0f));
        std::uniform_real_distribution<> phiInitDistr(0.0f, 180);
        std::uniform_real_distribution<> spdDistr(0.5f, 2.0f);
        float pRadius = pRadiusDistr(gen);
        float tRadius = planets.size() + sun_size * 1.5f;
        float t_init = thetaInitDistr(gen);
        float p_init = phiInitDistr(gen);
        float spd = spdDistr(gen);
        planets.push_back(Planet(pRadius, sun_size * 1.5f, t_init, p_init, spd, gen_rand_clr()));
    }

    void userInterface()
    {
        // Use ImGui for easy input/output of ints, floats, strings, etc...
        ImGui::Begin("Window");
//        ImGui::InputInt("This is an integer input", &dummyInteger); // Use ImGui::DragInt or ImGui::DragFloat for larger range of numbers.
//        ImGui::Text("Value is: %i", dummyInteger); // Use C printf formatting rules (%i is a signed integer)
        ImGui::TextWrapped("Welcome, use WASD, Space and Lcontrol to move the camera.\n Left click + drag to change view direction.\n Use '1', '2' and '3' to switch between camera modes.\n Use the buttons to move through the scenes");
        if(ImGui::Button("Previous Scene")) {
            if (currentScene > 0) {
                currentScene--;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Next Scene")) {
           
            currentScene++;
    
        }
        ImGui::Checkbox("Use material if no texture", &m_useMaterial);
        ImGui::Separator();
        switch(currentScene) {
            case 0:
                ImGui::TextWrapped("This is the first scene. You can use the '1', '2' and '3' keys to switch between camera modes.");
                break;
            case 1:
                ImGui::TextWrapped("Displaying Solar System. Use the options to change the parameters");
                ImGui::SliderFloat3("Sun Position", &sun_pos.x, -10.0f, 10.0f);
                ImGui::SliderFloat("Sun Radius", &sun_size, 0.2f, 3.0f);
                ImGui::ColorPicker3("Sun Color", &sun_color.x);
                ImGui::Separator();
                if (ImGui::Button("Add Planet")) {
                    create_planet();
                }
                //Code to select one planet in the list, taken from implementation in assignment 1.1 (Nicolas)
                if (planets.size() > 0) {
                    ImGui::Separator();
                    std::vector<std::string> planet_names = {};
                    std::vector<const char*> planet_list = {};
                    int count = 0;
                    for (const auto& plan : planets) {
                        auto p_name = "Planet " + std::to_string(count);
                        count++;
                        planet_names.push_back(p_name);
                    }
                    for (const auto& p : planet_names) {
                        planet_list.push_back(p.c_str());
                    }
                    if (selected_planet >= planet_list.size()) {
                        selected_planet = planet_list.size() - 1;
                    }
                    int temp_planet_index = selected_planet;
                    if (ImGui::ListBox("Select Planet", &temp_planet_index, planet_list.data(), planet_list.size())) {
                        selected_planet = temp_planet_index;
                    }
                    ImGui::TextWrapped("Planet %i", selected_planet);
                    ImGui::SliderFloat("Speed", &planets[selected_planet].speed, 0.0f, 3.0f);
                    ImGui::SliderFloat("Radius", &planets[selected_planet].radius, 0.5f, 3.0f);
                    ImGui::ColorPicker3("Planet Color", &planets[selected_planet].color.x);

                    if (ImGui::Button("Remove Moon")) {
                        planets[selected_planet].RemoveMoon();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Create Moon")) {
                        planets[selected_planet].CreateMoon();
                    }
                    if (ImGui::Button("Delete Planet")) {
                        planets.erase(planets.begin() + selected_planet);
                    }
                }
                
                break;
            case 2:
                ImGui::TextWrapped("Displaying Infinite maze");
                break;
            case 3:
                ImGui::TextWrapped("Displaying Procedural Trees");
                break;
            case 4:
                ImGui::TextWrapped("Displaying Procedural water surface");
                break;
            case 5:
                ImGui::TextWrapped("Displaying Inverse Kinematics Animation");
                break;
            case 6: 
                ImGui::TextWrapped("Displaying PBR shading");
                ImGui::Checkbox("Enable PBR", &usePbr);
                break;
            default:
                ImGui::TextWrapped("Default Scene");
                break;
        }
        ImGui::End();
    }

    

    void update()
    {
        while (!m_window.shouldClose()) {
            // This is your game loop
            // Put your real-time logic and rendering in here
            m_window.updateInput();

          
            //animation code
            if (inAnimation) {
				//switch to next animation if current animation is done
				if (animationTimer > animationLength && currentAnim < animPath.size() - 1) {
					currentAnim++;
					animationTimer = 0.0f;
				}
				//if all animations are done, stop animation
				else if (animationTimer > animationLength) {
                    inAnimation = false;
					animationTimer = 0.0f;
                    currentAnim = 0.0f;
                }
				//if animation is still going, update position and direction
                else {
                    std::pair<glm::vec3, glm::vec3> oldCoords = getPointOnCurve(animPath[currentAnim], animationTimer / animationLength, 100);
                    animationTimer += 0.1f;
                    std::pair<glm::vec3, glm::vec3> newCoords = getPointOnCurve(animPath[currentAnim], animationTimer / animationLength, 100);
                    currentPos += newCoords.first - oldCoords.first;
                    currentDir = glm::normalize(currentDir + newCoords.second - oldCoords.second);
                }
            }

            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), currentPos);
            glm::mat4 rotationMatrix = getRotationMatrix(currentDir);
            m_modelMatrix = translationMatrix * rotationMatrix;


            //camera movement
            if (viewMode == 1) {
                glm::vec3 rightDir = glm::normalize(glm::cross(viewDir, glm::vec3(0.0f, 1.0f, 0.0f)));
                if (moveForward) {
                    cameraPos += cameraSpeed * glm::normalize(viewDir * glm::vec3(1.0f, 0.0f, 1.0f));
                }
                if (moveBackward) {
                    cameraPos += -cameraSpeed * glm::normalize(viewDir * glm::vec3(1.0f, 0.0f, 1.0f));
                }
                if (moveLeft) {
                    cameraPos += -cameraSpeed * glm::normalize(rightDir * glm::vec3(1.0f, 0.0f, 1.0f));
                }
                if (moveRight) {
                    cameraPos += cameraSpeed * glm::normalize(rightDir * glm::vec3(1.0f, 0.0f, 1.0f));
                }
                if (moveUp) {
                    cameraPos += cameraSpeed * glm::vec3(0.0f, 1.0f, 0.0f);
                }
                if (moveDown) {
                    cameraPos += -cameraSpeed * glm::vec3(0.0f, 1.0f, 0.0f);
                }
                m_viewMatrix = glm::lookAt(cameraPos, cameraPos + viewDir, glm::vec3(0.0f, 1.0f, 0.0f));
            }
            else if (viewMode == 2) {
                cameraPos = currentPos + glm::vec3(0.0f, 1.0f, 0.0f);
                m_viewMatrix = glm::lookAt(cameraPos, currentPos, glm::vec3(0.0f, 0.0f, 1.0f));
            }
            else if (viewMode == 3) {
                glm::vec3 rightDir = glm::normalize(glm::cross(currentDir, glm::vec3(0.0f, 1.0f, 0.0f)));
				glm::vec3 upDir = glm::normalize(glm::cross(rightDir, currentDir));

				cameraPos = currentPos - 2.0f * currentDir + 0.5f * upDir;
				m_viewMatrix = glm::lookAt(cameraPos, currentPos, upDir);
            }


            

            userInterface();

            // Clear the screen
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // ...
            glEnable(GL_DEPTH_TEST);
            const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
            // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
            // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
            const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));
            switch (currentScene) {
                case 0:
                    for (GPUMesh& mesh : m_meshes) {
                        m_defaultShader.bind();
                        glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                        //Uncomment this line when you use the modelMatrix (or fragmentPosition)
                        //glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
                        glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
                        if (mesh.hasTextureCoords()) {
                            m_texture.bind(GL_TEXTURE0);
                            glUniform1i(m_defaultShader.getUniformLocation("colorMap"), 0);
                            glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_TRUE);
                            glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);
                        } else {
                            glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                            glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
                        }
                        mesh.draw(m_defaultShader);
                    }
                   break;
                case 1:
                    glm::mat4 sun_model = glm::mat4(1.0f);
                    sun_model = glm::translate(sun_model, sun_pos);
                    sun_model = glm::scale(sun_model, glm::vec3(sun_size));
                    glm::mat4 mvp = m_projectionMatrix * m_viewMatrix * sun_model;
                    glm::mat3 nModel = glm::inverseTranspose(glm::mat3(sun_model));
                    for(GPUMesh& mesh : ss_mesh){
                        m_solarShader.bind();
                        glUniformMatrix4fv(m_solarShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvp));
                        glUniformMatrix3fv(m_solarShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(nModel));
                        glUniform4f(m_solarShader.getUniformLocation("theColor"), 1.0f, 1.0f, 0.0f, 1.0f);
                        glUniform3f(m_solarShader.getUniformLocation("camPos"), cameraPos.x, cameraPos.y, cameraPos.z);
                        mesh.draw_no_mat(m_solarShader);
                        for (int i = 0; i < planets.size(); i++) {
                            Planet p = planets[i];
                            glm::mat4 p_model = p.GetModel(sun_model, solar_system_ts, i);
                            mvp = m_projectionMatrix * m_viewMatrix * p_model;
                            nModel = glm::inverseTranspose(glm::mat3(p_model));
                            glUniformMatrix4fv(m_solarShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvp));
                            glUniformMatrix3fv(m_solarShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(nModel));
                            glUniform4f(m_solarShader.getUniformLocation("theColor"), p.color.x, p.color.y, p.color.z, 1.0f);
                            mesh.draw_no_mat(m_solarShader);
                            for (int j = 0; j < p.moons.size(); j++) {
                                Moon m = p.moons[j];
                                glm::mat4 m_model = p.GetMoonModel(p_model, solar_system_ts, j);
                                glm::vec3 c = m.color;
                                mvp = m_projectionMatrix * m_viewMatrix * m_model;
                                nModel = glm::inverseTranspose(glm::mat3(m_model));
                                glUniformMatrix4fv(m_solarShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvp));
                                glUniformMatrix3fv(m_solarShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(nModel));
                                glUniform4f(m_solarShader.getUniformLocation("theColor"), c.x, c.y, c.z, 1.0f);
                                mesh.draw_no_mat(m_solarShader);
                            }
                        }
                    }
                    solar_system_ts += 1.0f;
                    break;
                case 2:
                    // Implementation based on wang tiles https://www.boristhebrave.com/permanent/24/06/cr31/stagecast/wang/intro.html
                    break;
                case 3:
                    break;
                case 4:
                    break;
                case 5:
                    break;
                case 6: 
                    for (GPUMesh& mesh : pbrMeshes) {
                        m_pbrShader.bind();
                        glUniformMatrix4fv(m_pbrShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                        glUniformMatrix4fv(m_pbrShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
                        glUniformMatrix3fv(m_pbrShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
                        glUniform1i(m_pbrShader.getUniformLocation("usePbr"), usePbr);
                        if (mesh.hasTextureCoords()) {
                            glUniform1i(m_pbrShader.getUniformLocation("hasTexCoords"), GL_TRUE);
                            kdTexture.bind(GL_TEXTURE0);
                            glUniform1i(m_pbrShader.getUniformLocation("colorMap"), 0);
                            m_texture.bind(GL_TEXTURE1);
                            glUniform1i(m_pbrShader.getUniformLocation("displacementMap"), 1);
                            normalTexture.bind(GL_TEXTURE2);
                            glUniform1i(m_pbrShader.getUniformLocation("normalMap"), 2);
                            kaTexture.bind(GL_TEXTURE3);
                            glUniform1i(m_pbrShader.getUniformLocation("kaMap"), 3);
                            roughnessTexture.bind(GL_TEXTURE4);
                            glUniform1i(m_pbrShader.getUniformLocation("roughnessMap"), 4);
                        }
                        else {
                            glUniform1i(m_pbrShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                        }
                        mesh.draw(m_pbrShader);
                    }

                    break;
                default:
                    break;
            }

            

            // Processes input and swaps the window buffer
            m_window.swapBuffers();
        }
    }

    // In here you can handle key presses
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyPressed(int key, int mods)
    {
        std::cout << "Key pressed: " << key << std::endl;
        switch (key) {
        case GLFW_KEY_Q:
            inAnimation = true;
            break;
        case GLFW_KEY_W:
            moveForward = true;
            break;
        case GLFW_KEY_S:
            moveBackward = true;
            break;
        case GLFW_KEY_A:
            moveLeft = true;
            break;
        case GLFW_KEY_D:
            moveRight = true;
            break;
        case GLFW_KEY_SPACE:
            moveUp = true;
            break;
        case GLFW_KEY_LEFT_CONTROL:
            moveDown = true;
            break;
        case GLFW_KEY_1:
            viewMode = 1;
            //set camera to look at object
            cameraPos = glm::vec3(-1.0f, 1.0f, -1.0f) + currentPos;
			viewDir = glm::normalize(currentPos - cameraPos);
            yaw = glm::degrees(atan2(viewDir.z, viewDir.x));
            pitch = glm::degrees(asin(viewDir.y));
            pitch = glm::clamp(pitch, -85.0f, 85.0f);
            viewDir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            viewDir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            viewDir.y = sin(glm::radians(pitch));
            viewDir = glm::normalize(viewDir);
            break;
        case GLFW_KEY_2:
            viewMode = 2;
            break;
        case GLFW_KEY_3:
            viewMode = 3;
            break;
        }
    }

    // In here you can handle key releases
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyReleased(int key, int mods)
    {
        switch (key) {
        case GLFW_KEY_W:
            moveForward = false;
            break;
        case GLFW_KEY_S:
            moveBackward = false;
            break;
        case GLFW_KEY_A:
            moveLeft = false;
            break;
        case GLFW_KEY_D:
            moveRight = false;
            break;
        case GLFW_KEY_SPACE:
            moveUp = false;
            break;
        case GLFW_KEY_LEFT_CONTROL:
            moveDown = false;
            break;
        }
    }

    // If the mouse is moved this function will be called with the x, y screen-coordinates of the mouse
    void onMouseMove(const glm::dvec2& cursorPos)
    {
		if (mouseDrag) {
            yaw += sensitivity * (cursorPos.x - mousePos.x);
            pitch += sensitivity * (cursorPos.y - mousePos.y);
            pitch = glm::clamp(pitch, -85.0f, 85.0f);
            viewDir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            viewDir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            viewDir.y = sin(glm::radians(pitch));
			viewDir = glm::normalize(viewDir);
        }
        mousePos = cursorPos;
       // std::cout << "Mouse at position: " << cursorPos.x << " " << cursorPos.y << std::endl;
    }

    // If one of the mouse buttons is pressed this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseClicked(int button, int mods)
    {
        if(button == GLFW_MOUSE_BUTTON_1 && viewMode == 1)
            mouseDrag = true;
       // std::cout << "Pressed mouse button: " << button << std::endl;
    }

    // If one of the mouse buttons is released this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseReleased(int button, int mods)
    {
        if (button == GLFW_MOUSE_BUTTON_1)
            mouseDrag = false;
        //std::cout << "Released mouse button: " << button << std::endl;
    }

private:
    Window m_window;

    // Shader for default rendering and for depth rendering
    Shader m_defaultShader;
    Shader m_shadowShader;
    
    bool usePbr;
    Shader m_pbrShader;
    Texture kaTexture;
    Texture kdTexture;
    Texture normalTexture;
    Texture roughnessTexture;
    Texture displacementTexture;
    std::vector<GPUMesh> pbrMeshes;

    std::vector<GPUMesh> m_meshes;
    Texture m_texture;
    bool m_useMaterial { true };

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_projectionMatrix = glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 30.0f);
    glm::mat4 m_viewMatrix = glm::lookAt(glm::vec3(-1, 1, -1), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 m_modelMatrix { 1.0f };

    //Animation variables
    bool inAnimation{ false };
    int currentAnim = 0;
	float animationTimer{ 0.0f };
	float animationLength{ 100.0f };
	std::vector<BezierSpline> animPath = loadSplines(RESOURCE_ROOT "resources/bezier_splines.json", false);
	glm::vec3 currentPos{ 0.0f };
    glm::vec3 currentDir{ 0.0f, 0.0f, 1.0f };


    //Camera variables
	glm::vec3 cameraPos{ -1.0f, 1.0f, -1.0f };
	glm::vec3 viewDir{ glm::normalize(-cameraPos)};
	int viewMode{ 1 };
    float cameraSpeed = 0.02f;
	bool moveForward{ false };
	bool moveBackward{ false };
	bool moveLeft{ false };
	bool moveRight{ false };
	bool moveUp{ false };
	bool moveDown{ false };
	bool mouseDrag{ false };
	glm::dvec2 mousePos{ 0.0, 0.0 };
	float sensitivity = 0.75f;
    float yaw = glm::degrees(atan2(viewDir.z, viewDir.x));
    float pitch = glm::degrees(asin(viewDir.y));

    //ImGui Stuff
    bool triggered{ false };
    int currentScene = 0;


  

    //Solar system stuff
    Shader m_solarShader;
    std::vector<GPUMesh> ss_mesh;
    glm::vec3 sun_pos = glm::vec3(0.0f, 0.0f, 0.0f);
    float sun_size = 1.0f;
    glm::vec3 sun_color = glm::vec3(1.0f, 1.0f, 0.0f);
    float solar_system_ts = 0.0f;
    std::vector<Planet> planets;
    size_t selected_planet;

};

int main()
{
    Application app;
    app.update();

    return 0;
}
