#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"

class DebugRenderer {
private:
    unsigned int m_VAO, m_VBO;
    std::vector<glm::vec3> m_lines;

public:
    Shader debugShader;

    DebugRenderer() : debugShader("shaders/debug_line_vertex.glsl", "shaders/debug_line_fragment.glsl")
    {
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);

        // --- CRITICAL FIX ---
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        
        // Define attribute 0 (aPos)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        
        glBindVertexArray(0); // Unbind to keep state clean
    }

    void AddLine(glm::vec3 start, glm::vec3 end) {
        m_lines.push_back(start);
        m_lines.push_back(end);
    }

    void Render(glm::mat4 view, glm::mat4 proj, glm::vec3 color) {
        if (m_lines.empty()) return;

        glDisable(GL_DEPTH_TEST);
        debugShader.use();
        debugShader.setMat4("view", view);
        debugShader.setMat4("projection", proj);
        
        // Set the color uniform here
        debugShader.setVec3("lineColor", color);

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, m_lines.size() * sizeof(glm::vec3), m_lines.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINES, 0, (int)m_lines.size());
        
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
        m_lines.clear();
    }
};