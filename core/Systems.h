#pragma once
#include "ECS.h"
#include "Shader.h"

constexpr float EPSILON = 1e-6f;

// A simple AABB structure for object
struct AABB{
	glm::vec3 min;
	glm::vec3 max;
};

// Return true if the ray hits the AABB
bool RayIntersectsAABB(const Ray& ray, glm::vec3 pos, glm::vec3 scale, float& hitDistance) {
    glm::vec3 min = pos - (scale * 0.5f);
    glm::vec3 max = pos + (scale * 0.5f);
    
    // Slab intersection algorithm
    glm::vec3 invDir = 1.0f / ray.direction;
    glm::vec3 t0 = (min - ray.origin) * invDir;
    glm::vec3 t1 = (max - ray.origin) * invDir;
    
    glm::vec3 tmin = glm::min(t0, t1);
    glm::vec3 tmax = glm::max(t0, t1);
    
    float t_near = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
    float t_far = glm::min(glm::min(tmax.x, tmax.y), tmax.z);
    
    if (t_far < 0 || t_near > t_far) return false;
    hitDistance = t_near;
    return true;
}

// Returns true if the ray intersects the plane, and sets 'distance' to the hit point
bool RayIntersectsPlane(const Ray& ray, glm::vec3 planePoint, glm::vec3 planeNormal, float& hitDistance) {
    float denom = glm::dot(planeNormal, ray.direction);
    
    // Check if the ray is parallel to the plane (avoid division by zero)
    if (std::abs(denom) > EPSILON) {
        glm::vec3 originToPlane = planePoint - ray.origin;
        hitDistance = glm::dot(originToPlane, planeNormal) / denom;
        
        // If distance is negative, the plane is behind the ray origin
        return (hitDistance >= 0);
    }
    
    return false;
}

Entity PickEntity(const Ray& ray, Registry& reg) {
    Entity closestEntity = -1;
    float closestDistance = 9999.0f;

    for (size_t e = 0; e < reg.hasTransform.size(); ++e) {
        if (!reg.hasTransform[e] || !reg.hasRenderable[e]) continue;

        float distance;
        if (RayIntersectsAABB(ray, reg.transforms[e].position, reg.transforms[e].scale, distance)) {
            if (distance < closestDistance) {
                closestDistance = distance;
                closestEntity = (Entity)e;
            }
        }
    }
    return closestEntity;
}


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
        registry.hasTexture[entityID] = false; 

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

void RenderSystem(Registry& reg, Shader& shader) { 

    for (size_t e = 0; e < reg.renderables.size(); ++e)
    {
        if (!reg.hasRenderable[e]) continue; // Skip if no mesh to draw

        // Iterate through every possible entity ID
        // Set the model matrix using your transform component (if it exists)
        glm::mat4 model = glm::mat4(1.0f); // Local scope is safer

        auto& renderable = reg.renderables[e];

        // Build the model matrix once using the transform
        if (reg.hasTransform[e]) {
            auto& t = reg.transforms[e];
            model = glm::translate(model, t.position);
            model = glm::rotate(model, glm::radians(t.rotation.x), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(t.rotation.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(t.rotation.z), glm::vec3(0, 0, 1));
            model = glm::scale(model, t.scale);
        }

        // Recalculate the model with the updated transformation data
        shader.setMat4("model", model);
        
    //    if (reg.hasTexture[e] && reg.textures[e].useTexture) {
    //        shader.setBool("useTexture", true);
    //        glActiveTexture(GL_TEXTURE0);
    //        glBindTexture(GL_TEXTURE_2D, reg.textures[e].textureID);
    //        
    //        shader.setInt("ourTexture", 0); // Must match the uniform name in your fragment shader
    //    } else {
	//		// Crucial: Tell the shader to ignore the texture and use the solid color
	//		shader.setBool("useTexture", false);
	//	}
      
		// Texture & Color State Logic
        bool hasTex = reg.hasTexture[e] && reg.textures[e].useTexture;
        shader.setBool("useTexture", hasTex);
        shader.setBool("isVertexColor", false); // Explicitly disable vertex color

        if (hasTex) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, reg.textures[e].textureID);
            shader.setInt("ourTexture", 0);
        }
  
        // Handle Color logic
		if (reg.hasColor[e]) {
			shader.setVec3("objectColor", reg.colors[e].color);
		} else {
			// If no color component exists, force white so it isn't black
			shader.setVec3("objectColor", glm::vec3(1.0f)); 
		}


        if (reg.hasColor[e]) {
            shader.setVec3("objectColor", reg.colors[e].color);
        }

        if (renderable.mesh != nullptr) {
            shader.setBool("isVertexColor", false);

            if (reg.hasColor[e]) {
                shader.setVec3("objectColor", reg.colors[e].color);
            }
            else {
                shader.setVec3("objectColor", glm::vec3(1.0f));
            }

            // Draw the mesh                
            shader.setBool("isVertexColor", false);

            // Set color
            shader.setBool("isVertexColor", false);
            if (reg.hasColor[e]) {
                shader.setVec3("objectColor", reg.colors[e].color);
            }
            else {
                shader.setVec3("objectColor", glm::vec3(1.0f)); // Default white
            }

            // Check the entity's specific wireframe flag before drawing
            if (reg.renderables[e].isWireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }
            else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }

            // Render the mesh using its own draw function
            reg.renderables[e].mesh->draw();
        }    

        // DEBUG: Check for GL Errors immediately after draw
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cout << "OpenGL Error after draw: " << err << std::endl;
        }
    }
}

