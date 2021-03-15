#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <util/filesystem.h>
#include <util/shader.h>
#include <util/camera.h>
#include <util/model.h>
#include <util/window.h>
#include <util/assets.h>

#include <iostream>
#include <functional>

void GLAPIENTRY
MessageCallback(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
        type, severity, message);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void renderQuad(void);
typedef std::function<void(void)> RenderFunc;

// settings
int SCR_WIDTH = 1024;
int SCR_HEIGHT = 1024;
float heightScale = 0.1;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
// input
bool firstMouse = true;
bool leftButtonDown = false;
bool middleButtonDown = false;
bool rightButtonDown = false;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// --- ASSETS ---
// textures
// for this tutorial diffuse normal and height maps are used
Assets textures = {
        {       "bricks", //group
            {   //{ TEX_FLIP, true }, // this causes textures to be flipped in y
                { "diffuse" , "../resources/textures/bricks2.jpg" },
                { "normal"  , "../resources/textures/bricks2_normal.jpg"},
                { "height"  , "../resources/textures/bricks2_disp.jpg"}
            }
        }, {    "toybox", //group
            {   //{ TEX_FLIP, true }, // this causes textures to be flipped in y
                { "diffuse" , "../resources/textures/toy_box_diffuse.png" },
                { "normal"  , "../resources/textures/toy_box_normal.png"},
                { "height"  , "../resources/textures/toy_box_disp.png"}
            }
        }, {    "woodgate", //group
            {   //{ TEX_FLIP, true }, // this causes textures to be flipped in y
                { "diffuse" , "../resources/textures/wood_gate/basecolor.jpg" },
                { "normal"  , "../resources/textures/wood_gate/normal.jpg"},
                { "height"  , "../resources/textures/wood_gate/height.png"}
            }
        }, {    "pebbles", //group
            {   //{ TEX_FLIP, true }, // this causes textures to be flipped in y
                { "diffuse" , "../resources/textures/stones/base.jpg" },
                { "normal"  , "../resources/textures/stones/normal.jpg"},
                { "height"  , "../resources/textures/stones/height.png"}
            }
        }, {    "debug", //group
            {   //{ TEX_FLIP, true }, // this causes textures to be flipped in y
                { "diffuse" , "../resources/textures/color_grid.jpg" },
                { "normal"  , "../resources/textures/toy_box_normal.png"},
                { "height"  , "../resources/textures/toy_box_disp.png"}
            }
        }, {    "metal", //group
            {   { TEX_FLIP, true }, // this causes textures to be flipped in y
                { "diffuse" , "../resources/textures/metal/base.jpg" },
                { "normal"  , "../resources/textures/metal/normal.jpg"},
                { "height"  , "../resources/textures/metal/height.png"}
            }
        }
    };
AssetManager texMan(textures); // handles texture loading
//
// primitives
AssetManager objMan({
        {   "quad", //group
            {   { "func", RenderFunc(renderQuad) }, } // adding fucntion works!
        },{
            "sphere",
            {  { "model", "../resources/simple/sphere.obj" }, } // this is how you can load models!
        }
    });



