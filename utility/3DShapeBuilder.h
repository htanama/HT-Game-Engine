// This is a helper file to generate cube vertex 
// and index data for different sized cubes (e.g., player, floor, projectiles).
#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "core/Mesh.h"

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

// Example sphere code https://www.songho.ca/opengl/gl_sphere.html
inline void GetSphereData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, float radius, int sectors, int stacks) {
    float sectorStep = 2.0f * 3.14159265359f / sectors;
    float stackStep = 3.14159265359f / stacks;

    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = 3.14159265359f / 2.0f - i * stackStep;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * sectorStep;

            // Position
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            glm::vec3 pos = glm::vec3(x, y, z);

            // Normal (normalized position for a sphere centered at origin)
            glm::vec3 normal = glm::normalize(pos);

            // UV Coordinates
            float u = (float)j / sectors;
            float v = (float)i / stacks;
            glm::vec2 uv = glm::vec2(u, v);

            // Default Color (White)
            glm::vec3 color = glm::vec3(1.0f);

            // Create Vertex matching Mesh.h struct constructor
            vertices.emplace_back(pos, color, normal, uv);
        }
    }

    // Index Generation
    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            indices.push_back(k1);
            indices.push_back(k2);
            indices.push_back(k1 + 1);

            indices.push_back(k1 + 1);
            indices.push_back(k2);
            indices.push_back(k2 + 1);
        }
    }
}

inline void GetCylinderData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, float radius, float height, int sectors) {
    vertices.clear();
    indices.clear();

    float halfHeight = height / 2.0f;

    // 1. Generate Vertices (Top and Bottom circles)
    for (int i = 0; i <= 1; ++i) {
        float y = (i == 0) ? -halfHeight : halfHeight;
        for (int j = 0; j <= sectors; ++j) {
            float angle = (float)j / sectors * 2.0f * 3.14159265359f;
            float x = radius * cosf(angle);
            float z = radius * sinf(angle);

            // Position and Normal
            glm::vec3 pos(x, y, z);
            glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z)); // Side normals
            glm::vec2 uv((float)j / sectors, (float)i);

            vertices.emplace_back(pos, glm::vec3(1.0f), normal, uv);
        }
    }

    // 2. Generate Indices (Side faces)
    // Connecting the top and bottom circles
    for (int j = 0; j < sectors; ++j) {
		// Change these to unsigned int to match indices.vector type
		unsigned int v1 = (unsigned int)j;
		unsigned int v2 = (unsigned int)j + 1;
		unsigned int v3 = (unsigned int)j + (sectors + 1);
		unsigned int v4 = (unsigned int)j + (sectors + 1) + 1;

        // Two triangles per segment
        indices.insert(indices.end(), {v1, v3, v2});
        indices.insert(indices.end(), {v2, v3, v4});
    }
}

