// ============================================================================
// 深度测试教程 - 解决 Z-Fighting 的多种方法
// ============================================================================
// 
// 方法1: Reverse-Z (需要 OpenGL 4.5+ 的 glClipControl)
//   - 在 OpenGL 3.3 中效果有限，因为 NDC 范围是 [-1,1]
//   - 浮点精度优势无法充分发挥
//
// 方法2: 对数深度缓冲 (Logarithmic Depth Buffer) - 本示例使用
//   - 在顶点/片段着色器中手动计算对数深度
//   - 在 OpenGL 3.3 中效果显著
//   - 适用于大场景（如太空、地球等）
//
// 方法3: 调整近平面距离
//   - 近平面越大，精度越高
//   - 但会裁剪掉近处物体
//
// ============================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 深度模式开关（按 R 键切换）
// 0 = 传统深度
// 1 = 对数深度
int depthMode = 0;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL - Reverse-Z", NULL, NULL);
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

	// ========================================================================
	// 配置深度测试
	// ========================================================================
	std::cout << "=== 深度测试配置 ===" << std::endl;
	std::cout << "按 R 键切换深度模式 (传统 / 对数深度)" << std::endl;
	std::cout << "当前状态: 传统深度" << std::endl;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearDepth(1.0);

	// 加载两套着色器：传统深度 和 对数深度
	// 使用 FileSystem::getPath 获取正确的路径
	Shader shaderNormal(
		FileSystem::getPath("src/4.advanced_opengl/1.1.depth_testing/1.1.depth_testing.vs").c_str(),
		FileSystem::getPath("src/4.advanced_opengl/1.1.depth_testing/1.1.depth_testing.fs").c_str()
	);
	Shader shaderLogZ(
		FileSystem::getPath("src/4.advanced_opengl/1.1.depth_testing/1.1.depth_testing_logz.vs").c_str(),
		FileSystem::getPath("src/4.advanced_opengl/1.1.depth_testing/1.1.depth_testing_logz.fs").c_str()
	);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	float cubeVertices[] = {
		// positions          // texture Coords
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};
	float planeVertices[] = {
		// positions          // texture Coords
		 5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
		-5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
		-5.0f, -0.5f, -5.0f,  0.0f, 2.0f,

		 5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
		-5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
		 5.0f, -0.5f, -5.0f,  2.0f, 2.0f
	};
	
	// ========================================================================
	// Z-Fighting 测试平面 - 两个几乎重叠的平面
	// ========================================================================
	// 这两个平面距离非常近 (0.0001)，用于演示 Z-Fighting
	// 在传统深度测试中，这两个平面会产生明显的闪烁
	// 使用 Reverse-Z 可以大幅减少这种现象
	float zFightPlane1[] = {
		// 第一个平面 (红色) - y = 0.5
		 2.0f, 0.5f,  2.0f,  1.0f, 0.0f,
		-2.0f, 0.5f,  2.0f,  0.0f, 0.0f,
		-2.0f, 0.5f, -2.0f,  0.0f, 1.0f,
		 2.0f, 0.5f,  2.0f,  1.0f, 0.0f,
		-2.0f, 0.5f, -2.0f,  0.0f, 1.0f,
		 2.0f, 0.5f, -2.0f,  1.0f, 1.0f
	};
	float zFightPlane2[] = {
		// 第二个平面 (蓝色) - y = 0.5001 (仅相差 0.0001!)
		 2.0f, 0.5001f,  2.0f,  1.0f, 0.0f,
		-2.0f, 0.5001f,  2.0f,  0.0f, 0.0f,
		-2.0f, 0.5001f, -2.0f,  0.0f, 1.0f,
		 2.0f, 0.5001f,  2.0f,  1.0f, 0.0f,
		-2.0f, 0.5001f, -2.0f,  0.0f, 1.0f,
		 2.0f, 0.5001f, -2.0f,  1.0f, 1.0f
	};

	// plane VAO
	unsigned int planeVAO, planeVBO;
	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glBindVertexArray(planeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindVertexArray(0);

	// cube VAO
	unsigned int cubeVAO, cubeVBO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindVertexArray(0);
	
	// Z-Fighting 测试平面1 VAO
	unsigned int zfVAO1, zfVBO1;
	glGenVertexArrays(1, &zfVAO1);
	glGenBuffers(1, &zfVBO1);
	glBindVertexArray(zfVAO1);
	glBindBuffer(GL_ARRAY_BUFFER, zfVBO1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(zFightPlane1), &zFightPlane1, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindVertexArray(0);
	
	// Z-Fighting 测试平面2 VAO
	unsigned int zfVAO2, zfVBO2;
	glGenVertexArrays(1, &zfVAO2);
	glGenBuffers(1, &zfVBO2);
	glBindVertexArray(zfVAO2);
	glBindBuffer(GL_ARRAY_BUFFER, zfVBO2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(zFightPlane2), &zFightPlane2, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindVertexArray(0);


	// load textures
	unsigned int cubeTexture = loadTexture(FileSystem::getPath("resources/textures/marble.jpg").c_str());
	unsigned int floorTexture = loadTexture(FileSystem::getPath("resources/textures/metal.png").c_str());
	// Z-Fighting 测试平面使用不同的纹理以便区分
	unsigned int zfTexture1 = loadTexture(FileSystem::getPath("resources/textures/container.jpg").c_str());
	unsigned int zfTexture2 = loadTexture(FileSystem::getPath("resources/textures/brickwall.jpg").c_str());

	// shader configuration
	shaderNormal.use();
	shaderNormal.setInt("texture1", 0);
	
	shaderLogZ.use();
	shaderLogZ.setInt("texture1", 0);

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 view = camera.GetViewMatrix();

		// ====================================================================
		// 投影矩阵配置
		// ====================================================================
		float nearPlane = 0.1f;
		float farPlane = 10000.0f;  // 使用很大的远平面来展示 Z-Fighting
		float fov = glm::radians(camera.Zoom);
		float aspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;
		glm::mat4 projection = glm::perspective(fov, aspect, nearPlane, farPlane);
		
		// 选择着色器并设置 uniform
		Shader* currentShader;
		if (depthMode == 1) {
			// 对数深度模式
			currentShader = &shaderLogZ;
			currentShader->use();
			
			// 计算对数深度系数
			// Fcoef = 2.0 / log2(farPlane + 1.0)
			float Fcoef = 2.0f / log2(farPlane + 1.0f);
			currentShader->setFloat("Fcoef", Fcoef);
			currentShader->setFloat("Fcoef_half", Fcoef * 0.5f);
		}
		else {
			// 传统深度模式
			currentShader = &shaderNormal;
			currentShader->use();
		}

		currentShader->setMat4("view", view);
		currentShader->setMat4("projection", projection);

		// floor
		glBindVertexArray(planeVAO);
		glBindTexture(GL_TEXTURE_2D, floorTexture);
		currentShader->setMat4("model", glm::mat4(1.0f));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		// cubes
		glBindVertexArray(cubeVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cubeTexture);
		model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
		currentShader->setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
		currentShader->setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		
		// ====================================================================
		// 绘制 Z-Fighting 测试平面
		// ====================================================================
		// 这两个平面仅相距很小的距离，用于演示 Z-Fighting
		// 移动相机远离这两个平面，观察闪烁现象
		// 按 R 键切换到对数深度模式，观察改善效果
		
		// 绘制测试平面1 (container 纹理)
		glBindVertexArray(zfVAO1);
		glBindTexture(GL_TEXTURE_2D, zfTexture1);
		currentShader->setMat4("model", glm::mat4(1.0f));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		// 绘制测试平面2 (brickwall 纹理)
		glBindVertexArray(zfVAO2);
		glBindTexture(GL_TEXTURE_2D, zfTexture2);
		currentShader->setMat4("model", glm::mat4(1.0f));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		glBindVertexArray(0);
		
		// ====================================================================
		// 输出相机与测试平面的距离（每秒输出一次）
		// ====================================================================
		static float lastPrintTime = 0.0f;
		if (currentFrame - lastPrintTime >= 1.0f) {
			lastPrintTime = currentFrame;
			
			// 测试平面中心位置 (0, 0.5, 0)
			glm::vec3 planeCenter(0.0f, 0.5f, 0.0f);
			glm::vec3 cameraPos = camera.Position;
			float distance = glm::length(cameraPos - planeCenter);
			
			// 计算深度值 (在视图空间中的 Z)
			glm::vec4 planePosView = view * glm::vec4(planeCenter, 1.0f);
			float viewZ = -planePosView.z;  // OpenGL 相机看向 -Z 方向
			
			// 计算裁剪空间和 NDC 深度
			glm::vec4 planePosClip = projection * planePosView;
			float ndcZ = planePosClip.z / planePosClip.w;
			float depthBufferNormal = (ndcZ + 1.0f) * 0.5f;  // OpenGL 默认 [-1,1] -> [0,1]
			
			// 对数深度值
			float Fcoef = 2.0f / log2(farPlane + 1.0f);
			float depthBufferLog = log2(viewZ + 1.0f) * Fcoef * 0.5f;
			
			std::cout << "----------------------------------------" << std::endl;
			std::cout << "相机位置: (" << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ")" << std::endl;
			std::cout << "到测试平面距离: " << distance << " 单位" << std::endl;
			std::cout << "视图空间 Z: " << viewZ << std::endl;
			std::cout << "传统深度缓冲值: " << depthBufferNormal << std::endl;
			std::cout << "对数深度缓冲值: " << depthBufferLog << std::endl;
			std::cout << "模式: " << (depthMode == 1 ? "对数深度" : "传统深度") << std::endl;
			std::cout << "----------------------------------------" << std::endl;
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// de-allocate resources
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &planeVAO);
	glDeleteVertexArrays(1, &zfVAO1);
	glDeleteVertexArrays(1, &zfVAO2);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &planeVBO);
	glDeleteBuffers(1, &zfVBO1);
	glDeleteBuffers(1, &zfVBO2);

	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	// 相机移动
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);

	// ====================================================================
	// 按 R 键切换深度模式 
	// ====================================================================
	static bool rKeyPressed = false;
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rKeyPressed) {
		rKeyPressed = true;
		depthMode = (depthMode + 1) % 2;

		std::cout << "\n=== 切换深度测试模式 ===" << std::endl;
		if (depthMode == 0) {
			std::cout << "当前模式: 传统深度" << std::endl;
			std::cout << "  - 深度精度集中在近平面" << std::endl;
			std::cout << "  - 远处物体容易 Z-Fighting" << std::endl;
		}
		else {
			std::cout << "当前模式: 对数深度 (Logarithmic Depth)" << std::endl;
			std::cout << "  - 深度精度均匀分布" << std::endl;
			std::cout << "  - 大场景效果显著" << std::endl;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
		rKeyPressed = false;
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
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

// utility function for loading a 2D texture from file
unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

// ============================================================================
// 对数深度缓冲 (Logarithmic Depth Buffer) 技术说明
// ============================================================================
//
// 【什么是 Z-Fighting？】
// Z-Fighting 是深度缓冲精度不足导致的闪烁现象，常见于：
// - 两个面非常接近时
// - 距离相机很远的物体
// - 深度范围很大的场景（如太空、地球等）
//
// 【传统深度缓冲的问题】
// 传统深度值计算：depth = (1/z - 1/near) / (1/far - 1/near)
// - 精度分布极不均匀
// - 50% 的精度在 near 到 near*2 之间
// - 远处物体几乎没有精度
//
// 例如 near=0.1, far=10000:
// - 距离 0.1-0.2: 50% 精度
// - 距离 0.2-10000: 另外 50% 精度
//
// 【对数深度缓冲解决方案】
// 公式：depth = log2(z + 1) * (2 / log2(far + 1))
// 
// 优势：
// 1. 精度在整个深度范围内均匀分布
// 2. 适用于极大的深度范围（如 0.1 到 100000000）
// 3. 有效消除 Z-Fighting
//
// 【实现方法】
// 1. 顶点着色器：计算 flogz = log2(gl_Position.w + 1.0) * Fcoef
// 2. 片段着色器：gl_FragDepth = flogz * 0.5
// 3. Fcoef = 2.0 / log2(farPlane + 1.0)
//
// 【为什么不用 Reverse-Z？】
// Reverse-Z 在 OpenGL 4.5+ (有 glClipControl) 效果很好
// 但 OpenGL 3.3 的 NDC 范围固定为 [-1,1]，效果有限
// 对数深度在 OpenGL 3.3 中是更好的选择
//
// 【对比效果】
// 按 R 键可以实时切换 传统深度/对数深度，观察区别
// 远离测试平面，观察 Z-Fighting 的变化
//
// ============================================================================

