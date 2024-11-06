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



class Application {
public:
    Application()
        : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL41)
        , m_texture(RESOURCE_ROOT "resources/checkerboard.png")
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


            // Any new shaders can be added below in similar fashion.
            // ==> Don't forget to reconfigure CMake when you do!
            //     Visual Studio: PROJECT => Generate Cache for ComputerGraphics
            //     VS Code: ctrl + shift + p => CMake: Configure => enter
            // ....
        } catch (ShaderLoadingException e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void userInterface()
    {
        // Use ImGui for easy input/output of ints, floats, strings, etc...
        ImGui::Begin("Window");
//        ImGui::InputInt("This is an integer input", &dummyInteger); // Use ImGui::DragInt or ImGui::DragFloat for larger range of numbers.
//        ImGui::Text("Value is: %i", dummyInteger); // Use C printf formatting rules (%i is a signed integer)
        ImGui::TextWrapped("Welcome, use WASD, space and Lcontrol to move the camera. Use '1', '2' and '3' to switch between camera modes. Use the buttons to move through the scenes");
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
                ImGui::TextWrapped("Displaying Solar System");
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
                    currentDir = newCoords.second;
                }
            }

            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), currentPos);
            glm::mat4 rotationMatrix = getRotationMatrix(currentDir);
            m_modelMatrix = translationMatrix * rotationMatrix;


            //camera movement
            if (viewMode == 1) {
                glm::vec3 rightDir = glm::normalize(glm::cross(viewDir, glm::vec3(0.0f, 1.0f, 0.0f)));
                if (moveForward) {
                    cameraPos += cameraSpeed * viewDir * glm::vec3(1.0f, 0.0f, 1.0f);
                }
                if (moveBackward) {
                    cameraPos += -cameraSpeed * viewDir * glm::vec3(1.0f, 0.0f, 1.0f);
                }
                if (moveLeft) {
                    cameraPos += -cameraSpeed * rightDir * glm::vec3(1.0f, 0.0f, 1.0f);
                }
                if (moveRight) {
                    cameraPos += cameraSpeed * rightDir * glm::vec3(1.0f, 0.0f, 1.0f);
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
            std::cout << "Scene: " << currentScene << std::endl;
            switch (currentScene) {
                case 0:
                    const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
                    // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
                    // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
                    const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));

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
                    const glm::mat4 mvp = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
                    const glm::mat3 nModel = glm::inverseTranspose(glm::mat3(m_modelMatrix));
                    for(GPUMesh& mesh : ss_mesh){
                        m_defaultShader.bind();
                        glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvp));
                        glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(nModel));
                        glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                        glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
                        mesh.draw(m_defaultShader);
                    }
                    break;
                case 2:
                    break;
                case 3:
                    break;
                case 4:
                    break;
                case 5:
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
       // std::cout << "Mouse at position: " << cursorPos.x << " " << cursorPos.y << std::endl;
    }

    // If one of the mouse buttons is pressed this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseClicked(int button, int mods)
    {
       // std::cout << "Pressed mouse button: " << button << std::endl;
    }

    // If one of the mouse buttons is released this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseReleased(int button, int mods)
    {
        //std::cout << "Released mouse button: " << button << std::endl;
    }

private:
    Window m_window;

    // Shader for default rendering and for depth rendering
    Shader m_defaultShader;
    Shader m_shadowShader;


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
	std::vector<BezierSpline> animPath = loadSplines(RESOURCE_ROOT "resources/bezier_splines.json");
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

    //ImGui Studd
    bool triggered{ false };
    int currentScene = 0;

    //Solar system stuff
    Shader solar_system;
    std::vector<GPUMesh> ss_mesh;

};

int main()
{
    Application app;
    app.update();

    return 0;
}
