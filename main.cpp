#include <iostream>
#define MINIAUDIO_IMPLEMENTATION
#include "libraries/miniaudio.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"
#include "camera.hpp"
#include "model.hpp"

Camera camera(glm::vec3(0.0f, 0.5f, 5.0f));
float lastX = 400, lastY = 300;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
// ... (mouse_callback remains same)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    float speedMult = 10.0f; 
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime * speedMult);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime * speedMult);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime * speedMult);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime * speedMult);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Model Viewer", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader ourShader("shader.vert", "shader.frag");
    Shader planetShader("planet.vert", "planet.frag");
    Shader skyShader("sky.vert", "sky.frag");
    Shader gridShader("grid.vert", "grid.frag");
    Shader enigmaShader("enigma.vert", "enigma.frag");
    Shader hudShader("hud.vert", "hud.frag");

    // HUD Setup
    float hudVertices[] = {
        0.55f, 0.96f,  0.0f, 0.0f, // Adjusted for better aspect ratio
        0.95f, 0.96f,  1.0f, 0.0f,
        0.95f, 0.88f,  1.0f, 1.0f,
        0.55f, 0.96f,  0.0f, 0.0f,
        0.95f, 0.88f,  1.0f, 1.0f,
        0.55f, 0.88f,  0.0f, 1.0f
    };
    unsigned int hudVAO, hudVBO;
    glGenVertexArrays(1, &hudVAO);
    glGenBuffers(1, &hudVBO);
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(hudVertices), hudVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));


    // Audio Setup
    ma_engine engine;
    ma_sound bgMusic;
    bool audioLoaded = false;
    
    if (ma_engine_init(NULL, &engine) == MA_SUCCESS) {
        // You can change this path to your audio file
        std::string audioPath = "/home/chiranjeet/Graphics/model_files/march-of-the-troopers-star-wars-style-cinematic-music-207056.mp3";
        if (ma_sound_init_from_file(&engine, audioPath.c_str(), 0, NULL, NULL, &bgMusic) == MA_SUCCESS) {
            ma_sound_set_looping(&bgMusic, MA_TRUE);
            ma_sound_start(&bgMusic);
            audioLoaded = true;
        }
    }

    // Load models
    Model ourModel("/home/chiranjeet/Graphics/model_files/ue4-storm-trooper-rigged-game-ready/source/Walking.fbx");
    Model planetModel("/home/chiranjeet/Graphics/model_files/wskrs-the-eyes-and-ears-of-seaquest/source/WSKRS.fbx");
    Model enigmaModel("/home/chiranjeet/Graphics/model_files/star-cruiser-x-enigma/scene.gltf");

    // Animation variables
    Assimp::Importer animationImporter;
    const aiScene* animationScene = animationImporter.ReadFile("/home/chiranjeet/Graphics/model_files/ue4-storm-trooper-rigged-game-ready/source/Walking.fbx", aiProcess_Triangulate);

    // Unified Uniform Grid (Vast and consistent)
    vector<float> gridVertices;
    int gridSize = 200;
    int gridSpacing = 5; // Uniform cell size
    for(int i = -gridSize; i <= gridSize; i += gridSpacing) {
        gridVertices.push_back((float)i); gridVertices.push_back(0.0f); gridVertices.push_back((float)-gridSize);
        gridVertices.push_back((float)i); gridVertices.push_back(0.0f); gridVertices.push_back((float)gridSize);
        gridVertices.push_back((float)-gridSize); gridVertices.push_back(0.0f); gridVertices.push_back((float)i);
        gridVertices.push_back((float)gridSize); gridVertices.push_back(0.0f); gridVertices.push_back((float)i);
    }

    unsigned int gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), &gridVertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // Skybox vertices
    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
    };
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // FPS Calculation
        static int frameCount = 0;
        static float lastTime = 0.0f;
        static int fps = 0;
        frameCount++;
        if (currentFrame - lastTime >= 1.0f) {
            fps = frameCount;
            frameCount = 0;
            lastTime = currentFrame;
        }

        // Auto-move camera with the army
        float moveForward = 2.0f * deltaTime;
        camera.position.z -= moveForward; // Assumes -Z is forward

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;
        glm::mat4 projection = glm::perspective(glm::radians(camera.zoom), aspect, 0.1f, 2000.0f); // Higher far plane
        glm::mat4 view = camera.GetViewMatrix();

        // Light Settings
        glm::vec3 lightPos(0.0f, 150.0f, camera.position.z - 200.0f);
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

        // 1. Draw Skybox (Procedural)
        glDisable(GL_BLEND);
        glDepthFunc(GL_LEQUAL);
        skyShader.use();
        
        glm::mat4 skyView = glm::mat4(glm::mat3(view));
        skyShader.setMat4("view", skyView); 
        skyShader.setMat4("projection", projection);
        
        // Procedural Sky Uniforms
        skyShader.setVec2("resolution", glm::vec2((float)SCR_WIDTH, (float)SCR_HEIGHT));
        skyShader.setMat4("inv_proj", glm::inverse(projection));
        skyShader.setMat4("inv_view", glm::inverse(skyView));
        
        // Dark space colors
        skyShader.setVec3("skyColorBottom", glm::vec3(0.005f, 0.005f, 0.01f));
        skyShader.setVec3("skyColorTop", glm::vec3(0.0f, 0.0f, 0.0f));
        
        // Sun direction logic
        glm::vec3 sunDir = glm::normalize(lightPos - camera.position);
        skyShader.setVec3("lightDirection", sunDir);
        skyShader.setFloat("time", currentFrame);

        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);

        planetShader.use();
        planetShader.setMat4("projection", projection);
        planetShader.setMat4("view", view);
        planetShader.setVec3("lightPos", lightPos);
        planetShader.setVec3("viewPos", camera.position);
        planetShader.setVec3("lightColor", lightColor);
        glm::mat4 model = glm::mat4(1.0f);
        // Position it relative to the camera to ensure it's never passed
        model = glm::translate(model, glm::vec3(-160.0f, 161.0f, camera.position.z - 400.0f)); 
        
        // Triple-Axis Periodic Rotation (Tumbling effect)
        model = glm::rotate(model, (float)sin(currentFrame * 0.7f) * 10.0f, glm::vec3(1.0f, 0.0f, 0.0f)); 
        model = glm::rotate(model, (float)sin(currentFrame * 0.5f) * 10.0f, glm::vec3(0.0f, 1.0f, 0.0f)); 
        model = glm::rotate(model, (float)sin(currentFrame * 0.3f) * 10.0f, glm::vec3(0.0f, 0.0f, 1.0f)); 
        
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f)); // Smaller scale
        planetShader.setMat4("model", model);
        planetModel.Draw(planetShader);

        // 3. Draw Star Cruiser Enigma (Opaque - Always in front)
        enigmaShader.use();
        enigmaShader.setMat4("projection", projection);
        enigmaShader.setMat4("view", view);
        enigmaShader.setVec3("lightPos", lightPos);
        enigmaShader.setVec3("viewPos", camera.position);
        enigmaShader.setVec3("lightColor", lightColor);
        
        // Hovering Effect (Vertical Oscillation)
        float hoverOffset = (float)sin(glfwGetTime() * 1.0f) * 5.0f;
        
        glm::mat4 enigmaM = glm::mat4(1.0f);
        // Position it far and high with hover offset
        enigmaM = glm::translate(enigmaM, glm::vec3(100.0f, 30.0f + hoverOffset, camera.position.z - 600.0f)); 
        enigmaM = glm::rotate(enigmaM, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        enigmaM = glm::rotate(enigmaM, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        enigmaM = glm::rotate(enigmaM, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        enigmaM = glm::rotate(enigmaM, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        enigmaM = glm::rotate(enigmaM, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        enigmaM = glm::scale(enigmaM, glm::vec3(1010.0f, 1010.0f, 1010.0f)); 
        enigmaShader.setMat4("model", enigmaM);
        enigmaModel.Draw(enigmaShader);

        // 4. Draw Green Wireframe Grid (Blending ON)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        gridShader.use();
        gridShader.setMat4("projection", projection);
        gridShader.setMat4("view", view);
        gridShader.setVec3("viewPos", camera.position);
        
        glm::mat4 gridModel = glm::mat4(1.0f);
        float snap = 5.0f; 
        float gridX = floor(camera.position.x / snap) * snap;
        float gridZ = floor(camera.position.z / snap) * snap;
        gridModel = glm::translate(gridModel, glm::vec3(gridX, 0.0f, gridZ)); 
        
        gridShader.setMat4("model", gridModel);
        glBindVertexArray(gridVAO);
        glDrawArrays(GL_LINES, 0, gridVertices.size() / 3);
        glDisable(GL_BLEND);

        // 4. Draw Troopers (Opaque)
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setVec3("lightPos", lightPos);
        ourShader.setVec3("viewPos", camera.position);
        ourShader.setVec3("lightColor", lightColor);

        if (animationScene && animationScene->mNumAnimations > 0)
        {
            vector<glm::mat4> transforms(100, glm::mat4(1.0f));
            ourModel.UpdateAnimation(currentFrame, animationScene, transforms);
            for (int i = 0; i < transforms.size(); i++)
                ourShader.setMat4("finalBonesMatrices[" + to_string(i) + "]", transforms[i]);
            ourShader.setBool("hasTexture", true);
        }

        // Render army of tiny troopers (Moving with the world)
        float worldOffset = currentFrame * 2.0f; // Matches camera auto-speed
        for (int x = -10; x <= 10; x++)
        {
            for (int z = -10; z <= 10; z++)
            {
                glm::mat4 trooperModel = glm::mat4(1.0f);
                // Translate relative to a moving base to keep up with camera
                trooperModel = glm::translate(trooperModel, glm::vec3((float)x * 2.0f, 0.0f, (float)z * 2.0f - worldOffset));
                trooperModel = glm::rotate(trooperModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); 
                trooperModel = glm::scale(trooperModel, glm::vec3(0.02f, 0.02f, 0.02f)); 
                ourShader.setMat4("model", trooperModel);
                ourModel.Draw(ourShader);
            }
        }

        // 5. Draw HUD (FPS Counter)
        glEnable(GL_BLEND);
        hudShader.use();
        hudShader.setInt("fps", fps);
        hudShader.setVec3("textColor", glm::vec3(0.0f, 1.0f, 0.0f)); // Bright green
        glBindVertexArray(hudVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisable(GL_BLEND);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    if (audioLoaded) {
        ma_sound_uninit(&bgMusic);
    }
    ma_engine_uninit(&engine);

    glfwTerminate();
    return 0;
}
