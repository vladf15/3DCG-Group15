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
#include"WangTile.h"
#include"WangTile.cpp"


struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    float life;
    float size;
};
struct Coord {
    int x; 
    int y;
};

class Application {
public:

    Application()
        : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL41)
        , m_texture(RESOURCE_ROOT "resources/checkerboard.png")
		, particle_texture(RESOURCE_ROOT "resources/Flame02_16x4_1.png")
        // Load textures for PBR shading 
        , kaTexture(RESOURCE_ROOT "resources/Stone/PavingStones142_2K-PNG_AmbientOcclusion.png")
        , kdTexture(RESOURCE_ROOT "resources/Stone/PavingStones142_2K-PNG_Color.png")
        , displacementTexture(RESOURCE_ROOT "resources/Stone/PavingStones142_2K-PNG_Displacement.png")
        , normalTexture(RESOURCE_ROOT "resources/Stone/PavingStones142_2K-PNG_NormalGL.png")
        , roughnessTexture(RESOURCE_ROOT "resources/Stone/PavingStones142_2K-PNG_Roughness.png")
        , breadOcclusion(RESOURCE_ROOT "resources/bread/3DBread012_HQ-2K-PNG_AmbientOcclusion.png")
        , breadColor(RESOURCE_ROOT "resources/bread/3DBread012_HQ-2K-PNG_Color.png")
        , breadNormals(RESOURCE_ROOT "resources/bread/3DBread012_HQ-2K-PNG_NormalGL.png")
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

		particle_mesh = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/quad.obj");
        m_meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/dragon.obj");
        
        //Solar system initialization
        ss_mesh = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/sphere.obj");
        solar_system_ts = 0.0f;
        bread = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/bread/3DBread012_HQ-2K-PNG.obj");

        ////////////////
        //PARTICLE SETUP
        ////////////////
		for (int i = 0; i < particle_num; i++) {
			Particle p;
			p.position = particleSource;
            p.velocity = 0.002f * glm::vec3((float)rand() / RAND_MAX - 0.5f, 2 * (float)rand() / RAND_MAX, (float)rand() / RAND_MAX - 0.5f);
			p.life = 2.0f + 1.0f * (float)rand() / RAND_MAX;
            p.size = 0.4f;
			particles.push_back(p);
		}
        pbrMeshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/Stone/surface.obj");
        //pbrMeshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/bread/3DBread012_HQ-2K-PNG.obj");
        

        //wang tiles initialization
        full_tileset.push_back(WangTile(0,  false,  false,  false,  false));
        full_tileset.push_back(WangTile(1,  true,   false,  false,  false));
        full_tileset.push_back(WangTile(2,  false,  true,   false,  false));
        full_tileset.push_back(WangTile(3,  true,   true,   false,  false));
        full_tileset.push_back(WangTile(4,  false,  false,  true,   false));
        full_tileset.push_back(WangTile(5,  true,   false,  true,   false));
        full_tileset.push_back(WangTile(6,  false,  true,   true,   false));
        full_tileset.push_back(WangTile(7,  true,   true,   true,   false));
        full_tileset.push_back(WangTile(8,  false,  false,  false,  true));
        full_tileset.push_back(WangTile(9,  true,   false,  false,  true));
        full_tileset.push_back(WangTile(10, false,  true,   false,  true));
        full_tileset.push_back(WangTile(11, true,   true,   false,  true));
        full_tileset.push_back(WangTile(12, false,  false,  true,   true));
        full_tileset.push_back(WangTile(13, true,   false,  true,   true));
        full_tileset.push_back(WangTile(14, false,  true,   true,   true));
        full_tileset.push_back(WangTile(15, true,   true,   true,   true));
        wang_tile_mode = 0;
        create_maze();
        
        try {
            ShaderBuilder defaultBuilder;
            defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
            m_defaultShader = defaultBuilder.build();

            ShaderBuilder shadowBuilder;
            shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
            shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "Shaders/shadow_frag.glsl");
            m_shadowShader = shadowBuilder.build();

			ShaderBuilder animatedTextureBuilder;
			animatedTextureBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/animated_texture_vert.glsl");
			animatedTextureBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/animated_texture_frag.glsl");
            m_animatedTextureShader = animatedTextureBuilder.build();

            ShaderBuilder ssBuilder;
            ssBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/ss_vert.glsl");
            ssBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/ss_frag.glsl");
            m_solarShader = ssBuilder.build();

            ShaderBuilder pbrBuilder; 
            pbrBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/pbr_vert.glsl");
            pbrBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/pbr_frag.glsl");
            m_pbrShader = pbrBuilder.build();

            ShaderBuilder lightBuilder;
            lightBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/light_vert.glsl");
            lightBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_frag.glsl");
            lightShader = lightBuilder.build();

            ShaderBuilder advancedBuilder;
            advancedBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/advanced_pbr_vert.glsl");
            advancedBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/advanced_pbr_frag.glsl");
            advancedPbrShader = advancedBuilder.build();


            // Any new shaders can be added below in similar fashion.
            // ==> Don't forget to reconfigure CMake when you do!
            //     Visual Studio: PROJECT => Generate Cache for ComputerGraphics
            //     VS Code: ctrl + shift + p => CMake: Configure => enter
            // ....
        } catch (ShaderLoadingException e) {
            std::cerr << e.what() << std::endl;
        }
    }
    void create_maze() {
        maze = {};
        // std::cout << "Creating a maze of " << maze_width << "X" << maze_height << std::endl;
        std::vector<std::vector<bool>> visited = {};
        std::vector<std::vector<Tile>> path_matrix = {};
        for(int j = 0; j <maze_height; j++){
            visited.push_back({});
            path_matrix.push_back({});
            maze.push_back({});
            for(int i = 0; i < maze_width; i++){
                visited[j].push_back(false);
                path_matrix[j].push_back(Tile{-1,-1,-1,-1});
                maze[j].push_back(-1);
            }
        }
        std::vector<Coord> visited_list = {};
        std::random_device rd;
        std::mt19937 gen(rd());

        // Only build the maze if both ends have a size larger than 0
        if(maze_height > 0 && maze_width > 0){
            std::uniform_int_distribution<> x_dist(0, maze_width - 1);
            std::uniform_int_distribution<> y_dist(0, maze_height - 1);
            int x = x_dist(gen);
            int y = y_dist(gen);
            visited_list.push_back(Coord{x,y});
            visited[y][x] = true;
            while(visited_list.size() > 0){
                std::uniform_int_distribution<> vl_rand(0, visited_list.size() - 1);
                int r_coord = vl_rand(gen);
                Coord r_xy = visited_list[r_coord];
                std::vector<Coord> neighbors = {};
                if(r_xy.x > 0) if(!visited[r_xy.y][r_xy.x - 1]) neighbors.push_back(Coord{r_xy.x - 1, r_xy.y});
                if(r_xy.x < maze_width - 1) if(!visited[r_xy.y][r_xy.x + 1]) neighbors.push_back(Coord{r_xy.x + 1, r_xy.y});
                if(r_xy.y > 0) if(!visited[r_xy.y - 1][r_xy.x]) neighbors.push_back(Coord{r_xy.x, r_xy.y - 1});
                if(r_xy.y < maze_height - 1) if(!visited[r_xy.y + 1][r_xy.x]) neighbors.push_back(Coord{r_xy.x, r_xy.y + 1});
                if(neighbors.size() == 0){
                    visited_list.erase(visited_list.begin() + r_coord);
                } else {
                    std::uniform_int_distribution<> n_rand(0, neighbors.size() - 1);
                    int n_coord = n_rand(gen);
                    Coord n_xy = neighbors[n_coord];
                    if(r_xy.x > n_xy.x){
                        path_matrix[r_xy.y][r_xy.x].left = 1;
                        path_matrix[n_xy.y][n_xy.x].right = 1;
                    }
                    if(r_xy.x < n_xy.x){
                        path_matrix[r_xy.y][r_xy.x].right = 1;
                        path_matrix[n_xy.y][n_xy.x].left = 1;
                    }
                    if(r_xy.y > n_xy.y){
                        path_matrix[r_xy.y][r_xy.x].down = 1;
                        path_matrix[n_xy.y][n_xy.x].up = 1;
                    }
                    if(r_xy.y < n_xy.y){
                        path_matrix[r_xy.y][r_xy.x].up = 1;
                        path_matrix[n_xy.y][n_xy.x].down = 1;
                    }
                    visited[n_xy.y][n_xy.x] = true;
                    visited_list.push_back(n_xy);
                }
            }

            // all undecided paths are no path
            for(int j = 0; j < maze_height; j++){
                for(int i = 0; i < maze_width; i++){
                    if(path_matrix[j][i].up == -1) path_matrix[j][i].up = 0;
                    if(path_matrix[j][i].right == -1) path_matrix[j][i].right = 0;
                    if(path_matrix[j][i].down == -1) path_matrix[j][i].down = 0;
                    if(path_matrix[j][i].left == -1) path_matrix[j][i].left = 0;

                }
            }

            for(int j = 0; j < maze_height; j++){
                for(int i = 0; i < maze_width; i++){
                    for(int k = 0; k < full_tileset.size(); k++){
                        if(full_tileset[k].matches_tile(path_matrix[j][i])){
                            maze[j][i] = k;
                        }
                    }
                }
            }


        }



        std::cout << "Visited: " << std::endl;
        for(int j = maze_height - 1; j >= 0; j--){
            for(int i = 0; i < maze_width; i++){
                std::cout << visited[j][i] << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "Path matrix: " << std::endl;
        for(int j = maze_height - 1; j >= 0; j--){
            for(int i = 0; i < maze_width; i++){
                std::cout << "{" << path_matrix[j][i].up << "," << path_matrix[j][i].right << "," << path_matrix[j][i].down << "," << path_matrix[j][i].left << "}"<< " ";
            }
            std::cout << std::endl;
        }
        std::cout << "Maze: " << std::endl;
        for(int j = maze_height - 1; j >= 0; j--){
            for(int i = 0; i < maze_width; i++){
                std::cout << maze[j][i] << " ";
            }
            std::cout << std::endl;
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
                triggered = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Next Scene")) {
            triggered == false;
            currentScene++;
    
        }
        ImGui::Checkbox("Use material from .mtl if no texture", &m_useMaterial);
        ImGui::Separator();
        switch(currentScene) {
            case 0:
                ImGui::TextWrapped("This is the first scene. You can use the '1', '2' and '3' keys to switch between camera modes.");
                ImGui::Checkbox("Enable PBR", &usePbr);
                if (usePbr) {
                    ImGui::Checkbox("Editable material parameters", &editableMaterial);
                    if (editableMaterial) {
                        ImGui::ColorEdit3("Diffuse color", &baseColor.x);
                        ImGui::ColorEdit3("Specular color (albedo)", &albedo.x);
                    }
                    ImGui::DragFloat("Metallic", &metallic, 0.01f, 0.0f, 1.0f);
                    ImGui::DragFloat("Roughness", &roughness, 0.01f, 0.0f, 1.0f);
                }
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
                ImGui::TextWrapped("Displaying Wang tiles scene");
                ImGui::Separator();
                ImGui::TextWrapped("Select the scene to visualize");
                if (ImGui::Button("Display Tileset")) {
                    wang_tile_mode = 0;
                }
                ImGui::SameLine();
                if (ImGui::Button("Infinite Terrain")) {
                    wang_tile_mode = 1;
                }
                ImGui::SameLine();
                if (ImGui::Button("Maze")) {
                    wang_tile_mode = 2;
                }
                ImGui::Separator();
                if (wang_tile_mode == 0) {
                    ImGui::TextWrapped("Displaying the full tileset");
                }
                else if (wang_tile_mode == 1) {
                    ImGui::TextWrapped("Infinite terrain!");
                }
                else {
                    ImGui::TextWrapped("Displaying the maze. Please adjust the width and height");
                    
                    ImGui::InputInt("Width", &t_maze_width);
                    ImGui::InputInt("Height", &t_maze_height);
                    if (ImGui::Button("Create maze")) {
                        if (t_maze_width > 0 && t_maze_height > 0) {
                            maze_width = t_maze_width;
                            maze_height = t_maze_height;
                            create_maze();
                        }
                    }
                }
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
                ImGui::TextWrapped("Displaying PBR shading with normal mapping and ambient occlusion mapping");
                ImGui::Checkbox("Use normal map", &useNormalMap);
                ImGui::ColorEdit3("Specular color (albedo)", &albedo.x);
                ImGui::DragFloat("Metallic", &metallic, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Roughness", &roughness, 0.01f, 0.0f, 1.0f);
                break;
            default:
                ImGui::TextWrapped("Default Scene");
                break;
        }
        ImGui::Separator();

        ImGui::Text("Lights");

        if (ImGui::Button("Create Light")) {
            selectedLightIndex = lightPositions.size();
            lightPositions.emplace_back(-1.0, 0.0, 0.0);
            lightColors.emplace_back(1.0f, 1.0f, 1.0f);
        }

        // Button for clearing lights
        if (ImGui::Button("Reset Lights")) {
            lightPositions.clear();
            lightPositions.emplace_back(1.0f, 1.0f, 1.0f);
            lightColors.clear();
            lightColors.emplace_back(1.0f, 1.0f, 1.0f);
        }

        std::vector<std::string> itemStrings = {};
        for (size_t i = 0; i < lightPositions.size(); i++) {
            auto string = "Light " + std::to_string(i);
            itemStrings.push_back(string);
        }

        std::vector<const char*> itemCStrings = {};
        for (const auto& string : itemStrings) {
            itemCStrings.push_back(string.c_str());
        }

        int tempSelectedItem = static_cast<int>(selectedLightIndex);
        if (ImGui::ListBox("Lights", &tempSelectedItem, itemCStrings.data(), (int)itemCStrings.size(), 4)) {
            selectedLightIndex = static_cast<size_t>(tempSelectedItem);
        }

        ImGui::DragFloat3("Position", &lightPositions[selectedLightIndex].x, 0.05f, -100.0f, 100.0f);
        ImGui::ColorEdit3("Color", &lightColors[selectedLightIndex].x);

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
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
            const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
            // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
            // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
            const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));
            switch (currentScene) {
                case 0:
                    for (GPUMesh& mesh : m_meshes) {
                        if (usePbr) {
                            m_pbrShader.bind();
                            glUniformMatrix4fv(m_pbrShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                            glUniformMatrix4fv(m_pbrShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
                            glUniformMatrix3fv(m_pbrShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
                            glUniform3fv(m_pbrShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));

                            glUniform1i(m_pbrShader.getUniformLocation("useMaterial"), static_cast<int>(m_useMaterial));

                            // Pass PBR parameters
                            glUniform1f(m_pbrShader.getUniformLocation("metallic"), metallic);
                            glUniform1f(m_pbrShader.getUniformLocation("roughness"), roughness);
                            glUniform3fv(m_pbrShader.getUniformLocation("albedo"), 1, glm::value_ptr(albedo));
                            glUniform1i(m_pbrShader.getUniformLocation("overrideBase"), static_cast<int>(editableMaterial));
                            glUniform3fv(m_pbrShader.getUniformLocation("baseColor"), 1, glm::value_ptr(baseColor));

                            // Pass lights
                            glUniform1i(m_pbrShader.getUniformLocation("num_lights"), lightPositions.size());
                            glUniform3fv(m_pbrShader.getUniformLocation("lightPositions"), lightPositions.size(), glm::value_ptr(lightPositions[0]));
                            glUniform3fv(m_pbrShader.getUniformLocation("lightColors"), lightColors.size(), glm::value_ptr(lightColors[0]));

                            glUniform1i(m_pbrShader.getUniformLocation("hasTexCoords"), static_cast<int>(mesh.hasTextureCoords()));
                            if (mesh.hasTextureCoords()) {
                                kdTexture.bind(GL_TEXTURE0);
                                glUniform1i(m_pbrShader.getUniformLocation("colorMap"), 0);
                                /*displacementTexture.bind(GL_TEXTURE1);
                                glUniform1i(m_pbrShader.getUniformLocation("displacementMap"), 1);
                                normalTexture.bind(GL_TEXTURE2);
                                glUniform1i(m_pbrShader.getUniformLocation("normalMap"), 2);
                                kaTexture.bind(GL_TEXTURE3);
                                glUniform1i(m_pbrShader.getUniformLocation("kaMap"), 3);
                                roughnessTexture.bind(GL_TEXTURE4);
                                glUniform1i(m_pbrShader.getUniformLocation("roughnessMap"), 4);*/
                            }
                            mesh.draw(m_pbrShader);
                        }
                        else {
                            m_defaultShader.bind();
                            glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                            glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
                            glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
                            if (mesh.hasTextureCoords()) {
                                m_texture.bind(GL_TEXTURE0);
                                glUniform1i(m_defaultShader.getUniformLocation("colorMap"), 0);
                                glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_TRUE);
                                glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);
                            }
                            else {
                                glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                                glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
                            }
                            mesh.draw(m_defaultShader);
                        }
                    }

					//PARTICLE RENDERING
                    glDepthFunc(GL_LESS);
                    glDepthMask(GL_FALSE);
                    glEnable(GL_BLEND);
                    glEnable(GL_DEPTH_TEST);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    for (Particle& p : particles) {
                        //animated texture on particles using a quad mesh
                        //texture taken from:
                        // https://unity.com/blog/engine-platform/free-vfx-image-sequences-flipbooks
                        glm::mat4 particleModel = glm::mat4(1.0f);
                        particleModel = glm::translate(particleModel, p.position);
                        particleModel = glm::scale(particleModel, glm::vec3(p.size));
						glm::vec3 directionToCamera = glm::normalize(p.position - cameraPos);
						float angle = -atan2(directionToCamera.z, directionToCamera.x);
						//rotate the particle around y axis to face the camera
						const glm::mat4 particleModelMatrix = glm::rotate(particleModel, angle, glm::vec3(0.0f, 1.0f, 0.0f));

                        const glm::mat4 particleMvp = m_projectionMatrix * m_viewMatrix * particleModelMatrix;
                        const glm::mat3 particleNormalModelMatrix = glm::inverseTranspose(glm::mat3(particleModelMatrix));
                        //fade out the particles as they despawn
						float maxAlpha = glm::clamp(0.1f + 0.9f * p.life / 1.5f , 0.0f, 1.0f);
                        for (GPUMesh& mesh : particle_mesh){
                            m_animatedTextureShader.bind();
                            glUniformMatrix4fv(m_animatedTextureShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(particleMvp));
                            glUniformMatrix3fv(m_animatedTextureShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(particleNormalModelMatrix));
                            particle_texture.bind(GL_TEXTURE5);
                            glUniform1i(m_animatedTextureShader.getUniformLocation("tex"), 5);
                            glUniform1i(m_animatedTextureShader.getUniformLocation("rows"), 4);
                            glUniform1i(m_animatedTextureShader.getUniformLocation("columns"), 16);
                            glUniform1f(m_animatedTextureShader.getUniformLocation("time"), glfwGetTime());
                            glUniform1f(m_animatedTextureShader.getUniformLocation("maxAlpha"), maxAlpha);
                            glUniform1f(m_animatedTextureShader.getUniformLocation("animationSpeed"), 40.0f);
                            mesh.draw(m_animatedTextureShader);
                        }

						p.position += p.velocity;
						p.size = glm::clamp(0.1f + 0.3f * p.life / 1.5f, 0.0f, 0.4f);
                        p.life -= 0.01f;
						//randomized velocity when spawning
                        if (p.life <= 0.0f) {
                            p.position = particleSource;
                            p.velocity = 0.002f * glm::vec3((float)rand() / RAND_MAX - 0.5f, 2 * (float)rand() / RAND_MAX, (float)rand() / RAND_MAX - 0.5f);
                            p.life = 2.0f + 1.0f * (float)rand() / RAND_MAX;
							p.size = 0.4f;
                        }
                    }
                    glDisable(GL_BLEND);
                    glDisable(GL_DEPTH_TEST);
                    glDepthMask(GL_TRUE);
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
                        glUniform4f(m_solarShader.getUniformLocation("theColor"), sun_color.x, sun_color.y, sun_color.z, 1.0f);
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
                    if (wang_tile_mode == 0) {
                        for (int i = 0; i < 4; i++) {
                            for (int j = 0; j < 4; j++) {
                                int k = 4 * i + j;
                                glm::vec3 newPos = 2.5f * glm::vec3(-j, 0.0f, i);
                                glm::mat4 tile_model = glm::translate(glm::mat4(1.0f), newPos);
                                glm::mat4 mvp = m_projectionMatrix * m_viewMatrix * tile_model;
                                glm::mat3 nModel = glm::inverseTranspose(glm::mat3(tile_model));
                                for (GPUMesh& m : full_tileset[k].mesh) {
                                    m_solarShader.bind();
                                    glUniformMatrix4fv(m_solarShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvp));
                                    glUniformMatrix3fv(m_solarShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(nModel));
                                    glUniform4f(m_solarShader.getUniformLocation("theColor"), 1.0f, 1.0f, 0.0f, 1.0f);
                                    glUniform3f(m_solarShader.getUniformLocation("camPos"), cameraPos.x, cameraPos.y, cameraPos.z);
                                    m.draw_no_mat(m_solarShader);
                                }

                            }
                        }

                    }
                    else if(wang_tile_mode == 1) {
                        
                    }
                    else{
                        for(int j = 0; j < maze_height; j++) {
                            for(int i = 0; i < maze_width; i++) {
                                int k = maze[j][i];
                                glm::vec3 newPos = 2.0f * glm::vec3(-i,0.0f,j);
                                glm::mat4 tile_model = glm::translate(glm::mat4(1.0f), newPos);
                                glm::mat4 mvp = m_projectionMatrix * m_viewMatrix * tile_model;
                                glm::mat3 nModel = glm::inverseTranspose(glm::mat3(tile_model));
                                for (GPUMesh& m : full_tileset[k].mesh) {
                                    m_solarShader.bind();
                                    glUniformMatrix4fv(m_solarShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvp));
                                    glUniformMatrix3fv(m_solarShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(nModel)); 
                                    glUniform4f(m_solarShader.getUniformLocation("theColor"), 1.0f, 1.0f, 0.0f, 1.0f);
                                    glUniform3f(m_solarShader.getUniformLocation("camPos"), cameraPos.x, cameraPos.y, cameraPos.z);
                                    m.draw_no_mat(m_solarShader);
                                }
                            }
                        }
                    }
                    break;
                case 3:
                    break;
                case 4:
                    break;
                case 5:
                    break;
                case 6: 
                    for (GPUMesh& mesh : bread) {
                        advancedPbrShader.bind();
                        glUniformMatrix4fv(advancedPbrShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                        glUniformMatrix4fv(advancedPbrShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
                        glUniformMatrix3fv(advancedPbrShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
                        glUniform3fv(advancedPbrShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));

                        glUniform1i(advancedPbrShader.getUniformLocation("useMaterial"), static_cast<int>(m_useMaterial));
                        glUniform1i(advancedPbrShader.getUniformLocation("useNormalMap"), static_cast<int>(useNormalMap));

                        // Pass PBR parameters
                        glUniform1f(advancedPbrShader.getUniformLocation("metallic"), metallic);
                        glUniform1f(advancedPbrShader.getUniformLocation("roughness"), roughness);
                        glUniform3fv(advancedPbrShader.getUniformLocation("albedo"), 1, glm::value_ptr(albedo));
                        glUniform1i(advancedPbrShader.getUniformLocation("overrideBase"), static_cast<int>(editableMaterial));
                        glUniform3fv(advancedPbrShader.getUniformLocation("baseColor"), 1, glm::value_ptr(baseColor));

                        // Pass lights
                        glUniform1i(advancedPbrShader.getUniformLocation("num_lights"), lightPositions.size());
                        glUniform3fv(advancedPbrShader.getUniformLocation("lightPositions"), lightPositions.size(), glm::value_ptr(lightPositions[0]));
                        glUniform3fv(advancedPbrShader.getUniformLocation("lightColors"), lightColors.size(), glm::value_ptr(lightColors[0]));

                        glUniform1i(advancedPbrShader.getUniformLocation("hasTexCoords"), static_cast<int>(mesh.hasTextureCoords()));
                        if (mesh.hasTextureCoords()) {
                            breadColor.bind(GL_TEXTURE0);
                            glUniform1i(advancedPbrShader.getUniformLocation("colorMap"), 0);
                            breadNormals.bind(GL_TEXTURE1);
                            glUniform1i(advancedPbrShader.getUniformLocation("normalMap"), 1);
                            breadOcclusion.bind(GL_TEXTURE2);
                            glUniform1i(advancedPbrShader.getUniformLocation("kaMap"), 2);
                            /*displacementTexture.bind(GL_TEXTURE3);
                            glUniform1i(m_pbrShader.getUniformLocation("displacementMap"), 3);
                            
                            
                            roughnessTexture.bind(GL_TEXTURE4);
                            glUniform1i(m_pbrShader.getUniformLocation("roughnessMap"), 4);*/
                        }
                        mesh.draw(advancedPbrShader);
                    }

                    break;
                default:
                    break;
            }

            lightShader.bind();
            if (selectedLightIndex >= 0) {
                const glm::vec4 screenPos = m_projectionMatrix * m_viewMatrix * glm::vec4(lightPositions[selectedLightIndex], 1.0f);
                const glm::vec3 color{ 1, 1, 0 };

                glPointSize(40.0f);
                glUniform4fv(lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));
                glUniform3fv(lightShader.getUniformLocation("color"), 1, glm::value_ptr(color));
                glDrawArrays(GL_POINTS, 0, 1);
                glBindVertexArray(0);
            }
            for (int i = 0; i < lightPositions.size(); i++) {
                const glm::vec4 screenPos = m_projectionMatrix * m_viewMatrix * glm::vec4(lightPositions[i], 1.0f);
                // const glm::vec3 color { 1, 0, 0 };

                glPointSize(10.0f);
                glUniform4fv(lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));
                glUniform3fv(lightShader.getUniformLocation("color"), 1, glm::value_ptr(lightColors[i]));
                glDrawArrays(GL_POINTS, 0, 1);
                glBindVertexArray(0);

            }

            // Processes input and swaps the window buffer
            m_window.swapBuffers();
        }
    }

    glm::vec3 calculateTangent(const glm::vec3& e1, const glm::vec3& e2, const glm::vec2& duv1, const glm::vec2& duv2) {
        float f = 1.0f / (duv1.x * duv2.y - duv2.x * duv1.y);

        glm::vec3 t{ (duv2.y * e1.x - duv1.y * e2.x),
                    (duv2.y * e1.y - duv1.y * e2.y),
                    (duv2.y * e1.z - duv1.y * e2.z) };

        return f * t;
    }

    glm::vec3 calculateBitangent(const glm::vec3& e1, const glm::vec3& e2, const glm::vec2& duv1,
        const glm::vec2& duv2) {
        float f = 1.0f / (duv1.x * duv2.y - duv2.x * duv1.y);

        glm::vec3 bit{ (-duv2.x * e1.x + duv1.x * e2.x),
                      (-duv2.x * e1.y + duv1.x * e2.y),
                      (-duv2.x * e1.z + duv1.x * e2.z) };

        return f * bit;
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

	Shader m_animatedTextureShader;

    bool usePbr{ false };
    Shader m_pbrShader;
    Shader advancedPbrShader;

    bool useNormalMap{ true };
    bool editableMaterial{ true };
    glm::vec3 baseColor{ 0.0f, 1.0f, 0.5f };
    glm::vec3 albedo{ 1.0f, 0.5f, 0.0f };
    
    float metallic{ 0.0f };
    float roughness{ 0.3f };

    Texture kaTexture;
    Texture kdTexture;
    Texture normalTexture;
    Texture roughnessTexture;
    Texture displacementTexture;
    std::vector<GPUMesh> pbrMeshes;

    // bread
    std::vector<GPUMesh> bread;
    Texture breadOcclusion;
    Texture breadColor;
    Texture breadNormals;

    Shader lightShader;
    std::vector<glm::vec3> lightPositions{ glm::vec3(1.0f, 1.0f, 1.0f) };
    std::vector<glm::vec3> lightColors{ glm::vec3(1.0f, 1.0f, 1.0f) };
    int selectedLightIndex{ 0 };
    int maxLights{ 32 }; // To allocate space in shaders

    std::vector<GPUMesh> m_meshes;
    Texture m_texture;
    bool m_useMaterial{ true };


   

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_projectionMatrix = glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 30.0f);
    glm::mat4 m_viewMatrix = glm::lookAt(glm::vec3(-1, 1, -1), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 m_modelMatrix{ 1.0f };

    //Animation variables
    bool inAnimation{ false };
    int currentAnim = 0;
	float animationTimer{ 0.0f };
	float animationLength{ 100.0f };
	std::vector<BezierSpline> animPath = loadSplines(RESOURCE_ROOT "resources/bezier_splines.json", false);
	glm::vec3 currentPos{ 0.0f };
    glm::vec3 currentDir{ 0.0f, 0.0f, 1.0f };

    //Particle variables
    std::vector<GPUMesh> particle_mesh;
    std::vector<Particle> particles;
    Texture particle_texture;
	int particle_num = 10;
	glm::vec3 particleSource = glm::vec3(0.0f, 0.5f, 0.0f);
    //glm::mat4 particleModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.5f, 0.0f));



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

    //Wang Tiles
    std::vector<WangTile> full_tileset;
    int wang_tile_mode; // Mode of visualization: 0 for the full tileset, 1 for the infinite terrain, 2 for the maze
    int maze_width = 0;
    int maze_height = 0;
    int t_maze_width = 0;
    int t_maze_height = 0;
    std::vector<std::vector<int>> maze;
};

int main()
{
    Application app;
    app.update();

    return 0;
}
