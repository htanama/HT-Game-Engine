#pragma once
#include <glad/glad.h>

#include "Log.h"
#include "Shader.h"
#include "Mesh.h"

class Renderer {

private:
    static const int WINDOW_WIDTH = 1920;
    static const int WINDOW_HEIGHT = 1080;
   
public:
    int currentWindowWidth = WINDOW_WIDTH;
    int currentWindowHeight = WINDOW_HEIGHT;

    inline int GetWindowWidth() const{
        return WINDOW_WIDTH;
    }

    inline int GetWindowHeight() const{
        return WINDOW_HEIGHT;
    }
    
    void UnbindFBO() { 
        glBindFramebuffer(GL_FRAMEBUFFER, 0); 
    }

    void DrawClearScreen(float red, float green, float blue, float alpha) const{
        glClearColor(red, green, blue, alpha);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // Drawing a simple non-indexed object (like a single triangle)
    void Draw(const Mesh& mesh, const Shader& shader) {
        shader.use();
        glBindVertexArray(mesh.Vao);
        glDrawArrays(GL_TRIANGLES, 0,(GLsizei)mesh.vertices.size()); 
        glBindVertexArray(0);
    }

    // Drawing a complex object (like a Cube)
    void DrawIndexed(const Mesh& mesh, const Shader& shader) {
        shader.use();
        glBindVertexArray(mesh.Vao);
        
        glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indices.size(), GL_UNSIGNED_INT, 0);
        
        glBindVertexArray(0);
    }

    void WindowResize(int width, int height){
        if(width <= 0 || height <= 0) return; // prevent invalid sizes        
    }
};