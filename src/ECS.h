#pragma once
#include <vector>
#include "Components.h"

using Entity = size_t; // Alias for entity IDs

class Registry {
public:
    std::vector<Transform> transforms;
    std::vector<Renderable> renderables;

    // We use a simple way to track which entity has which componetes. 
    std::vector<bool> hasTransform;
    std::vector<bool> hasRenderable;

    Entity CreateEntity(){
        Entity id = hasTransform.size(); // New entity ID is the current size of the component arrays
        transforms.push_back({}); // Add default Transform
        renderables.push_back({}); // Add default Renderable
        hasTransform.push_back(false); // Initially, the entity has no components
        hasRenderable.push_back(false); // Initially, the entity has no components

        return id;
    }
};