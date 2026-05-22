#pragma once
#include <glm/glm.hpp>                   // Core GLM functionality (vec3, mat4, etc.)
#include <memory>
#include "Mesh.h"

struct Transform{
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
};

// The Renderable component tells the RenderSystem which mesh to draw for this entity, 
// and what color to use if not using vertex colors.
struct Renderable {
    Mesh* mesh = nullptr; // Pointer to the mesh to render
    glm::vec3 color = glm::vec3(1.0f); // Default color (white)
    bool isVertexColor = false; // Whether to use vertex colors or a default shader color
};

struct Velocity{
    glm::vec3 value{0.0f};
};

struct ColorComponent {
    glm::vec3 color;
};

struct RotationComponent {
    float angle; // How much it rotates    
    glm::vec3 axis; // Usually glm::vec3(0.0f, 1.0f, 0.0f) for Y-axis rotation
    float speed; // How fast it rotates (degrees per second)
};

struct LifetimeComponent {
    float remainingTime = 60.0f; // 60 seconds
};