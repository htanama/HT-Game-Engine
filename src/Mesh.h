#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <cstddef> // Required for offsetof() to calculate memory alignment

// Element Buffer Object (EBO) (also known as an Index Buffer) is an OpenGL buffer object that
// stores indices to reference specific vertices in a Vertex Buffer Object (VBO). 

// Struct representing a single point in 3D space with multiple attributes
struct Vertex {
    glm::vec3 position; // Location (X, Y, Z)
    glm::vec3 color;    // Tint (R, G, B)
    glm::vec3 normal;   // Direction for lighting calculations (X, Y, Z)

    // Explicitly public constructor
    Vertex(glm::vec3 p, glm::vec3 c, glm::vec3 n) 
        : position(p), color(c), normal(n) {}
};

/**
 * The Mesh class handles the abstraction of GPU memory.
 * It supports three modes:
 * 1. Simple: Raw array of floats (Position only)
 * 2. Standard: Interleaved Vertex struct (Position + Color + Normal)
 * 3. Indexed: Standard + Element Buffer Object (EBO) for optimized geometry
 */
class Mesh {
private:
    unsigned int m_VAO, m_VBO, m_EBO; // OpenGL handles for GPU memory
    int m_elementCount;               // Number of indices or vertices to draw
    bool m_isCombined;                // True if using Vertex struct (has colors/normals)
    bool m_isIndexed;                 // True if using an Index Buffer (EBO)

    // Configures how the GPU reads our interleaved Vertex struct
    void SetupAttributes() {
        // The "stride" is the byte-size of one full Vertex struct (36 bytes).
        // It tells OpenGL: "To get to the next vertex, skip 36 bytes."
        GLsizei stride = sizeof(Vertex);

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        // NOTE: If we had more vertex attributes (e.g., color, normal), we would set them up here as well.
        // Define the Memory Layout Link (Vertex Attributes)    
        // Tells the GPU: "Hey, take data location 0, look at 3 floats at a time, 
        // and the gap between each point is 3 times the size of a float."
        // Define attribute pointers (Layout: 0, Size: 3 floats, Type: float)
        // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        // glEnableVertexAttribArray(0);
        /////////////////////////////////////////////////////////////////////////////////////////////////
        
        // Location 0: Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        
        // Location 1: Color
        // offsetof(Vertex, color) tells OpenGL exactly how many bytes into the struct the color starts
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, color));
        
        // Location 2: Normal
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, normal));
    }

public:
    // --- CONSTRUCTORS ---

    // 1. Simple Position-only data
    Mesh(float* vertices, int size) : m_elementCount(size / 3), m_isCombined(false), m_isIndexed(false) {
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), vertices, GL_STATIC_DRAW);

        // Simple position setup
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        
        glBindVertexArray(0);
    }

    // 2. Standard: Interleaved Data (no indices)
    Mesh(const std::vector<Vertex>& vertices) : m_elementCount(vertices.size()), m_isCombined(true), m_isIndexed(false) {
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        SetupAttributes();
        glBindVertexArray(0);
    }

    // 3. Indexed: Combined Data + EBO (Most efficient for complex shapes)
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) 
        : m_elementCount(indices.size()), m_isCombined(true), m_isIndexed(true) {
        
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);
        
        // Upload Vertex Data
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        // Upload Index Data (The recipe for connecting vertices)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        SetupAttributes();
        glBindVertexArray(0);
    }
    
    // Cleanup GPU memory when Mesh object goes out of scope
    ~Mesh() {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        if (m_isIndexed) glDeleteBuffers(1, &m_EBO);
        std::cout << "Mesh resources freed from GPU." << std::endl;
    }
    
    // Selects the appropriate Draw function based on constructor used
    void draw() {
        glBindVertexArray(m_VAO);
        
        // Mode 2: Indexed | Mode 1: Standard Combined | Mode 0: Simple
        int mode = (m_isCombined && m_isIndexed) ? 2 : (m_isCombined ? 1 : 0);
        
        switch (mode) {
            case 2: // Indexed: Uses the EBO to draw triangles
                glDrawElements(GL_TRIANGLES, m_elementCount, GL_UNSIGNED_INT, 0); 
                break;
            case 1: // Standard: Draws based on vertex order
            case 0: // Simple drawing
            default:
                glDrawArrays(GL_TRIANGLES, 0, m_elementCount); 
                break;
        }
        glBindVertexArray(0);
    }
};

#endif