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
#include"Grammar.h"
#include"Grammar.cpp"

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
struct TerrainTile {
    int id;
    Coord coord;
    Tile tile;
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
        branch_mesh = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/branch.obj");
        //Solar system initialization
        ss_mesh = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/sphere.obj");
        water_mesh = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/water.obj");
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
        ///////////////////////
		//POST-PROCESSING SETUP
        ///////////////////////
		//set up a framebuffer to capture the scene and then apply post-processing effects
		//using the color and depth textures from the framebuffer
        //inspiration for the method used: https://learnopengl.com/Advanced-Lighting/Bloom
        //results rendered to a fullscreen quad
        float postProcessingQuad[] = {
            //positions       //texcoords
            -1.0f,  1.0f,      0.0f, 1.0f,
            -1.0f, -1.0f,      0.0f, 0.0f,
             1.0f, -1.0f,      1.0f, 0.0f,
             1.0f,  1.0f,      1.0f, 1.0f
        }; 
        glGenVertexArrays(1, &postProcessingVAO);
        glGenBuffers(1, &postProcessingVBO);
        glBindVertexArray(postProcessingVAO);
        glBindBuffer(GL_ARRAY_BUFFER, postProcessingVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(postProcessingQuad), postProcessingQuad, GL_STATIC_DRAW);

        //positions
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        //texcoords
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

		//buffers and textures for post-processing effects
        glGenFramebuffers(1, &sceneBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, sceneBuffer);
		int windowWidth, windowHeight;
		windowWidth = m_window.getWindowSize().x;
		windowHeight = m_window.getWindowSize().y;

