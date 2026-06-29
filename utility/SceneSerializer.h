#pragma once
#include "ECS.h" // Your Registry definitions
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

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
            
            if (reg.hasRenderable[i]) { 
                entity["meshName"] = "cube"; 
                // for the future to add object using file path .obj 
                //entity["meshPath"] = reg.renderables[i].meshPath;
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
        for (auto& entityData : scene["entities"]) {
            Entity e = reg.CreateEntity(); // This creates a new empty index
           
            // Load Transform if it exists
            if (entityData.contains("transform")) {
                reg.transforms[e].position.x = entityData["transform"]["x"];
                reg.transforms[e].position.y = entityData["transform"]["y"];
                reg.transforms[e].position.z = entityData["transform"]["z"];
                reg.hasTransform[e] = true;
            }

            if (entityData.contains("rotation")) {
                reg.transforms[e].rotation.x = entityData["rotation"]["x"];
                reg.transforms[e].rotation.y = entityData["rotation"]["y"];
                reg.transforms[e].rotation.z = entityData["rotation"]["z"];
            }

            if (entityData.contains("scale")) {
                reg.transforms[e].scale.x = entityData["scale"]["x"];
                reg.transforms[e].scale.y = entityData["scale"]["y"];
                reg.transforms[e].scale.z = entityData["scale"]["z"];
            }
           
            // Load Color if it exists
            if (entityData.contains("color")) {
                reg.colors[e].color.x = entityData["color"]["r"];
                reg.colors[e].color.y = entityData["color"]["g"];
                reg.colors[e].color.z = entityData["color"]["b"];
                reg.hasColor[e] = true;
            }

            // Load Name if it exists
            if (entityData.contains("name")) {
                reg.names[e].name = entityData["name"];
                reg.hasName[e] = true;
            }

            if (entityData.contains("meshName")) {
                std::string meshName = entityData["meshName"];
                
                // Re-link the pointer!
                if (meshName == "cube") {
                    reg.renderables[e].mesh = MeshManager::CreateNewCubeMesh(); // Or your cache/library
                }
                reg.hasRenderable[e] = true;
            }

        }
    }
};