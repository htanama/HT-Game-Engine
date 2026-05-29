#version 460 core
out vec4 FragColor;

// The engine will pass the color here
uniform vec3 lineColor; 

void main() {
    FragColor = vec4(lineColor, 1.0);
}