#version 460 core

in vec3 ourColor; // Received from vertex shader
out vec4 FragColor;

void main() {
    // Output our custom bright amber/orange color
    // FragColor = vec4(1.0f, 0.5f, 0.0f, 1.0f);

    FragColor = vec4(ourColor, 1.0f);
    
}