inline void GetCapsuleData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, 
                          float radius, float height, int sectors, int stacks) {
    vertices.clear();
    indices.clear();

    float halfHeight = height / 2.0f;
    int halfStacks = stacks / 2;

    auto AddVertex = [&](float x, float y, float z, float u, float v, glm::vec3 normal) {
        vertices.emplace_back(glm::vec3(x, y, z), glm::vec3(1.0f), normal, glm::vec2(u, v));
    };

    // 1. Top Hemisphere: now goes POLE -> EQUATOR so it connects cleanly to the cylinder
    for (int i = 0; i <= halfStacks; ++i) {
        // i=0 -> PI/2 (pole), i=halfStacks -> 0 (equator)
        float stackAngle = (3.14159f / 2.0f) * (1.0f - (float)i / (float)halfStacks);
        float xy = radius * cosf(stackAngle);
        float y = radius * sinf(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * (2.0f * 3.14159f) / sectors;
            float x = xy * cosf(sectorAngle);
            float z = xy * sinf(sectorAngle);

            // V goes 0 (pole) -> 0.25 (equator)
            AddVertex(x, y + halfHeight, z, (float)j / sectors, (float)i / (halfStacks * 4.0f), glm::normalize(glm::vec3(x, y, z)));
        }
    } 

    // 2. Cylinder Body (unchanged - already goes top-equator -> bottom-equator)
    for (int i = 1; i < stacks; ++i) {
        float y = halfHeight - (float)i / stacks * height;
        for (int j = 0; j <= sectors; ++j) {
            float angle = j * (2.0f * 3.14159f) / sectors;
            AddVertex(radius * cosf(angle), y, radius * sinf(angle), (float)j / sectors, 0.25f + (float)i / stacks * 0.5f, glm::normalize(glm::vec3(cosf(angle), 0, sinf(angle))));
        }
    }

    // 3. Bottom Hemisphere: already correctly goes EQUATOR -> POLE, just fix UV denominator
    for (int i = 0; i <= halfStacks; ++i) {
        float stackAngle = (3.14159f / 2.0f) * (float)i / (float)halfStacks;
        float xy = radius * cosf(stackAngle);
        float y = -radius * sinf(stackAngle);
        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * (2.0f * 3.14159f) / sectors;
            float x = xy * cosf(sectorAngle);
            float z = xy * sinf(sectorAngle);
            AddVertex(x, y - halfHeight, z, (float)j / sectors, 0.75f + (float)i / (halfStacks * 4.0f), glm::normalize(glm::vec3(x, y, z)));
        }
    } 

    // 4. Indices: Connect the rings (unchanged - now correct since ring order is continuous)
    unsigned int ringVerts = sectors + 1;
    unsigned int totalRings = (halfStacks + 1) + (stacks - 1) + (halfStacks + 1);
    for (unsigned int r = 0; r < totalRings - 1; ++r) {
        for (unsigned int s = 0; s < sectors; ++s) {
            unsigned int i1 = r * ringVerts + s;
            unsigned int i2 = i1 + 1;
            unsigned int i3 = (r + 1) * ringVerts + s;
            unsigned int i4 = i3 + 1;
            indices.insert(indices.end(), {i1, i3, i2, i2, i3, i4});
        }
    }
}


inline void GetPyramidData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, float size, float height) {
    vertices.clear();
    indices.clear();

    float s = size / 2.0f;
    float h = height;

    // 1. Define Vertices
    // Apex
    glm::vec3 apex(0.0f, h, 0.0f);
    // Base corners
    glm::vec3 b1(-s, 0.0f,  s);
    glm::vec3 b2( s, 0.0f,  s);
    glm::vec3 b3( s, 0.0f, -s);
    glm::vec3 b4(-s, 0.0f, -s);

    // Add Base (as a square)
    vertices.emplace_back(b1, glm::vec3(1,1,1), glm::vec3(0,-1,0), glm::vec2(0,0));
    vertices.emplace_back(b2, glm::vec3(1,1,1), glm::vec3(0,-1,0), glm::vec2(1,0));
    vertices.emplace_back(b3, glm::vec3(1,1,1), glm::vec3(0,-1,0), glm::vec2(1,1));
    vertices.emplace_back(b4, glm::vec3(1,1,1), glm::vec3(0,-1,0), glm::vec2(0,1));

    // Add Apex
    vertices.emplace_back(apex, glm::vec3(1,1,1), glm::vec3(0,1,0), glm::vec2(0.5f, 0.5f));

    // 2. Define Indices
    // Base (two triangles)
    indices.insert(indices.end(), {0, 1, 2, 2, 3, 0});
    // Sides (four triangles)
    indices.insert(indices.end(), {4, 0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0});
}

inline void GetPlayerCubeData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    vertices.clear();
    indices.clear();

    // 0.08f * 2 = 0.16f total width/depth (The "skinny" part)
    float width = 0.08f;  
    float depth = 0.08f;  
    
    // 0.85f * 2 = 1.7f total height (The "shorter" part)
    float height = 0.50f;

    // Front Face (Z = 0.08)
    vertices.emplace_back(glm::vec3(-width, -height,  depth), glm::vec3(1,0,0), glm::vec3(0,0,1)); // 0
    vertices.emplace_back(glm::vec3( width, -height,  depth), glm::vec3(0,1,0), glm::vec3(0,0,1)); // 1
    vertices.emplace_back(glm::vec3( width,  height,  depth), glm::vec3(0,0,1), glm::vec3(0,0,1)); // 2
    vertices.emplace_back(glm::vec3(-width,  height,  depth), glm::vec3(1,1,0), glm::vec3(0,0,1)); // 3

    // Back Face (Z = -0.08)
    vertices.emplace_back(glm::vec3(-width, -height, -depth), glm::vec3(1,0,1), glm::vec3(0,0,-1)); // 4
    vertices.emplace_back(glm::vec3( width, -height, -depth), glm::vec3(0,1,1), glm::vec3(0,0,-1)); // 5
    vertices.emplace_back(glm::vec3( width,  height, -depth), glm::vec3(1,1,1), glm::vec3(0,0,-1)); // 6
    vertices.emplace_back(glm::vec3(-width,  height, -depth), glm::vec3(0,0,0), glm::vec3(0,0,-1)); // 7

    // Indices remain the same as they describe the topology of a cube
    indices = {
        0, 1, 2, 2, 3, 0,
        1, 5, 6, 6, 2, 1,
        7, 6, 5, 5, 4, 7,
        4, 0, 3, 3, 7, 4,
        4, 5, 1, 1, 0, 4,
        3, 2, 6, 6, 7, 3
    };
}


