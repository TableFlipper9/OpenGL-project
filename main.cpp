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

#include <iostream>

// sun animation
GLfloat sunAngle = 0.0f;
bool animateSun = false;
GLint viewPosLoc;

// point light (lamp)
glm::vec3 lampPos;
glm::vec3 lampColor;
GLint lampPosLoc;
GLint lampColorLoc;
GLint lampEnabledLoc;
bool lampEnabled = true;

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


// shaders
gps::Shader myBasicShader;

void CalculatePointLight() {
    // lamp (point light) parameters
    lampPos = glm::vec3(cubeModel * glm::vec4(1, 1, 1, 1));
    lampColor = glm::vec3(2.0f, 0.85f, 0.6f);  // warm light
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
	//TODO
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
        100.0f
    );

    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}


void processMovement() {
    if (pressedKeys[GLFW_KEY_W])
        myCamera.move(gps::MOVE_UP, cameraSpeed);
    if (pressedKeys[GLFW_KEY_S])
        myCamera.move(gps::MOVE_DOWN, cameraSpeed);
    if (pressedKeys[GLFW_KEY_A])
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
    if (pressedKeys[GLFW_KEY_D])
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);

    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));

    if (pressedKeys[GLFW_KEY_Q]) {
        angle = 1.0f;
        // update model matrix for teapot
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
        MultiplyByReference();
        CalculatePointLight();
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angle = -1.0f;
        // update model matrix for teapot
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
        MultiplyByReference();
        CalculatePointLight();
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }

    if (animateSun) {
        sunAngle += 0.2f;
        if (sunAngle > 360.0f)
            sunAngle -= 360.0f;

        glm::mat4 rotation = glm::rotate(
            glm::mat4(1.0f),
            glm::radians(sunAngle),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        glm::vec3 rotatedLightDir =
            glm::normalize(glm::vec3(rotation * glm::vec4(lightDir, 0.0f)));

        myBasicShader.useShaderProgram();
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(rotatedLightDir));
    }

    glUniform3fv(viewPosLoc, 1, glm::value_ptr(myCamera.getPosition()));
    glUniform1i(lampEnabledLoc, lampEnabled);
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
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    teapot.LoadModel("models/MainScene.obj");
    tavernBoard.LoadModel("models/tavern/Sign.obj");
    windMill.LoadModel("models/windmill/turbine.obj");
    lampCube.LoadModel("models/cube/cube.obj");

}

void initShaders() {
	myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
}

void initUniforms() {
	myBasicShader.useShaderProgram();

    model = glm::mat4(1.0f);
    // create model matrix for teapot
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

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 20.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

    //sunlight uniforms
    viewPosLoc = glGetUniformLocation(myBasicShader.shaderProgram, "viewPos");
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(myCamera.getPosition()));

    //lamp
    CalculatePointLight();
    lampEnabledLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lampEnabled");
    glUniform1i(lampEnabledLoc, lampEnabled);

	//set the light direction (direction towards the light)
    lightDir = glm::normalize(glm::vec3(0.0f, 1.0f, 1.0f));
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
    normalMatrix = glm::mat3(glm::inverseTranspose(view * boardModel));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    tavernBoard.Draw(myBasicShader);

    shader.useShaderProgram();

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(windmillModel));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * windmillModel));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    windMill.Draw(myBasicShader);
}

//debugg
void renderLampCube(gps::Shader shader) {
    shader.useShaderProgram();

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cubeModel));

    normalMatrix = glm::mat3(glm::inverseTranspose(view * cubeModel));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    lampCube.Draw(shader);
}


void renderTeapot(gps::Shader shader) {
    // select active shader program
    shader.useShaderProgram();

    //send teapot model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //send teapot normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // draw teapot
    teapot.Draw(shader);
}

void renderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//render the scene

	// render the teapot
	renderTeapot(myBasicShader);
    renderBasicObjects(myBasicShader);
    renderLampCube(myBasicShader);
}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
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
	initUniforms();
    setWindowCallbacks();

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
