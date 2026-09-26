#include "Camera.h"
#include "Core/Main.h"
#include <glm/gtx/string_cast.hpp>
#include "Gameplay/Player.h"
#include "utils/FE_math.h"
#include "render/window/WindowHandler.h"

// Global Variables

float timeAccumulator = 0;

void Camera::InitCamera(int width, int height, glm::vec3 position){
    Camera::width = width;
    Camera::height = height;
    Position = position;
}

void Camera::SetViewportSize(int newWidth, int newHeight) {
    width = newWidth;
    height = newHeight;
    aspect = static_cast<float>(width) / static_cast<float>(height);
}

void Camera::updateMatrix(){
    //std::cout << "Internal camera instance address: " << this << std::endl;
    // Initializes matrices

    // Makes camera look in the right direction from the right position
    view = glm::lookAt(Position, Position + Orientation, Up);
    Right = glm::normalize(glm::cross(Orientation, Up));
    
    aspect = static_cast<float>(width) / static_cast<float>(height);
    
    // Adds perspective to the scene
    projection = glm::perspective(glm::radians(fov), aspect, nearFar.x, nearFar.y);
    
    projectionAlwaysUnjittered = projection;
    cameraMatrixAlwaysUnjittered = projection * view;

    if (applyJitter) projection = FE_Math::createHaltonJitterProjectionMatrix(projection, currentJitter, width, height);
    
    cameraMatrix = projection * view;
}

void Camera::Matrix(Shader& shader, const char* uniform){
    // Exports the camera matrix to the Vertex Shader
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, uniform), 1, GL_FALSE, glm::value_ptr(cameraMatrix));
}

void Camera::saveLastMaticies(){
    previousJitter = currentJitter; 
    scaledPreviousJitter = scaledCurrentJitter;
    lastCameraMatrix = cameraMatrix; // set the previous
    
    hView = view;
    hProjection = projection;
}

const glm::vec2 HaltonJitters[8] = {
    glm::vec2( 0.0000f, -0.1666f),
    glm::vec2(-0.2500f,  0.1666f),
    glm::vec2( 0.2500f, -0.3888f),
    glm::vec2(-0.3750f, -0.0555f),
    glm::vec2( 0.1250f,  0.2777f),
    glm::vec2(-0.1250f, -0.2777f),
    glm::vec2( 0.3750f,  0.0555f),
    glm::vec2(-0.4375f,  0.3888f)
};

void Camera::updateHaltonJitter(float jitterFactor){
    // increase 
    jitterIndex +=1;
    if (jitterIndex > 7) jitterIndex = 0;// index starts at 0
    
    // fetch jitter

   currentJitter = HaltonJitters[jitterIndex] * jitterFactor;
    
    scaledCurrentJitter = glm::vec2(
    (currentJitter.x * 2.0f) / (float)width,
    (currentJitter.y * 2.0f) / (float)height
    );
}

void Camera::Inputs(GLFWwindow* window, float deltatime)
{
    float adjustedSpeed = speed * deltatime;
	if (!Player::s_DoGravity)
	{
        // Handles inputs
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            Position += adjustedSpeed * Orientation;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            Position += adjustedSpeed * -glm::normalize(glm::cross(Orientation, Up));
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            Position += adjustedSpeed * -Orientation;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            Position += adjustedSpeed * glm::normalize(glm::cross(Orientation, Up));
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) //jump
            Position += adjustedSpeed * Up;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            Position += adjustedSpeed * -Up;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            s_scrollSpeed += 10.0f * deltatime;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            s_scrollSpeed -= 10.0f * deltatime;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            speed = (10.0f + s_scrollSpeed);
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
            speed = (5.0f + s_scrollSpeed);
	}
    else {
        // Flattened forward direction (ignore Y component)
        glm::vec3 flatOrientation = glm::normalize(glm::vec3(Orientation.x, 0.0f, Orientation.z));
        /*
                // Handles inputs
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            Position += adjustedSpeed * flatOrientation;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            Position += adjustedSpeed * -glm::normalize(glm::cross(flatOrientation, Up));
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            Position += adjustedSpeed * -flatOrientation;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            Position += adjustedSpeed * glm::normalize(glm::cross(flatOrientation, Up));
        }
        */

        speed = s_scrollSpeed;
    }
}

void Camera::inputsMouse(GLFWwindow* window)
{
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        if (toggleESC) {
            MouseState = !MouseState;
            toggleESC = false;
        }

    }
    else
    {
        toggleESC = true;
    }
    // Handles mouse inputs
    if (MouseState)
    {
        // Disables mouse cursor and prevents it from leaving the window
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Prevents camera from jumping on the first click
        if (firstClick)
        {
            glfwSetCursorPos(window, (width / 2), (height / 2));
            firstClick = false;
        }

       
        // Stores the coordinates of the cursor
        //double mouseX =  windowHandler::mouseX;
        //double mouseY =  windowHandler::mouseY;
        // Fetches the coordinates of the cursor
        
        double mouseX;
        double mouseY;
        // Fetches the coordinates of the cursor
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Normalizes and shifts the coordinates of the cursor such that they begin in the middle of the screen
        // and then "transforms" them into degrees 
        float rotX = Camera::sensitivity.y * (float)(mouseY - (height / 2)) / height;
        float rotY = Camera::sensitivity.x * (float)(mouseX - (width / 2)) / width;
       
        // Calculates upcoming vertical change in the Orientation
        glm::vec3 newOrientation = glm::rotate(Orientation, glm::radians(-rotX), glm::normalize(glm::cross(Orientation, Up)));

        // Decides whether or not the next vertical Orientation is legal or not
        if (abs(glm::angle(newOrientation, Up) - glm::radians(90.0f)) <= glm::radians(85.0f))
        {
            Orientation = newOrientation;
        }

        // Rotates the Orientation left and right
        Orientation = glm::rotate(Orientation, glm::radians(-rotY), Up);

        // Sets mouse cursor to the middle of the screen so that it doesn't end up roaming around
        glfwSetCursorPos(window, (width / 2), (height / 2));
    }
    else
    {
        // Unhides cursor since camera is not looking around anymore
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        // Makes sure the next time the camera looks around it doesn't jump
        firstClick = true;
    }
}

