#pragma once 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    // Creates the View Matrix: Defines where the camera is looking
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }

    // Creates the Projection Matrix: Defines the FOV and perspective
    glm::mat4 GetProjectionMatrix(float aspect){
        return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    }
};