const char* APP_NAME = "normal/parallax mapping";
int main()
{
    // glfw: initialize and configure
    // ------------------------------
    InitWindowAndGUI(SCR_WIDTH, SCR_HEIGHT, APP_NAME );


    // OpenGL is initialized now, so we can load assets and so on ...
    // ------------------------------
    RenderFunc renderPrimitive = objMan.GetAsset<RenderFunc>("quad","func");
    auto renderModel = objMan.GetAsset<Model>("sphere", "model");
    unsigned int diffuseMap, normalMap, heightMap; 
    bool animateLight = true;
    float lightAngle = 0.0f;
    bool drawWireFrame = false;
    glm::vec4 bgColor = { 0.1,0.1,0.1,1.0 };
    texMan.SetActiveGroup("toybox");


    //glEnable(GL_DEBUG_OUTPUT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //glDebugMessageCallback(MessageCallback, 0);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    SetFramebufferSizeCallback(framebuffer_size_callback);
    SetCursorPosCallback(mouse_callback);
    SetScrollCallback(scroll_callback);
    SetMouseButtonCallback(mouse_button_callback);
    SetFramebufferSizeCallback([](GLFWwindow *window, int w, int h) {
        SCR_WIDTH = w; SCR_HEIGHT = h;
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        });
   
    // build and compile shaders
    // -------------------------
    std::vector<Shader> shaders;
    shaders.push_back(Shader("../src/normal_parallax/normal_mapping.vs.glsl", "../src/normal_parallax/simple_phong.fs.glsl")); // standard Blinn Phong shading
    shaders.push_back(Shader("../src/normal_parallax/normal_mapping.vs.glsl", "../src/normal_parallax/normal_mapping.fs.glsl")); // normal mapping
    shaders.push_back(Shader("../src/normal_parallax/normal_mapping.vs.glsl", "../src/normal_parallax/parallax_mapping.fs.glsl")); // (simple) parallax mapping
    shaders.push_back(Shader("../src/normal_parallax/normal_mapping.vs.glsl", "../src/normal_parallax/parallax_occlusion_mapping.fs.glsl")); // parallax occlusion mapping

    // get texture ids (Note: AssetManager is taking care of loading them)
    // -------------
    diffuseMap = texMan.GetActiveAsset<Tex>( "diffuse");
    normalMap  = texMan.GetActiveAsset<Tex>( "normal");
    heightMap  = texMan.GetActiveAsset<Tex>( "height");

    std::vector<glm::vec3> positions;
    positions.push_back(glm::vec3(-1,  1, 0));
    positions.push_back(glm::vec3( 1,  1, 0));
    positions.push_back(glm::vec3(-1, -1, 0));
    positions.push_back(glm::vec3( 1, -1, 0));
    assert(positions.size() == shaders.size());
    


    // shader configuration
    // --------------------
    for (auto &shader : shaders)
    {
        shader.use();
        shader.setInt("diffuseMap", 0);
        shader.setInt("normalMap", 1);
        shader.setInt("depthMap", 2);
    }

    // lighting info
    // -------------
    glm::vec3 origlightPos(0.5f, 4.0f, 5.0f);
    auto lightPos = origlightPos;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // matrices 
        glm::mat4 model, projection, view; // used and recomputed every frame!

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
                ImGui::SliderFloat("height scale", &heightScale, 0.0f, 1.0f);   // Edit 1 float using a slider from 0.0f to 1.0f
                ImGui::ColorEdit3("clear color", (float*)&bgColor.x); // Edit 3 floats representing a color

                // Combobox with Texture options:
                auto textures = texMan.GetGroups();
                int item_current = texMan.GetActiveGroupId();
                std::vector<const char*> strings;
                for (int i = 0; i < textures.size(); ++i)
                    strings.push_back(textures[i].c_str());
                ImGui::Combo("texture", &item_current, strings.data(), textures.size() );
                if (texMan.GetActiveGroupId() != item_current)
                {
                    texMan.SetActiveGroup(item_current);
                    diffuseMap = texMan.GetActiveAsset<Tex>("diffuse");
                    normalMap = texMan.GetActiveAsset<Tex>("normal");
                    heightMap = texMan.GetActiveAsset<Tex>("height");
                }

                // a Button to reload the shader (so you don't need to recompile the cpp all the time)
                if (ImGui::Button("reload shaders")) {
                    for (auto& shader : shaders)
                    {
                        shader.reload();
                        shader.use();
                        shader.setInt("diffuseMap", 0);
                        shader.setInt("normalMap", 1);
                        shader.setInt("depthMap", 2);
                    }
                }

                ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
                ImGui::End();
            }
        }



        // render
        // ------
        if (gui) ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
        

        // configure global opengl state
        // -----------------------------
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        // animate light
        // -------------
        if (animateLight)
        {
            lightAngle += deltaTime;
            auto rotate = glm::rotate(glm::mat4(1.0f), lightAngle, glm::vec3(0, 0, 1));
            lightPos = rotate * glm::vec4(origlightPos,1.0f);
        }

        // configure view/projection matrices
        projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        view = camera.GetViewMatrix();
        float countShaders = 0.0f;
        for (auto& shader : shaders)
        {
            shader.use();
            shader.setMat4("projection", projection);
            shader.setMat4("view", view);
            // render parallax-mapped quad
            model = glm::mat4(1.0f);
            model = glm::translate(model, positions.at(countShaders));
            //model = glm::rotate(model, glm::radians((float)glfwGetTime() * -10.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0))); // rotate the quad to show parallax mapping from multiple directions
            shader.setMat4("model", model);
            shader.setVec3("viewPos", camera.Position);
            shader.setVec3("lightPos", lightPos);
            shader.setFloat("heightScale", heightScale); // adjust with Q and E keys or the widget
            //std::cout << heightScale << std::endl;
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseMap);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, normalMap);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, heightMap);
            renderPrimitive();

            model = glm::translate(model,positions.at(countShaders)*2.0f);
            shader.setMat4("model", model);
            renderModel.Draw(shader);

            countShaders++;
        }

        // render light source (simply renders a small sphere for debugging/visualization)
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.1f));
        shaders.at(0).use();
        shaders.at(0).setMat4("model", model);
        renderModel.Draw(shaders.at(0));

        // do some things needed for window management. e.g., swap buffers, draw GUI, poll events ....
        if (gui) ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    DestroyWindow();
    return 0;
}

