#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <algorithm>
#include <iostream>
#include <sstream> 
#include <iomanip>
#include <optional>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include <util/filesystem.h>
#include <util/shader.h>
#include <util/camera.h>
#include <util/model.h>
#include <util/window.h>
#include <util/assets.h>



using namespace std;
//using namespace nanogui;

// --- ASSETS ---
// models and textures used in this tutorial
AssetManager assets ({
         {       "sphere", //group
            {   { "model"   , "../resources/simple/sphere.obj" },
                { "transformation"   , glm::scale(glm::mat4(1.0f), glm::vec3(1.0))  },
                //{ TEX_FLIP, true }, // if true, causes textures to be flipped in y
                { "albedo" , "../resources/textures/rusted_iron/albedo.png" },
                { "normal"  , "../resources/textures/rusted_iron/normal.png"},
                { "metallness"  , "../resources/textures/rusted_iron/metallic.png"},
                { "roughness"  , "../resources/textures/rusted_iron/roughness.png"},
                { "ao"  , "../resources/textures/rusted_iron/ao.png"}
            }
        }, {       "helmet", //group
            {   { "model"   , "../resources/objects/helmet/helmet.obj" },
                { "transformation"   , glm::scale( glm::mat4(1.0f), glm::vec3( 1.0 ) )  },
                //{ TEX_FLIP, true }, // if true, causes textures to be flipped in y
                { "albedo" , "../resources/objects/helmet/albedo.jpg" },
                { "normal"  , "../resources/objects/helmet/normal.jpg"},
                { "metallness"  , "../resources/objects/helmet/metall.jpg"},
                { "roughness"  , "../resources/objects/helmet/roughness.jpg"},
                { "ao"  , "../resources/objects/helmet/AO.jpg"}
            }
        },{       "cerberus", //group
            {   { "model"   , "../resources/objects/cerberus/Cerberus_LP.FBX"  },
                { "transformation"   , glm::scale(glm::mat4(1.0f), glm::vec3(0.01))  },
                //{ TEX_FLIP, true }, // if true, causes textures to be flipped in y
                { "albedo" , "../resources/objects/cerberus/Textures/Cerberus_A.tga" },
                { "normal"  , "../resources/objects/cerberus/Textures/Cerberus_N.tga"},
                { "metallness"  , "../resources/objects/cerberus/Textures/Cerberus_M.tga"},
                { "roughness"  , "../resources/objects/cerberus/Textures/Cerberus_R.tga"},
                { "ao"  , "../resources/textures/pbr/rusted_iron/ao.png"}
            }
        }, {    "backpack", //group
            {   { "model"   , "../resources/objects/backpack/backpack.obj"  },
                { "transformation"   , glm::scale(glm::mat4(1.0f), glm::vec3(1.0))  },
                { TEX_FLIP, true }, // this causes textures to be flipped in y
                { "albedo" , "../resources/objects/backpack/diffuse.jpg" },
                { "normal"  , "../resources/objects/backpack/normal.png"},
                { "metallness"  , "../resources/objects/backpack/specular.jpg"},
                { "roughness"  , "../resources/objects/backpack/roughness.jpg"},
                { "ao"  , "../resources/objects/backpack/ao.jpg"}
            }
        }});


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void renderSphere();

// settings
int SCR_WIDTH = 1280;
int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
bool firstMouse = true;
bool leftButtonDown = false;
bool middleButtonDown = false;
bool rightButtonDown = false;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float fps = 123.45f;




