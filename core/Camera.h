#pragma once 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class EditorState { Editor, Playing };

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

class Camera {
public:      
    float mouseSensitivity = 0.1f;
    float MovementSpeed = 2.0f;    
    float yaw = -90.0f; // Start facing forward
    float pitch = 0.0f;

    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);    
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f); 
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    // Creates the View Matrix: Defines where the camera is looking
    glm::mat4 GetViewMatrix() const {
        return glm::lookAt(position, position + front, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    // Creates the Projection Matrix: Defines the FOV and perspective
    glm::mat4 GetProjectionMatrix(float aspect) const {
        return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            position += front * velocity;
        if (direction == BACKWARD)
            position -= front * velocity;
        if (direction == LEFT)
            position -= right * velocity;
        if (direction == RIGHT)
            position += right * velocity;
    }

    void UpdateCameraVectors() {    
        // Update the front vector based on yaw and pitch
        glm::vec3 newFront;

        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));

        // when mouse move up player look up to make this we need to put negative sign before sin(pitch) 
        //because in OpenGL the positive Y axis is up, but when we look up we want to decrease the Y value of the front vector
        newFront.y = -sin(glm::radians(pitch));

        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        
        // Normalize the front vector to ensure consistent movement speed in all directions
        front = glm::normalize(newFront);                       

        // Calculate the right vector whenever rotation changes
        // Right is the cross product of Front and World Up
        // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f))); 
        up    = glm::normalize(glm::cross(right, front));
    }

    void SetDirection(glm::vec3 direction) {
        
        // Normalize the direction to ensure it's a unit vector
        direction = glm::normalize(direction);

        // Calculate Pitch (Vertical angle)
        // The asin of the y-component gives the angle in radians
        pitch = glm::degrees(asin(-direction.y));

        // Calculate Yaw (Horizontal angle)        
        yaw = glm::degrees(atan2(direction.z, direction.x));

        // Safety: Clamp pitch to prevent flipping
        if (pitch > 89.0f)  pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        // 4. Update the internal front, right, and up vectors
        UpdateCameraVectors();
    }    
    
    void RotateCamera(float xOffset, float yOffset) {   
         
        yaw += xOffset * mouseSensitivity;
        pitch += yOffset * mouseSensitivity; // Note: You might need to swap +/- depending on your preference
        
        // Constrain the pitch to prevent flipping
        if(pitch > 89.0f) pitch = 89.0f;
        if(pitch < -89.0f) pitch = -89.0f;        
       
        UpdateCameraVectors();
    }   

    glm::vec3 GetForward() {
        glm::vec3 forward;
        forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        forward.y = sin(glm::radians(pitch));
        forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return glm::normalize(forward);
    }


	Ray ScreenToWorldRay(float mouseX, float mouseY, int screenWidth, int screenHeight) const {
		// Calculate aspect ratio
		float aspect = (float)screenWidth / (float)screenHeight;

		// 1. Convert mouse to Normalized Device Coordinates (-1 to 1)
		float x = (2.0f * mouseX) / screenWidth - 1.0f;
		float y = 1.0f - (2.0f * mouseY) / screenHeight; 

		// 2. Define ray in clip space
		glm::vec4 ray_clip(x, y, -1.0f, 1.0f); 
		
		// PASS THE ASPECT RATIO HERE
		glm::vec4 ray_eye = glm::inverse(GetProjectionMatrix(aspect)) * ray_clip;
		ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);

		// Call your GetViewMatrix() (ensure it's marked const as well)
		glm::vec3 ray_world = glm::vec3(glm::inverse(GetViewMatrix()) * ray_eye);
		ray_world = glm::normalize(ray_world);

		return { position, ray_world };
	}


};
