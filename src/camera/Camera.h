#ifndef CAMERA_CLASS_H
#define CAMERA_CLASS_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "Render/shader/shaderClass.h"
#include "Core/Main.h"
#include "utils/timeUtil.h"

class Camera
{
public:
    
    bool MouseState = true, toggleESC = true;
    
    // Stores camera transform
    float s_scrollSpeed = 5.0f;
    glm::vec2 sensitivity;
    glm::mat4 cameraMatrix = glm::mat4(1.0f);
    glm::mat4 lastCameraMatrix = glm::mat4(1.0f); // cam mat of the previous frame
    glm::mat4 cameraMatrixAlwaysUnjittered = glm::mat4(1.0f);
    glm::vec3 Position = glm::vec3(0, 0, 0);
    glm::vec3 Orientation = glm::vec3(0.0f, 0.0f, -1.0f); // forward vecotr
    
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Right = glm::vec3(0.0f, 0.0f, 0.0f);
    
    bool firstClick = false;
    int width;
    int height;
    float aspect = 0.0f;
    float speed = 0.1f;
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 hView = glm::mat4(1.0f);
    glm::mat4 hProjection = glm::mat4(1.0f);
    glm::mat4 projectionAlwaysUnjittered = glm::mat4(1.0f);
    float fov = 60.0f;
    glm::vec2 nearFar = glm::vec2(0.1f, 100.0f);
    float gamma = 2.2f;
    
    // jitter
    int jitterIndex = 0;  // jitter frame 0-7 (8)
    glm::vec2 currentJitter = glm::vec2(0.0);
    glm::vec2 previousJitter = glm::vec2(0.0);
    glm::vec2 scaledCurrentJitter = glm::vec2(0.0);
    glm::vec2 scaledPreviousJitter = glm::vec2(0.0);
    bool applyJitter = false;

    // Camera constructor
    void InitCamera(int width, int height, glm::vec3 position);
    void SetViewportSize(int newWidth, int newHeight);
    // Updates and exports the camera matrix to the Vertex Shader
    void updateMatrix();
    void Matrix(Shader& shader, const char* uniform);
    
    void saveLastMaticies();

    void updateHaltonJitter(float jitterFactor);
    
    // Handles camera inputs
    void Inputs(GLFWwindow* window, float deltatime);
    
    void inputsMouse(GLFWwindow* window);

};

#endif