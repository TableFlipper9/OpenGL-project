#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>
#include <cmath>

// sun animation
GLfloat sunAngle = 0.0f;
bool animateSun = false;
GLint viewPosLoc;
glm::vec3 rotatedLightDir;

// time-based effects
GLint timeLoc;
GLint resolutionLoc;
bool rainEnabled = false;

// point light (lamp)
glm::vec3 lampPos;
glm::vec3 lampColor;
GLint lampPosLoc;
GLint lampColorLoc;
GLint lampEnabledLoc;
bool lampEnabled = true;

// shadow mapping
const unsigned int SHADOW_WIDTH = 4096;
const unsigned int SHADOW_HEIGHT = 4096;
GLuint depthMapFBO = 0;
GLuint depthMap = 0;
gps::Shader shadowShader;
glm::mat4 lightSpaceMatrix(1.0f);

//mouse
float lastX = 512.0f;
float lastY = 384.0f;
bool firstMouse = true;

float mouseSensitivity = 0.1f;
GLfloat cameraSpeed = 0.1f;

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLboolean pressedKeys[1024];

// models
gps::Model3D teapot;
gps::Model3D tavernBoard;
gps::Model3D windMill;
gps::Model3D lampCube;

// matrices for objects 

glm::mat4 boardModel;
glm::mat4 boardLocalTransform;

glm::mat4 windmillModel;
glm::mat4 windmillLocalTransform;

glm::mat4 cubeModel;
glm::mat4 cubeeLocalTransform;

GLfloat angle;

//animations
bool spinWindmill = false;
float windmillAngle = 0.0f;
glm::vec3 windmillPivot = glm::vec3(-4.2f, 16.0f, 1.1f);

// tavern sign swing (wind)
bool swingBoard = false;
float boardMaxSwingDeg = 12.0f;

//viewMode
int renderMode = 0;

//SkyBox
SkyBox skybox;
gps::Shader skyboxShader;

// fog enable
bool fogEnabled = true;


// shaders
gps::Shader myBasicShader;

void AnimateWindmill() {
    glm::mat4 base = model * windmillLocalTransform;

    windmillModel = glm::translate(base, windmillPivot);
    windmillModel = glm::rotate(
        windmillModel,
        glm::radians(windmillAngle),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );
    windmillModel = glm::translate(windmillModel, -windmillPivot);
}

void AnimateBoardSwing(float tSeconds) {

    glm::mat4 base = model * boardLocalTransform;
    float swingDeg = sinf(tSeconds * 1.2f) * boardMaxSwingDeg;
    const glm::vec3 pivotLocal(2.96306f, 6.159437f, 7.3132685f);

    glm::mat4 swing(1.0f);
    swing = glm::translate(swing, pivotLocal);
    swing = glm::rotate(swing, glm::radians(swingDeg), glm::vec3(0.0f, 0.0f, 1.0f));
    swing = glm::translate(swing, -pivotLocal);

    boardModel = base * swing;
}

void CalculatePointLight() {
    lampPos = glm::vec3(cubeModel * glm::vec4(0, 0, 0, 1));
    lampColor = glm::vec3(2.0f, 0.85f, 0.6f);  // warm light
    myBasicShader.useShaderProgram();
    lampPosLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lampPos");
    lampColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lampColor");
    glUniform3fv(lampPosLoc, 1, glm::value_ptr(lampPos));
    glUniform3fv(lampColorLoc, 1, glm::value_ptr(lampColor));
}

void MultiplyByReference() {
    boardModel = model * boardLocalTransform;
    windmillModel = model * windmillLocalTransform;
    cubeModel = model * cubeeLocalTransform;
}

