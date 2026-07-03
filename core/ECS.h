#pragma once
#include <vector>
#include <string>
#include "Components.h"

using Entity = size_t; // Alias for entity IDs

// The Registry class manages all entities and their associated components.
class Registry 
{

private:
    size_t entityCount = 0;

public:       

    std::vector<Transform> transforms;
    std::vector<Renderable> renderables;
    std::vector<Velocity> velocities;
    std::vector<ColorComponent> colors;
    std::vector<RotationComponent> rotations;
    std::vector<LifetimeComponent> lifetimes;
    std::vector<CameraComponent> cameras; // New vector to hold camera components
    std::vector<NameComponent> names;
    std::vector<TextureComponent> textures; // Storage for the component
    std::vector<bool> hasTexture;           // Tracker for the component
    
    
    // We use a simple way to track which entity has which componetes.  
    std::vector<bool> hasTransform;
    std::vector<bool> hasRenderable;
    std::vector<bool> hasVelocity;
    std::vector<bool> hasColor;
    std::vector<bool> hasRotation;
    std::vector<bool> hasLifetime;
    std::vector<bool> hasCamera; // Track which entities have a CameraComponent
    std::vector<bool> hasName;
    
    Entity CreateEntity(){        
        // New entity ID is the current size of the component arrays
        Entity id = hasTransform.size();                 
        std::string defaultName = "Object" + std::to_string(id);

        transforms.push_back({}); // Add default Transform
        renderables.push_back({}); // Add default Renderable
        velocities.push_back({}); // Add default Velocity
        colors.push_back({}); // Add default ColorComponent
        rotations.push_back({}); // Add default RotationComponent
        lifetimes.push_back({}); // Add default LifetimeComponent
        names.push_back({defaultName});
        textures.push_back({});

        hasTransform.push_back(false); // Initially, the entity has no components
        hasRenderable.push_back(false); // Initially, the entity has no components
        hasVelocity.push_back(false); // Initially, the entity has no components
        hasColor.push_back(false); // Initially, the entity has no components
        hasRotation.push_back(false); // Initially, the entity has no components
        hasLifetime.push_back(false); // Initially, the entity has no components
        hasName.push_back(true);
        hasTexture.push_back(false);
        
        ++entityCount;
        return id;
    }

    // Physically removes all "dead" entities from memory
    void PerformCleanup() {
        for (int i = (int)hasTransform.size() - 1; i >= 0; --i) {
            // Check a flag (like hasName) to see if the entity is truly gone
            if (!hasName[i]) { 
                names.erase(names.begin() + i);
                transforms.erase(transforms.begin() + i);
                renderables.erase(renderables.begin() + i);
                velocities.erase(velocities.begin() + i);
                colors.erase(colors.begin() + i);
                rotations.erase(rotations.begin() + i);
                lifetimes.erase(lifetimes.begin() + i);
                cameras.erase(cameras.begin() + i);
                textures.erase(textures.begin() + i);
                
                hasName.erase(hasName.begin() + i);
                hasTransform.erase(hasTransform.begin() + i);
                hasRenderable.erase(hasRenderable.begin() + i);
                hasVelocity.erase(hasVelocity.begin() + i);
                hasColor.erase(hasColor.begin() + i);
                hasRotation.erase(hasRotation.begin() + i);
                hasLifetime.erase(hasLifetime.begin() + i);
                hasCamera.erase(hasCamera.begin() + i);
                hasTexture.erase(hasTexture.begin() + i);
               
            }
        }        
    }

    // Helper function to check if an entity has a specific component
    void AddTransform(Entity e, Transform t) {
        if (transforms.size() <= e) transforms.resize(e + 1);
        transforms[e] = t;
        hasTransform[e] = 1;
    }
    
    void AddVelocity(Entity e, glm::vec3 vel){
        velocities[e] = {vel};
        hasVelocity[e] = true;
    }

    void AddRenderable(Entity e, Renderable r) {
        if (renderables.size() <= e) renderables.resize(e + 1);
        renderables[e] = r;
        hasRenderable[e] = 1;
    }

    void AddColor(Entity e, ColorComponent c) {
        if (colors.size() <= e) colors.resize(e + 1);
        colors[e] = c;
        hasColor[e] = 1;
    }

    void AddRotation(Entity e, RotationComponent r) {
        if (rotations.size() <= e) rotations.resize(e + 1);
        rotations[e] = r;
        hasRotation[e] = 1;
    }

    void AddLifetime(Entity e, LifetimeComponent l) {
        if (lifetimes.size() <= e) lifetimes.resize(e + 1);
        lifetimes[e] = l;
        hasLifetime[e] = 1;
    }

    size_t GetEntityCount() const {
        return entityCount;
    }

    void SubtractEntityCount() 
    {
        if (entityCount > 0) {
            --entityCount;
        }
    }
    
    void AddTexture(Entity e, TextureComponent t) {
		if (textures.size() <= e) textures.resize(e + 1);
		if(t.textureID != 0) t.useTexture = true;
		textures[e] = t;
		hasTexture[e] = true;
	}
    
};
