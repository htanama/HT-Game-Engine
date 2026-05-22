#pragma once
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


inline void GetCustomCubeData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, 
                             float width, float height, float depth) {    
    vertices.clear();
    indices.clear();

    float w = width / 2.0f;
    float h = height / 2.0f;
    float d = depth / 2.0f;

    // 8 Corners of the cube
    // Positions: Front (Z=d), Back (Z=-d)
    glm::vec3 p0(-w, -h,  d); // 0
    glm::vec3 p1( w, -h,  d); // 1
    glm::vec3 p2( w,  h,  d); // 2
    glm::vec3 p3(-w,  h,  d); // 3
    glm::vec3 p4(-w, -h, -d); // 4
    glm::vec3 p5( w, -h, -d); // 5
    glm::vec3 p6( w,  h, -d); // 6
    glm::vec3 p7(-w,  h, -d); // 7

    // Define vertices with normals (used for lighting)
    vertices.emplace_back(p0, glm::vec3(1,0,0), glm::vec3(0,0,1));
    vertices.emplace_back(p1, glm::vec3(0,1,0), glm::vec3(0,0,1));
    vertices.emplace_back(p2, glm::vec3(0,0,1), glm::vec3(0,0,1));
    vertices.emplace_back(p3, glm::vec3(1,1,0), glm::vec3(0,0,1));
    vertices.emplace_back(p4, glm::vec3(1,0,1), glm::vec3(0,0,-1));
    vertices.emplace_back(p5, glm::vec3(0,1,1), glm::vec3(0,0,-1));
    vertices.emplace_back(p6, glm::vec3(1,1,1), glm::vec3(0,0,-1));
    vertices.emplace_back(p7, glm::vec3(0,0,0), glm::vec3(0,0,-1));

    indices = {
        0, 1, 2, 2, 3, 0, // Front
        1, 5, 6, 6, 2, 1, // Right
        7, 6, 5, 5, 4, 7, // Back
        4, 0, 3, 3, 7, 4, // Left
        4, 5, 1, 1, 0, 4, // Bottom
        3, 2, 6, 6, 7, 3  // Top
    };

}