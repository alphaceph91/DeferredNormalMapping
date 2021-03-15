#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <util/filesystem.h>
#include <util/shader.h>
#include <util/camera.h>
#include <util/model.h>
#include <util/assets.h>
#include <util/window.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
//unsigned int loadTexture(const char *path, bool gammaCorrection);
void renderQuad();
void renderCube();

// settings
int SCR_WIDTH = 1024;
int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 1.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;
bool leftButtonDown = false;
bool middleButtonDown = false;
bool rightButtonDown = false;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


const char* APP_NAME = "postprocessing";
int main()
{
    // Settings and parameters to change
    glm::vec4 bgColor = { 0.1,0.1,0.1,1.0 };
    int postProcessingMode = 0;
    bool showWireframe = false;
    float kernelSize = 1.0;
    bool rotateModel = false;

    // glfw: initialize and configure
    // ------------------------------
    InitWindowAndGUI(SCR_WIDTH, SCR_HEIGHT, APP_NAME);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // avoid resizing the window!

    SetCursorPosCallback(mouse_callback);
    SetMouseButtonCallback(mouse_button_callback);
    SetScrollCallback(scroll_callback);
    //SetFramebufferSizeCallback(framebuffer_size_callback);

    
    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    // stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader modelShader("../src/postprocessing/model.vs.glsl", "../src/postprocessing/model.fs.glsl");
    Shader screenShader("../src/postprocessing/screenshader.vs.glsl", "../src/postprocessing/screenshader.fs.glsl");
    
    // advanced shaders below
    Shader filterShader("../src/postprocessing/screenshader.vs.glsl", "../src/postprocessing/filter.fs.glsl");
    Shader nightVisionShader("../src/postprocessing/screenshader.vs.glsl", "../src/postprocessing/nightvision.fs.glsl");

    std::map<std::string, Shader*> postProShader({ 
            { "00 none", &screenShader },
            { "01 invert color", new Shader("../src/postprocessing/screenshader.vs.glsl", "../src/postprocessing/invertcolor.fs.glsl") },
            { "02 vintage", new Shader("../src/postprocessing/screenshader.vs.glsl", "../src/postprocessing/vintage.fs.glsl") },
            { "03 night vision", &nightVisionShader},
            { "04 filter", &filterShader},
            { "05 sobel", new Shader("../src/postprocessing/screenshader.vs.glsl", "../src/postprocessing/sobel.fs.glsl") },
        });
    int numPostProcessingModes = postProShader.size();
    Shader* activeShader = &screenShader;


    // load models
    // -----------
    Model myModel(FileSystem::getPath("resources/objects/buddha2/buddha.obj"), true);

    // shader configuration
    // --------------------
    modelShader.use();
    modelShader.setInt("texture1", 0);

    activeShader->use();
    activeShader->setInt("screenTexture", 0);

    // framebuffer configuration
    // -------------------------
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // create a color attachment texture
    unsigned int textureColorbuffer;
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
    // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
    // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
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
                ImGui::ColorEdit3("clear color", (float*)&bgColor.x); // Edit 3 floats representing a color
                ImGui::Checkbox("rotate model", &rotateModel);

                //ImGui::SliderInt("postprocessing mode", &postProcessingMode, 0, numPostProcessingModes-1);
                // Combobox with options:
                {
                    std::vector<const char*> vkeys;
                    for (auto const& imap : postProShader)
                        vkeys.push_back(imap.first.c_str());
                    ImGui::Combo("postprocessing", &postProcessingMode, vkeys.data(), vkeys.size());
                    activeShader = postProShader.at(vkeys.at(postProcessingMode));
                    activeShader->use();
                    activeShader->setInt("screenTexture", 0);
                }

                ImGui::SliderFloat("kernel size", &kernelSize, 0.1, 20);
                ImGui::Checkbox("show wireframe", &showWireframe);

                // a Button to reload the shader (so you don't need to recompile the cpp all the time)
                if (ImGui::Button("reload shaders")) {
                    modelShader.reload();
                    modelShader.use();
                    modelShader.setInt("texture1", 0);
                    activeShader->reload();
                    activeShader->use();
                    activeShader->setInt("screenTexture", 0);
                }


                ImGui::End();
            }
            ImGui::Render();
        }

        // render
        // ------
        // bind to framebuffer and draw scene as we normally would to color texture 
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

        // make sure we clear the framebuffer's content
        glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        modelShader.use();
        glm::mat4 model = glm::mat4(1.0f);
        if (rotateModel) model = glm::rotate(model, currentFrame, glm::vec3(0, 1, 0));
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        modelShader.setMat4("model", model);
        modelShader.setMat4("view", view);
        modelShader.setMat4("projection", projection);
        // draw the model
        myModel.Draw(modelShader);

        // now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
        // clear all relevant buffers
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
        glClear(GL_COLOR_BUFFER_BIT);

        // draw as wireframe ? 
        if(showWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        activeShader->use();
        activeShader->setFloat("kernelSize", kernelSize);
        auto randomNumber = ((double)rand() / (RAND_MAX));
        activeShader->setFloat("randomNumber", randomNumber);
        activeShader->setFloat("timer", currentFrame);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);	// use the color attachment texture as the texture of the quad plane
        renderQuad();


        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        if (gui) ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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


    if (leftButtonDown)
        camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
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