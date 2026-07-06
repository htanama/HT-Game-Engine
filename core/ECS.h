#pragma once
#include <vector>
#include <string>
#include "Components.h"
#include "utility/MeshManager.h"

using Entity = size_t; // Alias for entity IDs

// The Registry class manages all entities and their associated components.
class Registry 
{

private:
    size_t entityCount = 0;

public:       

    std::vector<Transform> transforms;
    std::vector<Renderable> renderables;
    std::vector<Velocity> velocities;
    std::vector<ColorComponent> colors;    
    std::vector<LifetimeComponent> lifetimes;
    std::vector<CameraComponent> cameras; // New vector to hold camera components
    std::vector<NameComponent> names;
    std::vector<TextureComponent> textures; 
    std::vector<PhysicsComponent> physics;
    std::vector<MeshComponent> meshes;

    
    // We use a simple way to track which entity has which componetes.  
    std::vector<bool> hasTransform;
    std::vector<bool> hasRenderable;
    std::vector<bool> hasVelocity;
    std::vector<bool> hasColor;    
    std::vector<bool> hasLifetime;
    std::vector<bool> hasCamera; // Track which entities have a CameraComponent
    std::vector<bool> hasName;
    std::vector<bool> hasTexture;           
	std::vector<bool> hasPhysics;
    std::vector<bool> hasMesh;

    Entity CreateEntity(){        
        // New entity ID is the current size of the component arrays
        Entity id = hasTransform.size();                 
        std::string defaultName = "Object" + std::to_string(id);

        transforms.push_back({}); // Add default Transform
        renderables.push_back({}); // Add default Renderable
        velocities.push_back({}); // Add default Velocity
        colors.push_back({}); // Add default ColorComponent        
        lifetimes.push_back({}); // Add default LifetimeComponent
        names.push_back({defaultName});
        textures.push_back({});
		cameras.push_back({}); // we need to limit how many camera ?		
    	physics.push_back({});
        meshes.push_back({});

        hasTransform.push_back(false); // Initially, the entity has no components
        hasRenderable.push_back(false); // Initially, the entity has no components
        hasVelocity.push_back(false); // Initially, the entity has no components
        hasColor.push_back(false); // Initially, the entity has no components        
        hasLifetime.push_back(false); // Initially, the entity has no components
        hasName.push_back(true);
        hasTexture.push_back(false);
		hasCamera.push_back(false);
        hasPhysics.push_back(false);
        hasMesh.push_back(false);

        ++entityCount;
        return id;
    }
    
    // Add to Registry class in ECS.h
    Entity CopyEntity(Entity source) {
        Entity newEnt = CreateEntity();

        // Copy components if the source has them
        if (hasTransform[source]) AddTransform(newEnt, transforms[source]);
        if (hasRenderable[source]) AddRenderable(newEnt, renderables[source]);        
        if (hasColor[source]) AddColor(newEnt, colors[source]);        
        if (hasLifetime[source]) AddLifetime(newEnt, lifetimes[source]);
        if (hasTexture[source]) AddTexture(newEnt, textures[source]);
        if (hasPhysics[source]) AddPhysics(newEnt, physics[source]);
        if (hasVelocity[source]) AddVelocity(newEnt, velocities[source].value);
        if (hasMesh[source]) AddMeshComponent(newEnt, meshes[source]);

        // Copy the name and append "_copy"
        if (hasName[source]) {
            names[newEnt].name = names[source].name + "_copy";
        }

        return newEnt;
    }

