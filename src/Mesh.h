#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <iostream>

class Mesh {
public:
    unsigned int VAO, VBO;
    int vertexCount;

    // 1. Constructor: Allocates GPU memory and uploads data
    Mesh(float* vertices, int size) {
        // size is the total number of floats in the array
        vertexCount = size / 3; 

        // Generate buffers for Vertex Array Object and Vertex Buffer Object        
        // Vertex Array Object
        glGenVertexArrays(1, &VAO);

        // Vertex Buffer Object
        glGenBuffers(1, &VBO);

        // Bind VAO first
        glBindVertexArray(VAO);

        // Bind and upload VBO data. 
        // record the configuration of how the vertex data is laid out in memory (vertex attributes)
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // Note: GL_STATIC_DRAW tells the GPU that we will set this data once and draw it 
        // many times, which optimizes it for extreme rendering speeds inside the card.
        glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), vertices, GL_STATIC_DRAW);

        // Define the Memory Layout Link (Vertex Attributes)
        // Tells the GPU: "Hey, take data location 0, look at 3 floats at a time, 
        // and the gap between each point is 3 times the size of a float."
        // Define attribute pointers (Layout: 0, Size: 3 floats, Type: float)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Unbind to prevent accidental modifications
        glBindVertexArray(0);
        
        std::cout << "Mesh initialized with " << vertexCount << " vertices." << std::endl;
    }

    // 2. Draw: Binds the VAO and issues the draw command
    void draw() {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);
    }

    // 3. Destructor: Prevents memory leaks by freeing GPU resources
    ~Mesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        std::cout << "Mesh resources freed from GPU." << std::endl;
    }
};

#endif