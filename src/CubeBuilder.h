#include <vector>
#include <glm/glm.hpp>

#include "Mesh.h"

// 36 vertices (6 faces * 2 triangles/face * 3 vertices/triangle)
float cubeVertices[] = {
    // Front face
    -0.5f, -0.5f,  0.5f,
    0.5f, -0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
    // Back face
    -0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f,  0.5f, -0.5f,
    0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    // Left face
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    // Right face
    0.5f,  0.5f,  0.5f,
    0.5f,  0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,
    // Bottom face
    -0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f,  0.5f,
    0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,
    // Top face
    -0.5f,  0.5f, -0.5f,
    0.5f,  0.5f, -0.5f,
    0.5f,  0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f
};


// This function returns a vector of Vertices and a vector of Indices
inline void GetCubeData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    
    // Clear them first to be safe
    vertices.clear();
    indices.clear();
    
    // 8 Corners of a cube
    // Front Face (Z = 0.5)
    vertices.emplace_back(glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(1,0,0), glm::vec3(0,0,1)); // 0
    vertices.emplace_back(glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0,1,0), glm::vec3(0,0,1)); // 1
    vertices.emplace_back(glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(0,0,1), glm::vec3(0,0,1)); // 2
    vertices.emplace_back(glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(1,1,0), glm::vec3(0,0,1)); // 3

    // Back Face (Z = -0.5)
    vertices.emplace_back(glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(1,0,1), glm::vec3(0,0,-1)); // 4
    vertices.emplace_back(glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0,1,1), glm::vec3(0,0,-1)); // 5
    vertices.emplace_back(glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(1,1,1), glm::vec3(0,0,-1)); // 6
    vertices.emplace_back(glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(0,0,0), glm::vec3(0,0,-1)); // 7

    // The order of triangles
    indices = {
        0, 1, 2, 2, 3, 0, // Front
        1, 5, 6, 6, 2, 1, // Right
        7, 6, 5, 5, 4, 7, // Back
        4, 0, 3, 3, 7, 4, // Left
        4, 5, 1, 1, 0, 4, // Bottom
        3, 2, 6, 6, 7, 3  // Top
    };
}