		glGenTextures(1, &sceneColorTexture);
		glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTexture, 0);

        glGenTextures(1, &postProcessingColorTexture1);
        glBindTexture(GL_TEXTURE_2D, postProcessingColorTexture1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, postProcessingColorTexture1, 0);

		GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);

        glGenTextures(1, &postProcessingColorTexture2);
        glBindTexture(GL_TEXTURE_2D, postProcessingColorTexture2);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenTextures(1, &sceneDepthTexture);
		glBindTexture(GL_TEXTURE_2D, sceneDepthTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, windowWidth, windowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sceneDepthTexture, 0);

		glGenFramebuffers(1, &postProcessingBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


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
        terrain_width = 1;
        terrain_height = 1;
        t_terrain_width = 1;
        t_terrain_height = 1;
        current_coord = Coord{0,0};
        //  int terrain_width, terrain_height, t_terrain_width, t_terrain_height;
        

        // L Systems
        
	    grammars = {};

        std::string s = "F";
        std::vector<std::tuple<char,std::string>> rls = {};
        std::tuple<char, std::string> rule = std::make_tuple('F', "FF+[+F-F-F]-[-F+F+F]");
        rls.push_back(rule);
        grammars.push_back(Grammar(s, rls, 5));

        s = "X";
        rls = {};
        rls.push_back(std::make_tuple('F', "FF"));
        rls.push_back(std::make_tuple('X', "F[+X]F[-X]+X"));
        grammars.push_back(Grammar(s, rls, 7));

        s = "F";
        rls = {};
        rls.push_back(std::make_tuple('F', "FF*[++F-F-F]*[++F-F-F]*[++F-F-F]"));
        grammars.push_back(Grammar(s, rls, 4));

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

            ShaderBuilder waterBuilder;
            waterBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/water_vert.glsl");
            waterBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/water_frag.glsl");
            waterShader = waterBuilder.build();

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

			ShaderBuilder bloomBuilder;
            bloomBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/blur_vert.glsl");
            bloomBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/bloom_frag.glsl");
            m_bloomShader = bloomBuilder.build();

            ShaderBuilder dofBuilder;
            dofBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/blur_vert.glsl");
            dofBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/dof_frag.glsl");
            m_dofShader = dofBuilder.build();
            


            // Any new shaders can be added below in similar fashion.
            // ==> Don't forget to reconfigure CMake when you do!
            //     Visual Studio: PROJECT => Generate Cache for ComputerGraphics
            //     VS Code: ctrl + shift + p => CMake: Configure => enter
            // ....
        } catch (ShaderLoadingException e) {
            std::cerr << e.what() << std::endl;
        }
    }

    Coord get_tile_coord(){
        float x = cameraPos.x;
        float z = cameraPos.z;
        // std::cout << "Now at " << x << ", " << z << std::endl;
        float test_x = floor(x) == ceil(x) ? x - terrain_epsilon : x;
        float test_z = floor(z) == ceil(z) ? z - terrain_epsilon : z;

        return Coord{int(ceil(test_x)), int(floor(test_z))};
    }
    std::vector<int> get_possible_tiles(int up, int right, int down, int left){
        std::vector<int> possible_tiles = {};
        for(int i = 1; i < full_tileset.size(); i++){
            if(full_tileset[i].matches_tile(Tile{up, right, down, left})){
                possible_tiles.push_back(i);
            }
        }
        return possible_tiles;
    }
    void create_infinite_terrain(){
        terrain = {};
        std::random_device rd;
        std::mt19937 gen(rd());
        // Initializing the terrain as a matrix of all tiles id = -1 and coord = (0,0)
        for(int j = 0; j < (2 * terrain_height + 1); j++){
            terrain.push_back({});
            for(int i = 0; i < (2 * terrain_width + 1); i++){
                int x_coord = -i + current_coord.x + terrain_width;
                int y_coord = j + current_coord.y - terrain_height;
                int left = i > 0 ? terrain[j][i - 1].tile.right : -1;
                int down = j > 0 ? terrain[j - 1][i].tile.up : -1;
                int right = -1;
                int up = -1;
                int id = -1;
                std::vector<int> possible_tiles = get_possible_tiles(down, right, down, left);
                if(possible_tiles.size() > 0){
                    std::uniform_int_distribution<> dis(0, possible_tiles.size() - 1);
                    id = possible_tiles[dis(gen)];
                    up = full_tileset[id].up;
                    right = full_tileset[id].right;
                    down = full_tileset[id].down;
                    left = full_tileset[id].left;
                }
                else{
                    std::cout << "ERRORRRRRRRRRRRRRRRRRR" << std::endl;
                }
                terrain[j].push_back(TerrainTile{id, Coord{x_coord, y_coord}, Tile{up, right, down, left}});

            }
        }
        std::cout << "Current coord: " << current_coord.x << "," << current_coord.y << std::endl;
        std::cout << "Creating an infinite terrain of " << 2 * terrain_width  + 1<< "X" << 2 * terrain_height + 1<< std::endl;
        for(int j = 2 * terrain_height; j >= 0; j--){
            for(int i = 0; i < (2 * terrain_width + 1); i++){
                std::cout << terrain[j][i].id << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "Coords: " << std::endl;
        for(int j = 2 * terrain_height; j >= 0; j--){
            for(int i = 0; i < (2 * terrain_width + 1); i++){
                std::cout << terrain[j][i].coord.x << "," << terrain[j][i].coord.y << "\t";
            }
            std::cout << std::endl;
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

    void renderToTexture() {
        glBindVertexArray(postProcessingVAO);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);
    }

    void userInterface()
    {
        // Use ImGui for easy input/output of ints, floats, strings, etc...
        ImGui::Begin("Window");
//        ImGui::InputInt("This is an integer input", &dummyInteger); // Use ImGui::DragInt or ImGui::DragFloat for larger range of numbers.
//        ImGui::Text("Value is: %i", dummyInteger); // Use C printf formatting rules (%i is a signed integer)
        ImGui::TextWrapped("Welcome, use WASD, Space and Lcontrol to move the camera.\n Left click + drag to change view direction.\n Use '1', '2' and '3' to switch between camera modes.\n Use the buttons to move through the scenes");
        ImGui::SliderFloat("Camera Speed", &cameraSpeed, 0.01f, 0.5f);
        ImGui::InputFloat("Camera FOV", &cam_fov);
        ImGui::InputFloat("Camera near plane", &cam_near_plane);
        ImGui::InputFloat("Camera far plane", &cam_far_plane);
        ImGui::Checkbox("Orthogonal Camera?", &ortho);

        if(ImGui::Button("Update Projection Matrix")) {
            if(!ortho) m_projectionMatrix = glm::perspective(glm::radians(cam_fov), 1.0f, cam_near_plane, cam_far_plane);
            else m_projectionMatrix = glm::ortho(-3.0f, 3.0f, -3.0f, 3.0f, cam_near_plane, cam_far_plane);
        }
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
        ImGui::Checkbox("Show light options", &showLightOptions);
        ImGui::Separator();
        if(showLightOptions) {
            ImGui::Text("Lights");

            if (ImGui::Button("Create Light")) {
                selectedLightIndex = lightPositions.size();
                lightPositions.emplace_back(-1.0, 0.0, 0.0);
                lightColors.emplace_back(1.0f, 1.0f, 1.0f);
            }

            // Button for clearing lights
            if (ImGui::Button("Reset Lights")) {
                lightPositions.clear();
                lightColors.clear();
                lightPositions.emplace_back(1.0f, 1.0f, 1.0f);
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
            ImGui::Separator();
        }
        switch(currentScene) {
            case 0:
                ImGui::TextWrapped("This is the first scene. You can use the '1', '2' and '3' keys to switch between camera modes.");
                ImGui::Checkbox("Enable PBR", &usePbr);
				ImGui::Checkbox("Use Bloom", &useBloom);
				ImGui::Checkbox("Use Depth of Field", &useDoF);
                ImGui::Checkbox("Show Particle Effects", &useParticles);
                if (useBloom) {
					ImGui::SliderFloat("Bloom Threshold", &bloomThreshold, 0.0f, 2.0f);
					ImGui::SliderInt("Bloom Radius", &bloomRadius, 1.0f, 20.0f);
                    ImGui::SliderInt("Bloom Passes", &bloomPasses, 1, 10);
					ImGui::SliderFloat("Bloom Intensity", &bloomIntensity, 0.0f, 1.0f);
                }
				if (useDoF) {
					ImGui::SliderFloat("DoF Focal Length", &focalLength, focalRange, 10.0f);
					ImGui::SliderFloat("DoF Focal Range", &focalRange, 0.5f, focalLength);
				}
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
                    current_coord = get_tile_coord();
                    terrain_width = 3;
                    terrain_height = 3;
                    create_infinite_terrain();
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
                    ImGui::TextWrapped("Adjust the dimensions of the terrain");
                    ImGui::InputInt("Width", &t_terrain_width);
                    ImGui::InputInt("Height", &t_terrain_height);
                    if(ImGui::Button("Adjust terrain")) {
                        if (t_terrain_width > 0 && t_terrain_height > 0) {
                            terrain_width = t_terrain_width;
                            terrain_height = t_terrain_height;
                            current_coord = get_tile_coord();
                            create_infinite_terrain();
                        }
                    }
                    ImGui::TextWrapped("Current Position: %i, %i", current_coord.x, current_coord.y);
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
                if (ImGui::SliderInt("Select the grammar", &selected_grammar, 0, grammars.size() - 1)) {
                    grammar_step = 0;
                }

                ImGui::TextWrapped(grammars[selected_grammar].PrintInfo(false).c_str());
                ImGui::SliderInt("Set the step", &grammar_step, 0, grammars[selected_grammar].steps.size() - 1);
                ImGui::TextWrapped("Randomize the branch angles?");
                ImGui::Checkbox("Randomize the vertical angle", &random_inclination);
                ImGui::Checkbox("Randomize the horizontal angle", &random_rotation);
                if (!random_inclination) ImGui::SliderFloat("Set the branch angle (vertical)", &branch_inclination, 0.0f, 90.0f);
                if(!random_rotation) ImGui::SliderFloat("Set the branch angle (horizontal)", &branch_rotation, 0.0f, 180.0f);
                ImGui::Separator();

                break;
            case 4:
                ImGui::TextWrapped("Displaying Procedural water surface. Select parameters:");
                ImGui::TextWrapped("Wave center");
                ImGui::SliderFloat("X", &t_wave_center.x, -30.0f, 30.0f);
                ImGui::SliderFloat("Z", &t_wave_center.y, -30.0f, 30.0f);
                ImGui::InputFloat("Wave speed", &t_wave_speed);
                ImGui::SliderFloat("Wave length (as a proportion of radius)", &t_lambda, 0.01f, 1.0f);
                ImGui::InputFloat("Wave period", &t_phi);
                ImGui::InputFloat("Dampening (if less than one amplifies instead)", &t_dampening);
                ImGui::InputFloat("Animation Time", &max_wave_time);
                if(ImGui::Button("Start animation")) {
                    if(!water_enabled) {
                        water_ts = 0.1f;
                        water_enabled = true;
                        wave_center = t_wave_center;
                        wave_speed = t_wave_speed;
                        lambda = t_lambda;
                        phi = t_phi;
                        dampening = t_dampening;
                    }else{
                        water_ts = -1.0f;
                    }
                }
                break;
            case 5:
                ImGui::TextWrapped("Displaying Inverse Kinematics Animation");
                ImGui::TextWrapped("NOT IMPLEMENTED, GO TO NEXT SCENE");
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
       

        ImGui::End();
    }
    

    void update()
    {

        
        auto t1 = std::chrono::high_resolution_clock::now();
        while (!m_window.shouldClose()) {
            // This is your game loop
            // Put your real-time logic and rendering in here
            m_window.updateInput();
            auto t2 = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(t2 - t1).count();
            t1 = t2;
            std::cout << "deltaTime: " << std::to_string(deltaTime) << std::endl;
          
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
            //bind framebuffer to add post processing effects
            if (useBloom || useDoF) {
                glBindFramebuffer(GL_FRAMEBUFFER, sceneBuffer);
            }
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
                    waterShader.bind();
                    glm::mat4 mat_model = glm::mat4(1.0f);
                    glm::mat4 mvp_mat = m_projectionMatrix * m_viewMatrix * mat_model;
                    glm::mat3 nModel_mat = glm::inverseTranspose(glm::mat3(mat_model));
                    for (GPUMesh& m : water_mesh) {
                        glUniformMatrix4fv(waterShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvp_mat));
                        glUniformMatrix3fv(waterShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(nModel_mat));
                        glUniform3f(waterShader.getUniformLocation("camPos"), cameraPos.x, cameraPos.y, cameraPos.z);
                        glUniform4f(waterShader.getUniformLocation("theColor"), 0.0f, 0.0f, 1.0f, 1.0f);
                        m.draw_no_mat(waterShader);
                    }
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
                            // Bloom threshold
							glUniform1f(m_pbrShader.getUniformLocation("bloomThreshold"), bloomThreshold);

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

                    ////////////////////
                    //PARTICLE RENDERING
                    ////////////////////
                    if (useParticles) {
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
                            float maxAlpha = glm::clamp(0.1f + 0.9f * p.life / 1.5f, 0.0f, 1.0f);
                            for (GPUMesh& mesh : particle_mesh) {
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
                    }

					/////////////////////////
					//POST-PROCESSING EFFECTS
					/////////////////////////
                    if (useBloom || useDoF) {
                        glBindFramebuffer(GL_FRAMEBUFFER, postProcessingBuffer);
                        int windowWidth, windowHeight;
                        windowWidth = m_window.getWindowSize().x;
                        windowHeight = m_window.getWindowSize().y;
                        bool horizontal = true;
                        bool finalPass = false;
                        glActiveTexture(GL_TEXTURE6);
                        glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
                        glActiveTexture(GL_TEXTURE7);
                        glBindTexture(GL_TEXTURE_2D, postProcessingColorTexture1);
                        glActiveTexture(GL_TEXTURE8);
                        glBindTexture(GL_TEXTURE_2D, postProcessingColorTexture2);
                        if (useBloom){
                            for (int i = 0; i < bloomPasses; i++) {
                                
                                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postProcessingColorTexture2, 0);
                                m_bloomShader.bind();
                                glUniform1i(m_bloomShader.getUniformLocation("horizontal"), horizontal);
                                glUniform1i(m_bloomShader.getUniformLocation("sceneTexture"), 6);
                                glUniform1i(m_bloomShader.getUniformLocation("bloomTexture"), 7);
                                glUniform1i(m_bloomShader.getUniformLocation("width"), windowWidth);
                                glUniform1i(m_bloomShader.getUniformLocation("height"), windowHeight);
                                glUniform1i(m_bloomShader.getUniformLocation("bloomRadius"), bloomRadius);
                                glUniform1f(m_bloomShader.getUniformLocation("bloomIntensity"), bloomIntensity);
                                glUniform1i(m_bloomShader.getUniformLocation("finalPass"), finalPass);
                                renderToTexture();
                                horizontal = !horizontal;
                                if (i == bloomPasses - 1) {
                                    finalPass = true;
                                }
                                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postProcessingColorTexture1, 0);
                                m_bloomShader.bind();

                                glUniform1i(m_bloomShader.getUniformLocation("horizontal"), horizontal);
                                glUniform1i(m_bloomShader.getUniformLocation("sceneTexture"), 6);                  
                                glUniform1i(m_bloomShader.getUniformLocation("bloomTexture"), 8);
                                glUniform1i(m_bloomShader.getUniformLocation("width"), windowWidth);
                                glUniform1i(m_bloomShader.getUniformLocation("height"), windowHeight);
                                glUniform1i(m_bloomShader.getUniformLocation("bloomRadius"), bloomRadius);
                                glUniform1f(m_bloomShader.getUniformLocation("bloomIntensity"), bloomIntensity);
                                glUniform1i(m_bloomShader.getUniformLocation("finalPass"), finalPass);
							    renderToTexture();
                                }
                        }
                        if (useDoF) {
                            glActiveTexture(GL_TEXTURE9);
                            glBindTexture(GL_TEXTURE_2D, sceneDepthTexture);
                            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postProcessingColorTexture2, 0);
                            m_dofShader.bind();
                            glUniform1i(m_dofShader.getUniformLocation("horizontal"), true);
							if (useBloom) glUniform1i(m_dofShader.getUniformLocation("sceneTexture"), 7);
							else glUniform1i(m_dofShader.getUniformLocation("sceneTexture"), 6);
                            glUniform1i(m_dofShader.getUniformLocation("depthTexture"), 9);
                            glUniform1i(m_dofShader.getUniformLocation("width"), windowWidth);
                            glUniform1i(m_dofShader.getUniformLocation("height"), windowHeight);
                            glUniform1f(m_dofShader.getUniformLocation("focalLength"), focalLength);
                            glUniform1f(m_dofShader.getUniformLocation("focalRange"), focalRange);
                            renderToTexture();
                            m_dofShader.bind();
                            glUniform1i(m_dofShader.getUniformLocation("horizontal"), false);
                            glUniform1i(m_dofShader.getUniformLocation("sceneTexture"), 8);
                            glUniform1i(m_dofShader.getUniformLocation("depthTexture"), 9);
                            glUniform1i(m_dofShader.getUniformLocation("width"), windowWidth);
                            glUniform1i(m_dofShader.getUniformLocation("height"), windowHeight);
                            glUniform1f(m_dofShader.getUniformLocation("focalLength"), focalLength);
                            glUniform1f(m_dofShader.getUniformLocation("focalRange"), focalRange);
                        }

                        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind back to default framebuffer
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear screen
                        renderToTexture();
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
                        glUniform4f(m_solarShader.getUniformLocation("theColor"), sun_color.x, sun_color.y, sun_color.z, 1.0f);
                        glUniform3f(m_solarShader.getUniformLocation("camPos"), cameraPos.x, cameraPos.y, cameraPos.z);
                        mesh.draw_no_mat(m_solarShader);
                        for (int i = 0; i < planets.size(); i++) {
                            Planet p = planets[i];
                            glm::mat4 p_model = p.GetModel(sun_model, solar_system_ts, i, sun_size, sun_pos);
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
                        // std::cout << "Camera Pos: " << cameraPos.x << "," << cameraPos.z << std::endl;
                        Coord new_coord = get_tile_coord();
                        // std::cout << "Current Coord: " << current_coord.x << "," << current_coord.y << std::endl;
                        // std::cout << "__________________________________" << std::endl;
                        if (new_coord.x != current_coord.x || new_coord.y != current_coord.y) {
                            std::random_device rd;
                            std::mt19937 gen(rd());
                            std::vector<std::vector<TerrainTile>> temp = terrain;
                            //Move right
                            if(new_coord.x < current_coord.x) {
                                for(int j = 0; j < 2 * terrain_height + 1; j++) {
                                    for(int i = 0; i < 2 * terrain_width + 1; i++) {
                                        if(i < 2 * terrain_width){
                                            temp[j][i] = terrain[j][i + 1];
                                        }else{
                                            int l = temp[j][i].tile.right;
                                            int d = (j > 0) ? temp[j - 1][i].tile.up : -1;
                                            int r = -1;
                                            int u = -1;
                                            std::vector<int> possible = get_possible_tiles(u, r, d, l);
                                            std::uniform_int_distribution<int> dis(0, possible.size() - 1);
                                            int index = possible[dis(gen)];
                                            u = full_tileset[index].up;
                                            r = full_tileset[index].right;
                                            d = full_tileset[index].down;
                                            l = full_tileset[index].left;

                                            TerrainTile t{index, Coord{temp[j][i-1].coord.x - 1, temp[j][i].coord.y}, Tile{u,r,d,l}}; 
                                            temp[j][i] = t;
                                        }
                                    }
                                }
                            }    
                            //Move left
                            else if(new_coord.x > current_coord.x) {
                                for(int j = 0; j < 2 * terrain_height + 1; j++) {
                                    for(int i = 2 * terrain_width; i >= 0; i--) {
                                        if(i > 0){
                                            temp[j][i] = terrain[j][i - 1];
                                        }else{
                                            int u = -1;
                                            int r = temp[j][i].tile.left;
                                            int d = (j > 0) ? temp[j - 1][i].tile.up : -1;
                                            int l = -1;
                                            std::vector<int> possible = get_possible_tiles(u, r, d, l);
                                            std::uniform_int_distribution<int> dis(0, possible.size() - 1);
                                            int index = possible[dis(gen)];
                                            u = full_tileset[index].up;
                                            r = full_tileset[index].right;
                                            d = full_tileset[index].down;                                            
                                            l = full_tileset[index].left;
                                            TerrainTile t{index, Coord{temp[j][i+1].coord.x + 1, temp[j][i].coord.y}, Tile{u,r,d,l}}; 
                                            temp[j][i] = t;
                                        }
                                    }
                                }
                            }
                            //Move Down
                            else if(new_coord.y < current_coord.y) {
                                for(int j = 2 * terrain_height; j >= 0; j--) {
                                    for(int i = 0; i < 2 * terrain_width + 1; i++) {
                                        if(j > 0){
                                            temp[j][i] = terrain[j - 1][i];
                                        }else{
                                            int u = temp[j][i].tile.down;
                                            int r = -1;
                                            int d = -1;
                                            int l = (i > 0) ? temp[j][i - 1].tile.left : -1;
                                            std::vector<int> possible = get_possible_tiles(u, r, d, l);
                                            std::uniform_int_distribution<int> dis(0, possible.size() - 1);
                                            int index = possible[dis(gen)];
                                            u = full_tileset[index].up;
                                            r = full_tileset[index].right;                                            
                                            d = full_tileset[index].down;
                                            l = full_tileset[index].left;
                                            TerrainTile t{index, Coord{temp[j][i].coord.x, temp[j + 1][i].coord.y - 1}, Tile{u,r,d,l}};
                                            temp[j][i] = t;
                                        }
                                    }
                                }
                            }else{
                                for(int j = 0; j < 2 * terrain_height + 1; j++) {
                                    for(int i = 0; i < 2 * terrain_width + 1; i++) {
                                        if(j < 2 * terrain_height){
                                            temp[j][i] = terrain[j + 1][i];
                                        }else{
                                            int u = -1;
                                            int r = -1;
                                            int d =  temp[j][i].tile.up;
                                            int l = (i > 0) ? temp[j][i - 1].tile.left : -1;
                                            std::vector<int> possible = get_possible_tiles(u, r, d, l);
                                            std::uniform_int_distribution<int> dis(0, possible.size() - 1);
                                            int index = possible[dis(gen)];
                                            u = full_tileset[index].up;
                                            r = full_tileset[index].right;
                                            d = full_tileset[index].down;
                                            l = full_tileset[index].left;
                                            TerrainTile t{index, Coord{temp[j][i].coord.x, temp[j - 1][i].coord.y + 1}, Tile{u,r,d,l}};
                                            temp[j][i] = t;
                                        }
                                    }
                                }
                            }
                            terrain = temp;
                        }
                        
                        current_coord = new_coord;
                        for(int j = 0; j < terrain_height; j++) {
                            for(int i = 0; i < terrain_width; i++) {
                                int k = terrain[j][i].id;
                                int new_x = terrain[j][i].coord.x - i ;
                                int new_z = terrain[j][i].coord.y + j ;
                                glm::vec3 newPos =  glm::vec3(new_x,0.0f,new_z);
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
                {

                    m_solarShader.bind();
                    std::string tree = grammars[selected_grammar].steps[grammar_step];
                    std::vector<glm::mat4> saved = {};
                    glm::vec3 translation = glm::vec3(0.0f, 2.0f, 0.0f);
                    glm::vec3 branch_inclination_axis = glm::vec3(0.0f, 0.0f, 1.0f);
                    glm::vec3 branch_rotation_axis = glm::vec3(0.0f, 1.0f, 0.0f);
                    glm::mat4 branch_model = glm::mat4(1.0f);
                    glUniform3f(m_solarShader.getUniformLocation("camPos"), cameraPos.x, cameraPos.y, cameraPos.z);
                    glUniform4f(m_solarShader.getUniformLocation("theColor"), 1.0f, 1.0f, 0.0f, 1.0f);
                    for (int i = 0; i < tree.length(); i++) {
                        char t = tree[i];
                        switch (t) {
                        case 'F':
                            for (GPUMesh& m : branch_mesh) {
                                glUniformMatrix4fv(m_solarShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix * m_viewMatrix * branch_model));
                                glUniformMatrix3fv(m_solarShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(glm::inverseTranspose(glm::mat3(branch_model))));
                                m.draw_no_mat(m_solarShader);
                                branch_model = glm::translate(branch_model, translation);
                            }
                            break;
                        case '+':
                            branch_model = glm::rotate(branch_model, glm::radians(branch_inclination), branch_inclination_axis);
                            break;
                        case '-':
                            branch_model = glm::rotate(branch_model, -glm::radians(branch_inclination), branch_inclination_axis);
                            break;
                        case '*':
                            branch_model = glm::rotate(branch_model, glm::radians(branch_rotation), branch_rotation_axis);
                            break;
                        case '/':
                            branch_model = glm::rotate(branch_model, -glm::radians(branch_rotation), branch_rotation_axis);
                            break;
                        case '[':
                            saved.push_back(branch_model);
                            break;
                        case ']':
                            branch_model = saved.back();
                            saved.pop_back();
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                }
                case 4:
                    waterShader.bind();
                    glm::mat4 water_model = glm::mat4(1.0f);
                    glm::mat4 mvp_wtr = m_projectionMatrix * m_viewMatrix * water_model;
                    glm::mat3 nModel_wtr = glm::inverseTranspose(glm::mat3(water_model));
                    for (GPUMesh& m : water_mesh) {
                        glUniformMatrix4fv(waterShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvp_wtr));
                        glUniformMatrix3fv(waterShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(nModel_wtr));
                        glUniform3f(waterShader.getUniformLocation("camPos"), cameraPos.x, cameraPos.y, cameraPos.z);
                        glUniform4f(waterShader.getUniformLocation("theColor"), 0.0f, 0.0f, 1.0f, 1.0f);
                        glUniform1f(waterShader.getUniformLocation("ts"), water_ts);
                        glUniform2f(waterShader.getUniformLocation("wave_center"),wave_center.x,wave_center.y);
                        glUniform1f(waterShader.getUniformLocation("wave_speed"),wave_speed);
                        glUniform1f(waterShader.getUniformLocation("lambda"),lambda);
                        glUniform1f(waterShader.getUniformLocation("phi"),phi);
                        glUniform1f(waterShader.getUniformLocation("dampening"),dampening);
                        m.draw_no_mat(waterShader);
                    }
                    if(water_enabled){
                        //water_ts += 0.1f;
                        water_ts += deltaTime;
                        if(water_ts > max_wave_time){
                            water_ts = -1.0f;
                            water_enabled = false;
                        }
                    }
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

    bool usePbr{ false };
    Shader m_pbrShader;
    Shader advancedPbrShader;
    Shader waterShader;

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


    float cam_fov = 80.0f;
    float cam_near_plane = 0.1f;
    float cam_far_plane = 100.0f;
    bool ortho = false;

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_projectionMatrix = glm::perspective(glm::radians(cam_fov), 1.0f, cam_near_plane, cam_far_plane);
    glm::mat4 m_viewMatrix = glm::lookAt(glm::vec3(-1, 1, -1), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 m_modelMatrix{ 1.0f };

    //Animation variables
    Shader m_animatedTextureShader;
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
	bool useParticles{ true };
    //glm::mat4 particleModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.5f, 0.0f));

    //Post processing variables for bloom and DoF
    Shader m_bloomShader;
    Shader m_dofShader;
	bool useBloom{ false };
	bool useDoF{ false };
    int bloomPasses{ 5 };
    int bloomRadius{ 5 };
    float bloomIntensity{ 1.0f };
	float bloomThreshold{ 1.0f };

	float focalLength{ 2.0f };
    float focalRange{ 1.0f };
    GLuint postProcessingVAO, postProcessingVBO;
	GLuint sceneBuffer; //original output
	GLuint postProcessingBuffer; //extracted intense regions for bloom
    GLuint sceneColorTexture; //resulting extracted scene colors
    GLuint postProcessingColorTexture1; //ping pong textures
    GLuint postProcessingColorTexture2;
	GLuint sceneDepthTexture; //depth of scene for DoF

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
    bool showLightOptions { true };


  

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
    std::vector<std::vector<TerrainTile>> terrain;
    int terrain_width, terrain_height, t_terrain_width, t_terrain_height;
    Coord current_coord;
    float terrain_epsilon = 0.02f;


    //L-systems
    std::vector<GPUMesh> branch_mesh;
    std::vector<Grammar> grammars;
    int selected_grammar = 0;
    int grammar_step = 0;
    float branch_inclination = 22.5f;
    float branch_rotation = 120.0f;
    bool random_inclination = false;
    bool random_rotation = false;


    //Water

    std::vector<GPUMesh> water_mesh;
    float water_ts = 0.0f;
    bool water_enabled = false;
    float max_wave_time = 10.0f;
    glm::vec2 wave_center = glm::vec2(0.0f, 0.0f);
    float wave_speed = 1.0f;
    float lambda = 0.9f;
    float phi = 10.0f;
    float dampening = 2.0f;
    glm::vec2 t_wave_center = glm::vec2(0.0f, 0.0f);
    float t_wave_speed = 1.0f;
    float t_lambda = 0.9f;
    float t_phi = 10.0f;
    float t_dampening = 2.0f;
};

int main()
{
    Application app;
    app.update();

    return 0;
}