void MultipleObjectsSetup() {
    boardLocalTransform = glm::mat4(1.0f);
    windmillLocalTransform = glm::mat4(1.0f);
    cubeeLocalTransform = glm::mat4(1.0f);

    boardLocalTransform = glm::translate(boardLocalTransform, glm::vec3(-0.644f, 2.482f, -2.1f));
    boardLocalTransform = glm::scale(boardLocalTransform, glm::vec3(0.2f));

    windmillLocalTransform = glm::rotate(windmillLocalTransform, glm::radians(90.0f) ,glm::vec3(0.0f, 1.0f, 0.0f));
    windmillLocalTransform = glm::translate(windmillLocalTransform, glm::vec3(-4.24f, 3.275f, -1.93f));
    windmillLocalTransform = glm::scale(windmillLocalTransform,glm::vec3(0.025f));

    cubeeLocalTransform = glm::translate(cubeeLocalTransform, glm::vec3(-1.12f, 2.9f, -0.1f));
    cubeeLocalTransform = glm::scale(cubeeLocalTransform, glm::vec3(0.05f));

    MultiplyByReference();
}


GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	int fbWidth = 0, fbHeight = 0;
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	myWindow.setWindowDimensions({ fbWidth, fbHeight });
	glViewport(0, 0, fbWidth, fbHeight);

	// Update projection aspect ratio
	myBasicShader.useShaderProgram();
	projection = glm::perspective(
		glm::radians(myCamera.getFov()),
		(float)fbWidth / (float)fbHeight,
		0.1f, 200.0f
	);
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
	if (resolutionLoc != -1) {
		glUniform2f(resolutionLoc, (float)fbWidth, (float)fbHeight);
	}
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        animateSun = !animateSun;
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS) {
        lampEnabled = !lampEnabled;
    }

    if (key == GLFW_KEY_I && action == GLFW_PRESS) {
        spinWindmill = !spinWindmill;
        swingBoard = !swingBoard;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
        rainEnabled = !rainEnabled;
    }

    if (key == GLFW_KEY_U && action == GLFW_PRESS) {
        fogEnabled = !fogEnabled;
    }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        renderMode = 0;
    }

    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        renderMode = 1;
    }

    if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
        renderMode = 2;
    }

    if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
        glShadeModel(GL_FLAT);
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;

    myCamera.rotate(yOffset, xOffset);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    myCamera.zoom((float)yoffset);

    projection = glm::perspective(
        glm::radians(myCamera.getFov()),
        (float)myWindow.getWindowDimensions().width /
        (float)myWindow.getWindowDimensions().height,
        0.1f,
        200.0f
    );

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}


void processMovement() {
    // camera: WASD + SPACE/SHIFT (W/S = forward/back; SPACE/SHIFT = up/down)
    if (pressedKeys[GLFW_KEY_W])
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
    if (pressedKeys[GLFW_KEY_S])
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
    if (pressedKeys[GLFW_KEY_A])
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
    if (pressedKeys[GLFW_KEY_D])
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
    if (pressedKeys[GLFW_KEY_SPACE])
        myCamera.move(gps::MOVE_UP, cameraSpeed);
    if (pressedKeys[GLFW_KEY_LEFT_SHIFT])
        myCamera.move(gps::MOVE_DOWN, cameraSpeed);

    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    // normals are kept in world space (NOT for view)
    normalMatrix = glm::mat3(glm::inverseTranspose(model));

    if (pressedKeys[GLFW_KEY_Q]) {
        angle = 1.0f;
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
        MultiplyByReference();
        CalculatePointLight();
        // update normal matrix for scene
        normalMatrix = glm::mat3(glm::inverseTranspose(model));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angle = -1.0f;
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
        MultiplyByReference();
        CalculatePointLight();
        // update normal matrix for scene
        normalMatrix = glm::mat3(glm::inverseTranspose(model));
    }

    if (animateSun) {
        sunAngle += 0.35f; 
        if (sunAngle > 180.0f) 
            sunAngle -= 180.0f;

        float theta = glm::radians(sunAngle);
        lightDir = glm::normalize(glm::vec3(0.0f, sinf(theta), cosf(theta)));

        myBasicShader.useShaderProgram();
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));
    }

    if (spinWindmill) {
        windmillAngle += 3.0f;
        if (windmillAngle > 360.0f)
            windmillAngle -= 360.0f;
        AnimateWindmill();
    }

    if (swingBoard) {
        AnimateBoardSwing((float)glfwGetTime());
    } else {
        boardModel = model * boardLocalTransform;
    }

    glUniform3fv(viewPosLoc, 1, glm::value_ptr(myCamera.getPosition()));
    glUniform1i(lampEnabledLoc, lampEnabled);

    // time + rain
    glUniform1f(timeLoc, (float)glfwGetTime());
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "rainEnabled"), rainEnabled);
	if (resolutionLoc != -1) {
		glUniform2f(resolutionLoc,
			(float)myWindow.getWindowDimensions().width,
			(float)myWindow.getWindowDimensions().height);
	}
}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetScrollCallback(myWindow.getWindow(), scrollCallback);
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); 
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
    glShadeModel(GL_SMOOTH);

}