std::pair<glm::vec3, glm::vec3> computeTangentBiTangent(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2, 
    const glm::vec2 &uv0, const glm::vec2 &uv1, const glm::vec2 &uv2 )
{
    glm::vec3 q1 = p1 - p0;
    glm::vec3 q2 = p2 - p0;
    glm::vec2 st1 = uv1 - uv0;
    glm::vec2 st2 = uv2 - uv0;


    float r = 1.0f / (uv1.x * uv2.y - uv1.y * uv2.x);
    assert(!isnan(r) && !isinf(r));
    glm::vec3 outT = (q1 * uv2.y - q2 * uv1.y) * r;
    glm::vec3 outB = (q2 * uv1.x - q1 * uv2.x) * r;
    

    outT = glm::normalize(outT);
    outB = glm::normalize(outB);

    return { outT, outB };
}

// renders a 1x1 quad in NDC with manually calculated tangent vectors
// ------------------------------------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad(void)
{
    if (quadVAO == 0)
    {
        // positions
        glm::vec3 pos1(-1.0f,  1.0f, 0.0f);
        glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
        glm::vec3 pos3( 1.0f, -1.0f, 0.0f);
        glm::vec3 pos4( 1.0f,  1.0f, 0.0f);
        // texture coordinates
        glm::vec2 uv1(0.0f, 0.0f);
        glm::vec2 uv2(0.0f, 1.0f);
        glm::vec2 uv3(1.0f, 1.0f);
        glm::vec2 uv4(1.0f, 0.0f);
        // normal vector
        glm::vec3 nm(0.0f, 0.0f, 1.0f);

        // calculate tangent/bitangent vectors of both triangles
        glm::vec3 tangent1, bitangent1;
        glm::vec3 tangent2, bitangent2;
        // triangle 1
        // ----------
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        glm::mat2x3 q1q2{ edge1, edge2 };
        glm::mat2x2 st12{ deltaUV1, deltaUV2 };

        glm::mat2x2 ist12 = glm::inverse(st12);

        glm::mat2x3 res =  ist12 * glm::transpose(q1q2);
        auto T = glm::normalize(res[0]);
        auto B = glm::normalize(res[1]);
        //auto TB = ist12 * q1q2;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent1 = glm::normalize(tangent1);

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent1 = glm::normalize(bitangent1);

        // test function
        auto TB = computeTangentBiTangent(pos1, pos2, pos3, uv1, uv2, uv3);
        glm::vec3 tT{ TB.first }, tB{ TB.second };
        assert(tT == tangent1 & tB == bitangent1);

        // triangle 2
        // ----------
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent2 = glm::normalize(tangent2);


        bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent2 = glm::normalize(bitangent2);

        TB = computeTangentBiTangent(pos1, pos3, pos4, uv1, uv3, uv4);
        tT = TB.first; tB = TB.second;
        assert(tT == tangent2 & tB == bitangent2);

        float quadVertices[] = {
            // positions            // normal         // texcoords  // tangent                          // bitangent
            pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
            pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
            pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

            pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
            pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
            pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
        };
        // configure plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
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

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        if (heightScale > 0.0f)
            heightScale -= 0.0005f;
        else
            heightScale = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        if (heightScale < 1.0f)
            heightScale += 0.0005f;
        else
            heightScale = 1.0f;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
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

    if (leftButtonDown) // move camera with left button
        camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever any mouse button is pressed, this callback is called
// ----------------------------------------------------------------------
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT)   leftButtonDown = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) middleButtonDown = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)  rightButtonDown = true;
    }
    else if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_LEFT)   leftButtonDown = false;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) middleButtonDown = false;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)  rightButtonDown = false;
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}
