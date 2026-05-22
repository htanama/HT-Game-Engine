#pragma once
#include "ECS.h"
#include "Shader.h"

void RenderSystem(Registry& reg, Shader& shader, float time) {
    for (Entity i = 0; i < reg.hasTransform.size(); ++i) {
        if (reg.hasTransform[i] && reg.hasRenderable[i]) {
            // Create a model matrix for this entity based on its Transform component
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, reg.transforms[i].position);
            
            // Add the same rotation logic you had in your main loop
            // model = glm::rotate(model, time * glm::radians(50.0f), glm::vec3(0.5f, -1.0f, 0.0f));
            
            if(reg.hasRotation[i]){
               reg.rotations[i].angle += reg.rotations[i].speed * time; // Rotate 20 degrees per second
               model = glm::rotate(model, glm::radians(reg.rotations[i].angle), reg.rotations[i].axis);
            }

            // Pass the model matrix to the shader for this entity
            shader.setMat4("model", model);

            // If the Renderable component has a specific color and isn't using vertex colors, set that uniform
            if (i < reg.hasColor.size() && reg.hasColor[i]) {
                shader.setVec3("objectColor", reg.colors[i].color);
                shader.setbBool("isVertexColor", false);
            } else {
                shader.setbBool("objectColor", true); // Tell shader to use vertex colors
            }

            // Draw the mesh associated with this entity
            reg.renderables[i].mesh->draw();
        }
    }
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