void initModels() {
    teapot.LoadModel("models/MainScene.obj");
    tavernBoard.LoadModel("models/tavern/Sign.obj");
    windMill.LoadModel("models/windmill/turbine.obj");
    lampCube.LoadModel("models/cube/cube.obj");

    std::vector<std::string> faces = {
    "models/SkyeBox_blink/right.png",
    "models/SkyeBox_blink/left.png",
    "models/SkyeBox_blink/top.png",
    "models/SkyeBox_blink/bottom.png",
    "models/SkyeBox_blink/front.png",
    "models/SkyeBox_blink/back.png"
    };

    skybox.Load(faces);

}

void initShaders() {
	myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
	shadowShader.loadShader(
		"shaders/shadow.vert",
		"shaders/shadow.frag");
    skyboxShader.loadShader(
        "shaders/skybox.vert",
        "shaders/skybox.frag"
    );

}

void initShadowMap() {
	glGenFramebuffers(1, &depthMapFBO);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0,
		GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initUniforms() {
	myBasicShader.useShaderProgram();
	glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "diffuseTexture"), 0);
	// Reserve a high texture unit for the shadow map so model materials (which may bind
	// multiple textures starting at unit 0) don't accidentally overwrite it.
	glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 10);

    model = glm::mat4(1.0f);
    // create model matrix for scene
    model = glm::translate(model, glm::vec3(0.0f, -0.2f, 0.0f));
    //model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

    MultipleObjectsSetup();

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for teapot (world-space)
    normalMatrix = glm::mat3(glm::inverseTranspose(model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
							   0.1f, 200.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

    //sunlight uniforms
    viewPosLoc = glGetUniformLocation(myBasicShader.shaderProgram, "viewPos");
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(myCamera.getPosition()));

	// time uniform (used for animated effects like rain)
	timeLoc = glGetUniformLocation(myBasicShader.shaderProgram, "time");
	glUniform1f(timeLoc, (float)glfwGetTime());
	glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "rainEnabled"), rainEnabled);
	resolutionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "resolution");
	glUniform2f(resolutionLoc,
		(float)myWindow.getWindowDimensions().width,
		(float)myWindow.getWindowDimensions().height);

	// light-space matrix for shadow mapping (updated each frame)
	glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    //lamp
    CalculatePointLight();
    lampEnabledLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lampEnabled");
    glUniform1i(lampEnabledLoc, lampEnabled);

	// set the sun direction (FROM the scene TOWARDS the sun)
    lightDir = glm::normalize(glm::vec3(0.0f, 0.7f, 0.7f));
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
}

void renderBasicObjects(gps::Shader shader) {
    shader.useShaderProgram();

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(boardModel));
	normalMatrix = glm::mat3(glm::inverseTranspose(boardModel));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    tavernBoard.Draw(myBasicShader);

    shader.useShaderProgram();

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(windmillModel));
	normalMatrix = glm::mat3(glm::inverseTranspose(windmillModel));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    windMill.Draw(myBasicShader);
}

//debugg
void renderLampCube(gps::Shader shader) {
    shader.useShaderProgram();

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cubeModel));

	normalMatrix = glm::mat3(glm::inverseTranspose(cubeModel));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    lampCube.Draw(shader);
}


void renderObjects(gps::Shader shader) {
    // select active shader program
    shader.useShaderProgram();

    //send scene model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //send scene normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // draw Scene
    teapot.Draw(shader);
}

