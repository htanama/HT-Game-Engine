#version 460 core

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // Right-to-left linear algebra matrix transformation pipeline
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}