    // Physically removes all "dead" entities from memory
    void PerformCleanup() {
        for (int i = (int)hasTransform.size() - 1; i >= 0; --i) {
            // Check a flag (like hasName) to see if the entity is truly gone
            if (!hasName[i]) { 
                names.erase(names.begin() + i);
                transforms.erase(transforms.begin() + i);
                renderables.erase(renderables.begin() + i);
                velocities.erase(velocities.begin() + i);
                colors.erase(colors.begin() + i);                
                lifetimes.erase(lifetimes.begin() + i);
                cameras.erase(cameras.begin() + i);
                textures.erase(textures.begin() + i);                
                meshes.erase(meshes.begin() + i); 

                hasName.erase(hasName.begin() + i);
                hasTransform.erase(hasTransform.begin() + i);
                hasRenderable.erase(hasRenderable.begin() + i);
                hasVelocity.erase(hasVelocity.begin() + i);
                hasColor.erase(hasColor.begin() + i);
                hasLifetime.erase(hasLifetime.begin() + i);
                hasCamera.erase(hasCamera.begin() + i);
                hasTexture.erase(hasTexture.begin() + i);
                hasMesh.erase(hasMesh.begin() + i);
               
            }
        }        
    }
	
	void ClearScene() {
		// Clear all component vectors
		transforms.clear();
		renderables.clear();
		velocities.clear();
		colors.clear();		
		lifetimes.clear();
		cameras.clear();
		names.clear();
		textures.clear();
		physics.clear();
        meshes.clear();
		
		// Reset all tracking vectors
		hasTransform.clear();
		hasRenderable.clear();
		hasVelocity.clear();
		hasColor.clear();		
		hasLifetime.clear();
		hasName.clear();
		hasTexture.clear();
		hasCamera.clear();
		hasPhysics.clear();
		hasMesh.clear();

		entityCount = 0;
	}

    // Helper function to check if an entity has a specific component
    void AddTransform(Entity e, Transform t) {
        if (transforms.size() <= e) transforms.resize(e + 1);
        transforms[e] = t;
        hasTransform[e] = 1;
    }
    
    void AddVelocity(Entity e, glm::vec3 vel){
        velocities[e] = {vel};
        hasVelocity[e] = true;
    }

    void AddRenderable(Entity e, Renderable r) {
        if (renderables.size() <= e) renderables.resize(e + 1);
        renderables[e] = r;
        hasRenderable[e] = 1;
    }

    void AddColor(Entity e, ColorComponent c) {
        if (colors.size() <= e) colors.resize(e + 1);
        colors[e] = c;
        hasColor[e] = 1;
    }

    void AddLifetime(Entity e, LifetimeComponent l) {
        if (lifetimes.size() <= e) lifetimes.resize(e + 1);
        lifetimes[e] = l;
        hasLifetime[e] = 1;
    }

    void AddMeshComponent(Entity e, MeshComponent mc) {
        // Ensure the vector is large enough to prevent out-of-bounds access
        if (meshes.size() <= e) {
            meshes.resize(e + 1);
            hasMesh.resize(e + 1, false);
        }
       
        // Assign the passed-in component
        meshes[e] = mc;
        hasMesh[e] = true;

        // 3. Ensure the actual mesh data is initialized via the manager
        if (!meshes[e].mesh) {
            meshes[e].mesh = MeshManager::CreateMeshFromType(mc.type);
        }
        hasMesh[e] = true;
    }

    size_t GetEntityCount() const {
        return entityCount;
    }

    void SubtractEntityCount() 
    {
        if (entityCount > 0) {
            --entityCount;
        }
    }
    
    void AddTexture(Entity entity, TextureComponent textureComponent) {
		if (textures.size() <= entity) 
			textures.resize(entity + 1);
		
		if(textureComponent.textureID != 0) 
			textureComponent.useTexture = true;
		
		textures[entity] = textureComponent;
		hasTexture[entity] = true;
	}
	
	// Enables collision/physics for an entity. Without calling this,
    // hasPhysics[e] stays false and MovementSystem's collision check
    // against this entity will always be skipped.
    void AddPhysics(Entity e, PhysicsComponent p) {
        if (physics.size() <= e) physics.resize(e + 1);
        physics[e] = p;
        hasPhysics[e] = true;
    }	    


};
