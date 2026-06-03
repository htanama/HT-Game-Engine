#pragma once
#include "ECS.h"
#include "Shader.h"

void CleanupUnusedMeshes();

// Search for an entity by name
Entity FindEntityByName(Registry& reg, const std::string& name) {
    for (Entity i = 0; i < reg.names.size(); ++i) {
        if (reg.hasName[i] && reg.names[i].name == name) {
            return i;
        }
    }
    return -1; // Not found
}

// Safely mark an entity for deletion
void RequestDeleteEntity(Registry& registry, Entity entityID) {
   if (entityID < registry.hasTransform.size()) {
        registry.hasTransform[entityID] = false;
        registry.hasRenderable[entityID] = false;
        registry.hasVelocity[entityID] = false;
        registry.hasName[entityID] = false;
        registry.hasColor[entityID] = false;
        registry.hasRotation[entityID] = false;
        registry.hasLifetime[entityID] = false;
    }

    registry.renderables[entityID].mesh = nullptr;    
      
}

void MovementSystem(Registry& reg, float deltaTime) {
    // Iterate through the vector using an index to get the Entity ID
    for (size_t entity = 0; entity < reg.hasTransform.size(); ++entity) {
        
        // Ensure the entity actually has both a Transform and a Velocity component
        if (reg.hasTransform[entity] && reg.hasVelocity[entity]) {
            
            // Apply the velocity: Position += Velocity * DeltaTime
            reg.transforms[entity].position += reg.velocities[entity].value * deltaTime;
        }
    }
}


void LifetimeSystem(Registry& reg, float deltaTime) {
    for (size_t entity = 0; entity < reg.hasLifetime.size(); ++entity) {
        if (reg.hasLifetime[entity]) {
            reg.lifetimes[entity].remainingTime -= deltaTime;
            if (reg.lifetimes[entity].remainingTime <= 0.0f) {
                // For simplicity, we just mark the entity as not having a Transform and Renderable anymore
                reg.hasTransform[entity] = false;
                reg.hasRenderable[entity] = false;
                reg.hasVelocity[entity] = false;
                reg.hasColor[entity] = false;
                reg.hasRotation[entity] = false;
                reg.hasLifetime[entity] = false;                                    
            }
        }
    }
}

Entity GetProjectile(Registry& reg) {
    // Look through the existing entities to find one that has a LifetimeComponent 
    // but no Transform or Renderable (i.e., an "empty" projectile slot)
    for (size_t entity = 0; entity < reg.hasLifetime.size(); ++entity   ) {
        
        if (reg.hasLifetime[entity] && !reg.hasTransform[entity] && !reg.hasRenderable[entity]) {
            return entity; // Reuse this entity ID for a new projectile
        
        }
    }

    if (reg.hasTransform.size() > 1000) {
        std::cerr << "Warning: Too many entities! Consider implementing an entity pool or recycling system." << std::endl;
        return -1;
    }

    // If no empty slot is found, create a new entity
    return reg.CreateEntity();
}

void CameraSystem(Registry& reg, Entity playerID, Camera& cam) {
    if (playerID != -1 && reg.hasTransform[playerID]) {
        // Sync position
        glm::vec3 playerPos = reg.transforms[playerID].position;
        cam.position = playerPos + glm::vec3(0.0f, 1.6f, 0.0f); // 1.6f offset for head height
        
        // Ensure the camera recalculates the view matrix using its new position
        cam.UpdateCameraVectors();
    }
}

void RenderSystem(Registry& reg, Shader& shader, float time) {
    std::cout << "Rendering!!!! " << std::endl;
    
    for (Entity i = 0; i < reg.hasTransform.size(); ++i) {
        if (reg.hasTransform[i] && reg.hasRenderable[i]) {           
            // DEBUG: Check if mesh is valid
            if (reg.renderables[i].mesh == nullptr) {
                std::cout << "Error: Entity " << i << " has no mesh assigned!" << std::endl;
                continue; 
            }
            
            // Build the model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, reg.transforms[i].position);
            
            if (reg.hasRotation[i]) {
                model = glm::rotate(model, glm::radians(reg.rotations[i].angle), reg.rotations[i].axis);
            }
            
            // Pass the model matrix to the shader
            shader.setMat4("model", model);

            // Bind and Draw
            reg.renderables[i].mesh->draw();

            // DEBUG: Check for GL Errors immediately after draw
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                std::cout << "OpenGL Error after draw: " << err << std::endl;
            }
        }
    }
}