void renderScene() {
    // Shadow pass: render depth from light POV
    glm::mat4 lightProjection = glm::ortho(-70.0f, 70.0f, -70.0f, 70.0f, 1.0f, 150.0f);
    glm::vec3 sceneCenter(0.0f, 5.0f, 0.0f);
    glm::vec3 lightPos = sceneCenter + lightDir * 80.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));
    lightSpaceMatrix = lightProjection * lightView;

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glDisable(GL_CULL_FACE);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 4.0f);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    shadowShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(shadowShader.shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    // render scene to depth map
    glUniformMatrix4fv(glGetUniformLocation(shadowShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    teapot.Draw(shadowShader);

    glUniformMatrix4fv(glGetUniformLocation(shadowShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(boardModel));
    tavernBoard.Draw(shadowShader);

    glUniformMatrix4fv(glGetUniformLocation(shadowShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(windmillModel));
    windMill.Draw(shadowShader);

    glUniformMatrix4fv(glGetUniformLocation(shadowShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));
    lampCube.Draw(shadowShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Restore state for main pass
	glDisable(GL_POLYGON_OFFSET_FILL);
	glEnable(GL_CULL_FACE);

    // Main pass
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
	if (resolutionLoc != -1) {
		glUniform2f(resolutionLoc,
			(float)myWindow.getWindowDimensions().width,
			(float)myWindow.getWindowDimensions().height);
	}
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, depthMap);

	//render the scene
    switch (renderMode) {
    case 0:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;

    case 1:
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;

    case 2:
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        break;
    }
;
	// render the objects
	renderObjects(myBasicShader);
    renderBasicObjects(myBasicShader);
    renderLampCube(myBasicShader);


    myBasicShader.useShaderProgram();

    glUniform3f(
        glGetUniformLocation(myBasicShader.shaderProgram, "fogColor"),
        0.6f, 0.7f, 0.8f   // light bluish fog
    );

    glUniform1f(
        glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity"),
        0.015f
    );

    glUniform1i(
        glGetUniformLocation(myBasicShader.shaderProgram, "fogEnabled"),
        fogEnabled
    );

    // draw skybox
    skyboxShader.useShaderProgram();
    glUniform1i(
        glGetUniformLocation(skyboxShader.shaderProgram, "skybox"),
        0
    );
    glUniformMatrix4fv(
        glGetUniformLocation(skyboxShader.shaderProgram, "view"),
        1, GL_FALSE, glm::value_ptr(view)
    );
    glUniformMatrix4fv(
        glGetUniformLocation(skyboxShader.shaderProgram, "projection"),
        1, GL_FALSE, glm::value_ptr(projection)
    );

    skybox.Draw(skyboxShader);

}


void cleanup() {
    // IMPORTANT: delete GPU resources *before* destroying the OpenGL context.

    // 1) Shadow map resources
    if (depthMapFBO != 0) {
        glDeleteFramebuffers(1, &depthMapFBO);
        depthMapFBO = 0;
    }
    if (depthMap != 0) {
        glDeleteTextures(1, &depthMap);
        depthMap = 0;
    }

    // 2) Skybox GPU buffers/texture
    skybox.cleanup();

    // 3) Shader programs
    if (myBasicShader.shaderProgram) {
        glDeleteProgram(myBasicShader.shaderProgram);
        myBasicShader.shaderProgram = 0;
    }
    if (shadowShader.shaderProgram) {
        glDeleteProgram(shadowShader.shaderProgram);
        shadowShader.shaderProgram = 0;
    }
    if (skyboxShader.shaderProgram) {
        glDeleteProgram(skyboxShader.shaderProgram);
        skyboxShader.shaderProgram = 0;
    }

    // 4) Models 
    teapot.~Model3D();
    new (&teapot) gps::Model3D();

    tavernBoard.~Model3D();
    new (&tavernBoard) gps::Model3D();

    windMill.~Model3D();
    new (&windMill) gps::Model3D();

    lampCube.~Model3D();
    new (&lampCube) gps::Model3D();

    // 5) Destroy window and terminate GLFW (destroys GL context)
    myWindow.Delete();
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
	initModels();
	initShaders();
	initShadowMap();
	initUniforms();
    setWindowCallbacks();
    skybox.init();

	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
