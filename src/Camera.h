#pragma once 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    // Add these persistent member variables
    float yaw = -90.0f; // Start facing forward
    float pitch = 0.0f;

    glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);    
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f); 
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        
    // Creates the View Matrix: Defines where the camera is looking
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }

    // Creates the Projection Matrix: Defines the FOV and perspective
    glm::mat4 GetProjectionMatrix(float aspect){
        return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
    }

    void Camera::SetDirection(glm::vec3 direction) {
        // 1. Normalize the direction to ensure it's a unit vector
        direction = glm::normalize(direction);

        // 2. Calculate Pitch (Vertical angle)
        // The asin of the y-component gives the angle in radians
        pitch = glm::degrees(asin(direction.y));

        // 3. Calculate Yaw (Horizontal angle)
        // atan2(z, x) gives the angle in the XZ plane
        yaw = glm::degrees(atan2(direction.z, direction.x)) - 90.0f;

        // 4. Update the internal front, right, and up vectors
        UpdateCameraVectors();
    }

    void UpdateCameraVectors() {
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);
        
        // Recompute right and up
        right = glm::normalize(glm::cross(front, up));
        up    = glm::normalize(glm::cross(right, front));
    }
    
    void RotateCamera(float xOffset, float yOffset) {    
        yaw += xOffset;
        pitch += yOffset; // Note: You might need to swap +/- depending on your preference
        
        // Constrain the pitch to prevent flipping
        if(pitch > 89.0f) pitch = 89.0f;
        if(pitch < -89.0f) pitch = -89.0f;
        
        // Update the front vector based on yaw and pitch
        glm::vec3 newFront;

        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));        
        // when mouse move up player look up to make this we need to put negative sign before sin(pitch) 
        //because in OpenGL the positive Y axis is up, but when we look up we want to decrease the Y value of the front vector
        newFront.y = -sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        
        // Normalize the front vector to ensure consistent movement speed in all directions
        front = glm::normalize(newFront);
        
        // ADD THIS: Calculate the right vector whenever rotation changes
        // Right is the cross product of Front and World Up
        right = glm::normalize(glm::cross(front, up));
    }   
};