inline void GetCubeDataWithTexture(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    vertices.clear();
    indices.clear();

    std::function<void(glm::vec3, glm::vec3, glm::vec3, glm::vec3, glm::vec3)> AddFace =
        [&](glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 v4, glm::vec3 normal) {
            unsigned int start = (unsigned int)vertices.size();
            vertices.emplace_back(v1, glm::vec3(1,1,1), normal, glm::vec2(0.0f, 0.0f));
            vertices.emplace_back(v2, glm::vec3(1,1,1), normal, glm::vec2(1.0f, 0.0f));
            vertices.emplace_back(v3, glm::vec3(1,1,1), normal, glm::vec2(1.0f, 1.0f));
            vertices.emplace_back(v4, glm::vec3(1,1,1), normal, glm::vec2(0.0f, 1.0f));
            indices.insert(indices.end(), {start, start+1, start+2, start+2, start+3, start});
        };

    float w = 0.5f, h = 0.5f, d = 0.5f;

    AddFace({-w,-h, d}, { w,-h, d}, { w, h, d}, {-w, h, d}, {0, 0, 1});   // Front
    AddFace({ w,-h,-d}, {-w,-h,-d}, {-w, h,-d}, { w, h,-d}, {0, 0,-1});   // Back
    AddFace({-w, h,-d}, {-w, h, d}, { w, h, d}, { w, h,-d}, {0, 1, 0});   // Top
    AddFace({-w,-h, d}, {-w,-h,-d}, { w,-h,-d}, { w,-h, d}, {0,-1, 0});   // Bottom
    AddFace({-w,-h,-d}, {-w,-h, d}, {-w, h, d}, {-w, h,-d}, {-1,0, 0});   // Left
    AddFace({ w,-h, d}, { w,-h,-d}, { w, h,-d}, { w, h, d}, { 1,0, 0});   // Right
}


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

    // Helper to add a face with a specific normal
    auto AddFace = [&](glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 v4, glm::vec3 normal) {
        unsigned int start = (unsigned int)vertices.size();
        vertices.emplace_back(v1, glm::vec3(1,1,1), normal);
        vertices.emplace_back(v2, glm::vec3(1,1,1), normal);
        vertices.emplace_back(v3, glm::vec3(1,1,1), normal);
        vertices.emplace_back(v4, glm::vec3(1,1,1), normal);
        indices.insert(indices.end(), {start, start+1, start+2, start+2, start+3, start});
    };

    // Add 6 faces with correct normals
    AddFace({-w,-h, d}, { w,-h, d}, { w, h, d}, {-w, h, d}, {0, 0, 1});   // Front
    AddFace({ w,-h,-d}, {-w,-h,-d}, {-w, h,-d}, { w, h,-d}, {0, 0,-1});   // Back
    AddFace({-w, h,-d}, {-w, h, d}, { w, h, d}, { w, h,-d}, {0, 1, 0});   // Top
    AddFace({-w,-h, d}, {-w,-h,-d}, { w,-h,-d}, { w,-h, d}, {0,-1, 0});   // Bottom
    AddFace({-w,-h,-d}, {-w,-h, d}, {-w, h, d}, {-w, h,-d}, {-1,0, 0});  // Left
    AddFace({ w,-h, d}, { w,-h,-d}, { w, h,-d}, { w, h, d}, { 1,0, 0});  // Right
}


/* OLD GetCustomCubeData 
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
    
}*/
