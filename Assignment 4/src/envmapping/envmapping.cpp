#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "soil.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <util/filesystem.h>
#include <util/shader.h>
#include <util/camera.h>
#include <util/model.h>
#include <util/assets.h>
#include <util/window.h>

using namespace glm;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

#define NUM_VERTICES 36

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

// settings
int SCR_WIDTH = 1280;
int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;
bool firstMouse = true;
bool leftButtonDown = false;
bool middleButtonDown = false;
bool rightButtonDown = false;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

unsigned int cubeTexture;
int cubeTextureID;
int shaderMode = 0;
bool rotateModel = false;

// --- ASSETS ---
// models and textures used in this tutorial
AssetManager models({
		{   "cube", //group
			{   { "model"   , "../resources/simple/cube.obj" },
				{ "transformation"   , glm::scale(glm::mat4(1.0f), glm::vec3(1.0))  }
			}
		},{   "sphere", //group
			{   { "model"   , "../resources/simple/sphere.obj" },
				{ "transformation"   , glm::scale(glm::mat4(1.0f), glm::vec3(1.0))  }
			}
		},{   "teapot", //group
			{   { "model"   , "../resources/simple/teapot.obj" },
				{ "transformation"   , glm::scale(glm::mat4(1.0f), glm::vec3(1.0))  }
			}
		},{"helmet", //group
			{   { "model"   , "../resources/objects/helmet/helmet.obj" },
				{ "transformation"   , glm::scale(glm::mat4(1.0f), glm::vec3(1.0))  }
			}
		},{"cerberus", //group
			{   { "model"   , "../resources/objects/cerberus/Cerberus_LP.FBX"  },
				{ "transformation"   , glm::scale(glm::mat4(1.0f), glm::vec3(0.01))  }
			}
		}, {"backpack", //group
			{   { "model"   , "../resources/objects/backpack/backpack.obj"  },
				{ "transformation"   , glm::scale(glm::mat4(1.0f), glm::vec3(1.0))  }
			}
		} 
	});




CubeMapPaths sea_cubemap({
	{ "front" , "../resources/textures/cubemaps/sea/front.jpg" },
	{ "back" , "../resources/textures/cubemaps/sea/back.jpg" },
	{ "left" , "../resources/textures/cubemaps/sea/left.jpg" },
	{ "right" , "../resources/textures/cubemaps/sea/right.jpg" },
	{ "bottom" , "../resources/textures/cubemaps/sea/bottom.jpg" },
	{ "top" , "../resources/textures/cubemaps/sea/top.jpg" } 
	});


AssetManager skyboxes({
		{   "beach", //group
			{   //{ TEX_FLIP, true }, // this causes textures to be flipped in y
				{ "cubemap", CubeMapPaths({
					{ "front" , "../resources/textures/cubemaps/beach/back.jpg" },
					{ "back" , "../resources/textures/cubemaps/beach/front.jpg" },
					{ "left" , "../resources/textures/cubemaps/beach/left.jpg" },
					{ "right" , "../resources/textures/cubemaps/beach/right.jpg" },
					{ "bottom" , "../resources/textures/cubemaps/beach/bottom.jpg" },
					{ "top" , "../resources/textures/cubemaps/beach/top.jpg" }
					}) }
			}
		},{ "sea",
		  {	{ "cubemap", sea_cubemap } }
		},{   "winter", //group
			{   //{ TEX_FLIP, true }, // this causes textures to be flipped in y
				{ "cubemap", CubeMapPaths({
					{ "front" , "../resources/textures/cubemaps/winter/pz.jpg" },
					{ "back" , "../resources/textures/cubemaps/winter/nz.jpg" },
					{ "left" , "../resources/textures/cubemaps/winter/nx.jpg" },
					{ "right" , "../resources/textures/cubemaps/winter/px.jpg" },
					{ "bottom" , "../resources/textures/cubemaps/winter/ny.jpg" },
					{ "top" , "../resources/textures/cubemaps/winter/py.jpg" }
					}) }
			}
		},
	});

/*
void loadCubemap() {

	glGenTextures(1, &cubeTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);

	std::string faces[6] = { "right", "left","top","bottom","front","back" };

	//stbi_set_flip_vertically_on_load(true);

	int width, height, nrComponents;
	for (unsigned int i = 0; i < 6; i++) {
		auto imgpath = cubeMap[faces[i]];
		unsigned char* image = stbi_load(imgpath.c_str(), &width, &height, &nrComponents, 0);

		if (image) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
			stbi_image_free(image);
		}
		else {
			std::cout << "Cubemap texture failed to load for: " << faces[i] << std::endl;
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}
*/

