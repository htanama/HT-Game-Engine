#version 460 core

in vec3 ourColor; // This comes from the Vertex Shader
out vec4 FragColor;

uniform bool isVertexColor; // This is the toggle

void main() {
    /*
    if (isVertexColor) {
        FragColor = vec4(ourColor, 1.0f);
    } else {
        FragColor = vec4(1.0f, 0.5f, 0.0f, 1.0f); // Solid Orange
    }   
    */
    
    FragColor = vec4(ourColor, 1.0f);
}