#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath) {
        // 1. Retrieve the vertex/fragment source code from filePath
        std::string vertexCode = readFile(vertexPath);
        std::string fragmentCode = readFile(fragmentPath);
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // 2. Compile shaders
        unsigned int vertex = compileShader(vShaderCode, GL_VERTEX_SHADER);
        unsigned int fragment = compileShader(fShaderCode, GL_FRAGMENT_SHADER);

        // 3. Link Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);

        // 4. Cleanup shaders as they are linked into our program now
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    ~Shader() {
        // This runs automatically when the object is destroyed
        glDeleteProgram(ID);
    }

    void use() { glUseProgram(ID); }

    // Helper functions to send uniforms to the GPU
    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

private:
    std::string readFile(const char* filePath) {
        std::ifstream shaderFile(filePath);
        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        return shaderStream.str();
    }

    unsigned int compileShader(const char* code, GLenum type) {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &code, NULL);
        glCompileShader(shader);
        // Add error checking here for professional use!
        return shader;
    }
};

#endif