const char* APP_NAME = "envmapping";
int main() {
	// glfw: initialize and configure
	// ------------------------------
	InitWindowAndGUI(SCR_WIDTH, SCR_HEIGHT, APP_NAME);

	SetCursorPosCallback(mouse_callback);
	SetMouseButtonCallback(mouse_button_callback);
	SetScrollCallback(scroll_callback);
	SetFramebufferSizeCallback(framebuffer_size_callback);

	skyboxes.SetActiveGroup("beach");
	cubeTexture = skyboxes.GetActiveAsset<Tex>("cubemap");

	Shader skyboxShader("../src/envmapping/skybox.vert", "../src/envmapping/skybox.frag");
	Shader myShader("../src/envmapping/model.vert", "../src/envmapping/model.frag");
	models.SetActiveGroup("sphere");
	Model myModel = models.GetActiveAsset<Model>("model");
	auto modelTransformation = models.GetActiveAsset<glm::mat4>("transformation");
	Model skyboxCube = models.GetAsset<Model>("cube","model");
	//Model myModel("objects/torus/torus.obj");
	//Model myModel("objects/sphere/sphere.obj");
	//Model myModel("objects/cyborg/cyborg.obj");

	myShader.use();
	
	glEnable(GL_DEPTH_TEST);
	
	myShader.setInt("cubeTex", 0);
	skyboxShader.setInt("skybox", 0);
	
	// Main Loop
	while (!glfwWindowShouldClose(window)) {
	
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

				// Combobox with options:
				{
					auto ass = models.GetGroups();
					int item_current = models.GetActiveGroupId();
					std::vector<const char*> strings;
					for (int i = 0; i < ass.size(); ++i)
						strings.push_back(ass[i].c_str());
					ImGui::Combo("model", &item_current, strings.data(), ass.size());
					if (models.GetActiveGroupId() != item_current)
					{
						models.SetActiveGroup(item_current);
						myModel = models.GetActiveAsset<Model>("model");
						modelTransformation = models.GetActiveAsset<glm::mat4>("transformation");
					}
				}
				ImGui::Checkbox("rotate model", &rotateModel);

				// Combobox with options:
				{
					auto ass = skyboxes.GetGroups();
					int item_current = skyboxes.GetActiveGroupId();
					std::vector<const char*> strings;
					for (int i = 0; i < ass.size(); ++i)
						strings.push_back(ass[i].c_str());
					ImGui::Combo("environment", &item_current, strings.data(), ass.size());
					if (skyboxes.GetActiveGroupId() != item_current)
					{
						skyboxes.SetActiveGroup(item_current);
						cubeTexture = skyboxes.GetActiveAsset<Tex>("cubemap");
						glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
					}
				}

				const char* mode_combo[] = { "reflection","refraction" };
				ImGui::Combo("reflect/refract", &shaderMode, mode_combo, 2);

				if (ImGui::Button("reload shaders")) {
					myShader.reload();
					myShader.use();
					myShader.setInt("cubeTex", 0);
					skyboxShader.reload();
					skyboxShader.use();
					skyboxShader.setInt("skybox", 0);
				}


				ImGui::End();
			}
			ImGui::Render();
		}

		mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		mat4 view = camera.GetViewMatrix();
		
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		mat4 model = mat4(1.0f);  
		if (rotateModel) { model = rotate(model, (float)glfwGetTime(), vec3(0.0f, 1.0f, 0.0f)); }
		//model = rotate(model, (float)glfwGetTime(), vec3(0.0f, 0.0f, 1.0f));
		model = translate(model, vec3(0.0f, -0.5f, 0.0f));
		model = scale(model, vec3(0.2f, 0.2f, 0.2f));
		myShader.use();
		myShader.setMat4("projection", projection);
		myShader.setMat4("view", view);
		myShader.setMat4("model", model);
		myShader.setVec3("cameraPos", camera.Position);
		myShader.setInt("mode", shaderMode);

		myModel.Draw(myShader);

		// draw the skybox
		glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
		skyboxShader.use();
		skyboxShader.setMat4("projection", projection);
		skyboxShader.setMat4("view", view);
		skyboxCube.Draw(skyboxShader);
		glDepthFunc(GL_LESS);  // change depth function so depth test passes when values are equal to depth buffer's content

		if (gui) ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
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

	if (leftButtonDown)
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
