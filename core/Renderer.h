#pragma once
#include <glad/glad.h>
#include <SDL3/SDL.h>
#include "Log.h"

class Renderer {

private:
    static const int WINDOW_WIDTH = 1920;
    static const int WINDOW_HEIGHT = 1080;
    
    unsigned int vao, vbo;
    unsigned int fbo, fboTexture;

    public:
    void Init() {
        float vertices[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.0f,  0.5f, 0.0f
        };

        Logger::Log("Render initialization");

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);        

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Create texture to render to
        glGenTextures(1, &fboTexture);
        glBindTexture(GL_TEXTURE_2D, fboTexture);

        // Initial size (can be changed later if the window resizes)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1920, 1080, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);

        // Unbind to return to default rendering
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    }

    inline int GetWindowWidth() const{
        return WINDOW_WIDTH;
    }

    inline int GetWindowHeight() const{
        return WINDOW_HEIGHT;
    }
    
    void BindFBO() { 
        glBindFramebuffer(GL_FRAMEBUFFER, fbo); 
    }

    void UnbindFBO() { 
        glBindFramebuffer(GL_FRAMEBUFFER, 0); 
    }

    unsigned int GetTextureID() { 
        return fboTexture; 
    }

    void Draw() {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void WindowResize(int width, int height){
        if(width <= 0 || height <= 0) return; // prevent invalid sizes

        glBindTexture(GL_TEXTURE_2D, fboTexture);
    }
};