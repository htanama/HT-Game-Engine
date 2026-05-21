#pragma once
#include <glm/glm.hpp>                   // Core GLM functionality (vec3, mat4, etc.)
#include "Mesh.h"

struct Transform{
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
};

struct Renderable {
    Mesh* mesh = nullptr; // Pointer to the mesh to render
    bool isVertexColor = false; // Whether to use vertex colors or a default shader color
};
