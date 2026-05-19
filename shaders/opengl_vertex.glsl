#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;  // Connects to location 1 from Mesh.h
layout (location = 2) in vec3 aNormal; // Connects to location 2 from Mesh.h

out vec3 ourColor; // Pass to fragment shader

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    ourColor = aColor; // Send the color to the fragment shader
}