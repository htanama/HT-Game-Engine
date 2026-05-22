#version 460 core

in vec3 ourColor; 
in vec3 FragPos; 

out vec4 FragColor;

uniform vec3 objectColor;
uniform bool isVertexColor;

// A simple noise generator
float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

// Generates smooth, puffy clouds
float getCloud(vec2 uv) {
    vec2 i = floor(uv);
    vec2 f = fract(uv);
    
    // Smooth interpolation (the "puffy" part)
    vec2 u = f * f * (3.0 - 2.0 * f);
    
    // Mix noise values to get soft blobs
    return mix(mix(hash(i + vec2(0,0)), hash(i + vec2(1,0)), u.x),
               mix(hash(i + vec2(0,1)), hash(i + vec2(1,1)), u.x), u.y);
}

void main() {    
    vec3 baseColor = isVertexColor ? ourColor : objectColor;

    // Apply cloud pattern to the sky area (using FragPos.xz)
    vec2 uv = FragPos.xz * 0.2;
    float clouds = getCloud(uv);
    
    // "Puffy" threshold: increase this value to make clouds smaller/puffier
    clouds = smoothstep(0.6, 0.8, clouds);
    
    vec3 skyBase = vec3(0.53, 0.81, 0.92);
    vec3 cloudColor = vec3(1.0, 1.0, 1.0);
    
    vec3 skyFinal = mix(skyBase, cloudColor, clouds);

    // Final color: Object + Sky tint
    FragColor = vec4(mix(baseColor, skyFinal, 0.3), 1.0);
}