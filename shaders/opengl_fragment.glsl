#version 460 core

in vec3 ourColor;
in vec2 TexCoord; 

out vec4 FragColor;

uniform vec3 objectColor;
uniform bool isVertexColor;
uniform bool useTexture; 
uniform sampler2D ourTexture; 

void main() {    
    vec3 baseColor = isVertexColor ? ourColor : objectColor;
    
    // Flip the Y-coordinate to fix the upside-down texture issue
    vec2 flippedTexCoord = vec2(TexCoord.x, 1.0 - TexCoord.y);
    vec4 textureColor = texture(ourTexture, flippedTexCoord);
    
    // Apply texture if enabled, otherwise use the base color
    vec3 finalColor = useTexture ? (baseColor * textureColor.rgb) : baseColor;
    
    FragColor = vec4(finalColor, 1.0);    
}
