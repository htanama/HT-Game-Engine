#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;
// Add the texture coordinate input attribute (location 3)
layout (location = 3) in vec2 aTexCoord; 

out vec3 ourColor;
out vec3 FragPos; 
// Add the output variable that matches your Fragment Shader
out vec2 TexCoord; 

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    ourColor = aColor;
    
    // Pass the texture coordinate to the fragment shader
    TexCoord = aTexCoord; 
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
