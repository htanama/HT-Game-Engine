#pragma once
#include <vector>
#include "Components.h"

using Entity = size_t; // Alias for entity IDs

struct Velocity{
    glm::vec3 value{0.0f};
};

class Registry {
public:
    std::vector<Transform> transforms;
    std::vector<Renderable> renderables;
    std::vector<Velocity> velocities;

    // We use a simple way to track which entity has which componetes. 
    std::vector<bool> hasTransform;
    std::vector<bool> hasRenderable;
    std::vector<bool> hasVelocity;

    Entity CreateEntity(){
        Entity id = hasTransform.size(); // New entity ID is the current size of the component arrays
        
        transforms.push_back({}); // Add default Transform
        renderables.push_back({}); // Add default Renderable
        velocities.push_back({}); // Add default Velocity
        
        hasTransform.push_back(false); // Initially, the entity has no components
        hasRenderable.push_back(false); // Initially, the entity has no components
        hasVelocity.push_back(false); // Initially, the entity has no components        

        return id;
    }

    // Helper function to check if an entity has a specific component
    void AddVelocity(Entity e, glm::vec3 vel){
        velocities[e] = {vel};
        hasVelocity[e] = true;
    }
};