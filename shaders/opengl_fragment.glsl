#version 460 core

in vec3 ourColor; 
out vec4 FragColor;

uniform vec3 objectColor;
uniform bool isVertexColor;

void main() {    
    // Just output the color you pick in the Inspector
    vec3 finalColor = isVertexColor ? ourColor : objectColor;
    FragColor = vec4(finalColor, 1.0);    
}