const char* APP_NAME = "PBR";
int main()
{
    // controllable settings
    float gamma = 2.2f;
    glm::vec3 albedo{ 1.0, 0.0, 0.0 };
    float roughness = 0.2f;
    float metallic = 0.0f;
    bool useTextures = false;

    // glfw: initialize and configure
    // ------------------------------
    InitWindowAndGUI(SCR_WIDTH, SCR_HEIGHT, APP_NAME);


    // OpenGL is initialized now, so we can use OpenGL functions (glFoo ...)
    // ------------------------------
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //glDebugMessageCallback(MessageCallback, 0);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    SetCursorPosCallback(mouse_callback);
    SetMouseButtonCallback(mouse_button_callback);  
    SetScrollCallback(scroll_callback);
    SetFramebufferSizeCallback(framebuffer_size_callback);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);


    // loaded model
    // -------------------------
    assets.SetActiveGroup("sphere");
    Model loadedModel = assets.GetActiveAsset<Model>("model"); // new Model(FileSystem::getPath("resources/objects/cerberus/Cerberus_LP.FBX").c_str());
    glm::mat4 modelTransformation = assets.GetActiveAsset<glm::mat4>("transformation");
    //loadedModel = backpack.getModel();

    // load PBR material textures
    // --------------------------
    unsigned int albedoMap = assets.GetActiveAsset<Tex>("albedo"); //loadTexture(FileSystem::getPath("resources/objects/cerberus/Textures/Cerberus_A.tga").c_str());
    unsigned int normalMap = assets.GetActiveAsset<Tex>("normal"); //     = loadTexture(FileSystem::getPath("resources/objects/cerberus/Textures/Cerberus_N.tga").c_str());
    unsigned int metallicMap = assets.GetActiveAsset<Tex>("metallness"); //   = loadTexture(FileSystem::getPath("resources/objects/cerberus/Textures/Cerberus_M.tga").c_str());
    unsigned int roughnessMap = assets.GetActiveAsset<Tex>("roughness"); //  = loadTexture(FileSystem::getPath("resources/objects/cerberus/Textures/Cerberus_R.tga").c_str());
    unsigned int aoMap = assets.GetActiveAsset<Tex>("ao"); //        = loadTexture(FileSystem::getPath("resources/textures/pbr/rusted_iron/ao.png").c_str());



    // build and compile shaders
    // -------------------------
    Shader shader("../src/pbr/pbr.vs.glsl", "../src/pbr/pbr.fs.glsl");

    shader.use();
    shader.setInt("albedoMap", 0);
    shader.setInt("normalMap", 1);
    shader.setInt("metallicMap", 2);
    shader.setInt("roughnessMap", 3);
    shader.setInt("aoMap", 4);





    // lights
    // ------
    glm::vec3 lightPositions[] = {
       glm::vec3(-10.0f,  10.0f, 10.0f),
       glm::vec3( 10.0f,  10.0f, 20.0f),
       glm::vec3(-10.0f, -10.0f, 30.0f),
       glm::vec3( 10.0f, -10.0f, 40.0f),
    };
    glm::vec3 lightColors[] = {
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f)
    };
    int nrRows = 7;
    int nrColumns = 7;
    float spacing = 2.5;

    // initialize static shader uniforms before rendering
    // --------------------------------------------------
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    shader.use();
    shader.setMat4("projection", projection);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        glEnable(GL_DEPTH_TEST);

        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // input
        // -----
        processInput(window);
        if (gui) {
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
            {
                ImGui::Begin(APP_NAME);
                ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
                ImGui::SliderFloat("gamma", &gamma, 0.1f, 5.0f);   // Edit 1 float using a slider from 0.0f to 1.0f
                ImGui::Checkbox("use textures", &useTextures);
                if (!useTextures) {
                    ImGui::ColorEdit3("albedo", (float*)&albedo.x); // Edit 3 floats representing a color
                    ImGui::SliderFloat("roughness", &roughness, 0.05f, 1.0f);
                    ImGui::SliderFloat("matallic", &metallic, 0, 1);
                }
             
                // Combobox with Texture/Model options:
                auto ass = assets.GetGroups();
                int item_current = assets.GetActiveGroupId();
                std::vector<const char*> strings;
                for (int i = 0; i < ass.size(); ++i)
                    strings.push_back(ass[i].c_str());
                ImGui::Combo("model", &item_current, strings.data(), ass.size());
                if (assets.GetActiveGroupId() != item_current)
                {
                    assets.SetActiveGroup(item_current);
                    // loaded model and (PBR) texutes
                    // -------------------------
                    loadedModel = assets.GetActiveAsset<Model>("model"); 
                    modelTransformation = assets.GetActiveAsset<glm::mat4>("transformation");
                    albedoMap = assets.GetActiveAsset<Tex>("albedo"); 
                    normalMap = assets.GetActiveAsset<Tex>("normal"); 
                    metallicMap = assets.GetActiveAsset<Tex>("metallness"); 
                    roughnessMap = assets.GetActiveAsset<Tex>("roughness"); 
                    aoMap = assets.GetActiveAsset<Tex>("ao"); 

                }
                // a Button to reload the shader (so you don't need to recompile the cpp all the time)
                if (ImGui::Button("reload shaders")) {
                    shader.reload();
                    shader.use();
                    shader.setInt("albedoMap", 0);
                    shader.setInt("normalMap", 1);
                    shader.setInt("metallicMap", 2);
                    shader.setInt("roughnessMap", 3);
                    shader.setInt("aoMap", 4);
                }

                ImGui::End();
            }
        }


        // render
        // ------
        if (gui) ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // configure global opengl state
        // -----------------------------
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        shader.use();
        projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        shader.setMat4("projection", projection);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("view", view);
        shader.setVec3("camPos", camera.Position);
        shader.setVec3("Albedo", albedo.r, albedo.g, albedo.b);
        shader.setFloat("AO", 1.0f);
        shader.setFloat("Metallic", metallic);
        shader.setFloat("Roughness", glm::clamp(roughness, 0.05f, 1.0f)); //  we clamp the roughness to 0.05 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off  on direct lighting.
        shader.setFloat("gamma", gamma);
        shader.setFloat("useTextures", useTextures ? 1.0f : 0.0f);

        if (useTextures)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, albedoMap);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, normalMap);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, metallicMap);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, roughnessMap);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, aoMap);
        }
        
        auto model = (glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f))) * modelTransformation;
        shader.setMat4("model", model);
        shader.setFloat("roughness", 0.05f);
        loadedModel.Draw(shader);

        // render light source (simply re-render sphere at light positions)
        // this looks a bit off as we use the same shader, but it'll make their positions obvious and 
        // keeps the codeprint small.
        for (unsigned int i = 0; i < sizeof(lightPositions) / sizeof(lightPositions[0]); ++i)
        {
            glm::vec3 newPos = lightPositions[i] + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
            newPos = lightPositions[i];
            shader.setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
            shader.setVec3("lightColors[" + std::to_string(i) + "]", lightColors[i]);

            model = glm::mat4(1.0f);
            model = glm::translate(model, newPos);
            model = glm::scale(model, glm::vec3(0.5f));
            shader.setMat4("model", model);
            renderSphere();
        }


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        if (gui) ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    SCR_WIDTH = width; SCR_HEIGHT = height;
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if(leftButtonDown)
        camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever any mouse button is pressed, this callback is called
// ----------------------------------------------------------------------
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT)   leftButtonDown      = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) middleButtonDown    = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)  rightButtonDown     = true;
    } else if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_LEFT)   leftButtonDown      = false;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) middleButtonDown    = false;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)  rightButtonDown     = false;
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

// renders (and builds at first invocation) a sphere
// -------------------------------------------------
unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359;
        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y       * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y       * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = indices.size();

        std::vector<float> data;
        for (std::size_t i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        float stride = (3 + 2 + 3) * sizeof(float);
        // make sure that this matches the layout in the vertex shader!
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

