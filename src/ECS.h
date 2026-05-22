#pragma once
#include <vector>
#include "Components.h"

using Entity = size_t; // Alias for entity IDs

class Registry 
{
public:
    std::vector<Transform> transforms;
    std::vector<Renderable> renderables;
    std::vector<Velocity> velocities;
    std::vector<ColorComponent> colors;
    std::vector<RotationComponent> rotations;
    std::vector<LifetimeComponent> lifetimes;

    // We use a simple way to track which entity has which componetes.  
    std::vector<bool> hasTransform;
    std::vector<bool> hasRenderable;
    std::vector<bool> hasVelocity;
    std::vector<bool> hasColor;
    std::vector<bool> hasRotation;
    std::vector<bool> hasLifetime;

    Entity CreateEntity(){
        // New entity ID is the current size of the component arrays
        Entity id = hasTransform.size(); 
        
        transforms.push_back({}); // Add default Transform
        renderables.push_back({}); // Add default Renderable
        velocities.push_back({}); // Add default Velocity
        colors.push_back({}); // Add default ColorComponent
        rotations.push_back({}); // Add default RotationComponent
        lifetimes.push_back({}); // Add default LifetimeComponent
        
        hasTransform.push_back(false); // Initially, the entity has no components
        hasRenderable.push_back(false); // Initially, the entity has no components
        hasVelocity.push_back(false); // Initially, the entity has no components
        hasColor.push_back(false); // Initially, the entity has no components
        hasRotation.push_back(false); // Initially, the entity has no components
        hasLifetime.push_back(false); // Initially, the entity has no components

        return id;
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
};