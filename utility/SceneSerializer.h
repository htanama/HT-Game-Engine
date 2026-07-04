#pragma once
#include "core/ECS.h" // Your Registry definitions
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

// NOTE: TextureComponent (in Components.h) needs a `std::string path;` member
// alongside `textureID` and `useTexture`. Without it there's no way to reload
// the texture from disk after a restart -- textureID is just a runtime GPU
// handle that no longer means anything once the GL context is gone.

class SceneSerializer {
public:
    // Saves all active entities into a JSON file
    static void SaveScene(Registry& reg, const std::string& filepath) {
        json scene;

        // Loop through all entities
        for (size_t i = 0; i < reg.hasTransform.size(); ++i) {
            // Only save if the entity is actually active
            if (!reg.hasTransform[i] && !reg.hasRenderable[i]) continue;

            json entity;
            entity["id"] = i; // Save ID to maintain references

            // DYNAMIC SERIALIZATION: 
            // We check if the component exists in the registry, then add it to JSON
            if (reg.hasTransform[i]) {
                entity["transform"] = {
                    {"x", reg.transforms[i].position.x},
                    {"y", reg.transforms[i].position.y},
                    {"z", reg.transforms[i].position.z}
                };

                // Capture rotation
                entity["rotation"] = {
                    {"x", reg.transforms[i].rotation.x},
                    {"y", reg.transforms[i].rotation.y},
                    {"z", reg.transforms[i].rotation.z}
                };

                // Capture Scale
                entity["scale"] = {
                    {"x", reg.transforms[i].scale.x},
                    {"y", reg.transforms[i].scale.y},
                    {"z", reg.transforms[i].scale.z}
                };
                
            }
                     
            if (reg.hasColor[i]) {
                entity["color"] = {
                    {"r", reg.colors[i].color.x},
                    {"g", reg.colors[i].color.y},
                    {"b", reg.colors[i].color.z}
                };
            }

            if (reg.hasName[i]) {
                entity["name"] = reg.names[i].name;
            }
            
            
            if (reg.hasPhysics[i]) { // Ensure you have a check for the physics component
				entity["physics"] = {
					{"isEnabled", reg.physics[i].isEnabled}
				};
			}
            
            if (reg.hasRenderable[i]) { 
                entity["meshName"] = "cube"; 
                // for the future to add object using file path .obj 
                //entity["meshPath"] = reg.renderables[i].meshPath;
            }
            
			// Save texture asset path so it can be reloaded from disk later.
			// We deliberatly do NOT save textureID -- it's a runtime GPU handle
			// and gets regenerated fresh by MeshManager::LoadTexture() on load.
			if(reg.hasTexture[i]){
				entity["texture"] = {
					{"path", reg.textures[i].path},
					{"useTexture", reg.textures[i].useTexture}
				};

			}

            scene["entities"].push_back(entity);
        }

        std::ofstream file(filepath);
        file << scene.dump(4);
    }

    // Clears current registry and loads from file
    static void LoadScene(Registry& reg, const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return;

        json scene;
        file >> scene;

        // 1. Clear existing entities (Reset the Registry)
        reg.transforms.clear();
        reg.hasTransform.clear();
        reg.colors.clear();
        reg.hasColor.clear();
        reg.names.clear();
        reg.hasName.clear();
        reg.renderables.clear();
        reg.hasRenderable.clear();
        // (Add any other components you have)

        // 2. Load entities from JSON
		// auto or nlohmann::json&
        for (nlohmann::json& entityData : scene["entities"]) {
            Entity entity = reg.CreateEntity(); // This creates a new empty index
           
            // Load Transform if it exists
            if (entityData.contains("transform")) {
                reg.transforms[entity].position.x = entityData["transform"]["x"];
                reg.transforms[entity].position.y = entityData["transform"]["y"];
                reg.transforms[entity].position.z = entityData["transform"]["z"];
                reg.hasTransform[entity] = true;
            }

            if (entityData.contains("rotation")) {
                reg.transforms[entity].rotation.x = entityData["rotation"]["x"];
                reg.transforms[entity].rotation.y = entityData["rotation"]["y"];
                reg.transforms[entity].rotation.z = entityData["rotation"]["z"];
            }

            if (entityData.contains("scale")) {
                reg.transforms[entity].scale.x = entityData["scale"]["x"];
                reg.transforms[entity].scale.y = entityData["scale"]["y"];
                reg.transforms[entity].scale.z = entityData["scale"]["z"];
            }
           
            // Load Color if it exists
            if (entityData.contains("color")) {
                reg.colors[entity].color.x = entityData["color"]["r"];
                reg.colors[entity].color.y = entityData["color"]["g"];
                reg.colors[entity].color.z = entityData["color"]["b"];
                reg.hasColor[entity] = true;
            }

			if (entityData.contains("physics")) {
				reg.physics[entity].isEnabled = entityData["physics"]["isEnabled"];
				reg.hasPhysics[entity] = true; // Mark component as active
			}

            // Load Name if it exists
            if (entityData.contains("name")) {
                reg.names[entity].name = entityData["name"];
                reg.hasName[entity] = true;
            }

            if (entityData.contains("meshName")) {
                std::string meshName = entityData["meshName"];
                
                // Re-link the pointer!
                if (meshName == "cube") {
					// MeshManager::CreateNewCubeMesh()already builds vertices via 
					// GetCubeDataWithTexture(), which backes UV/texCoords into the
					// mesh itself -- so texture coordinates come back automatically,
					// no need to serialize them per-entity
                    reg.renderables[entity].mesh = MeshManager::CreateNewCubeMesh(); // Or your cache/library
                }
                reg.hasRenderable[entity] = true;
            }

			// Reload the texture from disk using the saved asset path.
			if(entityData.contains("texture")){
				TextureComponent textureComponent;
				textureComponent.path = entityData["texture"]["path"];
				textureComponent.useTexture = entityData["texture"]["useTexture"];

				if(!textureComponent.path.empty()){
					textureComponent.textureID = MeshManager::LoadTexture(textureComponent.path);
				}

				if(textureComponent.textureID == 0){
					textureComponent.useTexture = false;
				}

				reg.AddTexture(entity, textureComponent);
			
			}

        }
    }
};
