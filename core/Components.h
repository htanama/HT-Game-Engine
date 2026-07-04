#pragma once
#include <glm/glm.hpp>                   // Core GLM functionality (vec3, mat4, etc.)
#include <memory>
#include "Mesh.h"
#include "Camera.h"

struct Transform{
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
};

struct TextureComponent {
    unsigned int textureID = 0; // OpenGL texture handle
    bool useTexture = false;    // Toggle for the shader
	std::string path;			// Source asset path for save/load (e.g. "assets/tile.png")
};

struct CameraComponent {
    Camera camera;
    bool isPlayerCamera = false; // True if this camera should be used for player view
};

struct PhysicsComponent{
	bool isEnabled = false;
	bool isPhysicsWireframe = false; // Used to toggle visual debug
	// TODO store a simplified convex hull mesh for performance

};


// The Renderable component tells the RenderSystem which mesh to draw for this entity, 
// and what color to use if not using vertex colors.
struct Renderable {
    std::shared_ptr<Mesh> mesh;
    std::string meshPath; // Keep track of the source file
    glm::vec3 color = glm::vec3(1.0f); // Default color (white)
    bool isVertexColor = false; // Whether to use vertex colors or a default shader color
    bool isWireframe = false;
};

struct Velocity{
    glm::vec3 value{0.0f};
};

struct ColorComponent {
    glm::vec3 color;
};

struct RotationComponent {
    float angle; // How much it rotates    
    glm::vec3 axis; // Usually glm::vec3(0.0f, 1.0f, 0.0f) for Y-axis rotation
    float speed; // How fast it rotates (degrees per second)
};

struct LifetimeComponent {
    float remainingTime = 60.0f; // 60 seconds
};

struct NameComponent {
    std::string name;
};
