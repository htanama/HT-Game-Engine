#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "utility/3DShapeBuilder.h"
#include "core/Mesh.h"
#include <SDL3_image/SDL_image.h>
#include "utility/stb_image.h"

namespace MeshManager 
{
    // Use 'inline' for the vector so it is shared across all files
    inline std::vector<std::shared_ptr<Mesh>> meshLibrary;

	// Use 'inline' for the functions to avoid "multiple definition" errors
    inline std::shared_ptr<Mesh> CreateNewCubeMesh() {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        //GetCubeData(vertices, indices); 
        GetCubeDataWithTexture(vertices, indices); 

        auto newMesh = std::make_shared<Mesh>(vertices, indices);

        meshLibrary.push_back(newMesh);
        return newMesh;
    }

	// Helper function to get a mesh from a type
	inline std::shared_ptr<Mesh> CreateMeshFromType(MeshType type) {
		std::shared_ptr<Mesh> newMesh;
		std::vector<Vertex> v;
		std::vector<unsigned int> i;

		switch(type) {
			case MeshType::Cube: 
				newMesh = CreateNewCubeMesh(); 
				break;
				
			case MeshType::Sphere:
				v.clear(); i.clear(); // Explicitly clear before passing
				GetSphereData(v, i, 0.5f, 36, 18);
				newMesh = std::make_shared<Mesh>(v, i);
				break;

			case MeshType::Cylinder:
				v.clear(); i.clear(); // Explicitly clear before passing
				GetCylinderData(v, i, 0.5f, 1.0f, 36);
				newMesh = std::make_shared<Mesh>(v, i);
				break;

			case MeshType::Capsule:
				v.clear(); i.clear(); // Explicitly clear before passing
				GetCapsuleData(v, i, 0.5f, 1.0f, 36, 18);
				newMesh = std::make_shared<Mesh>(v, i);
				break;

			case MeshType::Pyramid:
				v.clear(); i.clear(); // Explicitly clear before passing
				GetPyramidData(v, i, 1.0f, 1.0f);
				newMesh = std::make_shared<Mesh>(v, i);
				break;
		}

		// Only add to library if creation succeeded
		if (newMesh) {
			meshLibrary.push_back(newMesh);
		}
		return newMesh;
	}

	
    inline void CleanupUnusedMeshes() {
        meshLibrary.erase(
            std::remove_if(meshLibrary.begin(), meshLibrary.end(), 
            [](const std::shared_ptr<Mesh>& m) {
                return m.use_count() <= 1; 
            }), 
            meshLibrary.end()
        );
    }
    
    // using std_image 
	inline unsigned int LoadTexture(const std::string& path) {
		stbi_set_flip_vertically_on_load(true); // OpenGL expects (0,0) at bottom-left

		int width, height, channels;
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
		if (!data) {
			std::cerr << "Failed to load texture: " << path << " - " << stbi_failure_reason() << std::endl;
			return 0;
		}

		GLenum format = GL_RGB;
		if (channels == 1) format = GL_RED;
		else if (channels == 3) format = GL_RGB;
		else if (channels == 4) format = GL_RGBA;

		unsigned int textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(data); // stb's own cleanup, not SDL_DestroySurface
		return textureID;

	}		
	

	unsigned int LoadTextureToOpenGL(const char* filePath) {
		SDL_Surface* surface = IMG_Load(filePath);
		if (!surface) return 0;

		// 1. Convert to a standard 32-bit RGBA format that OpenGL expects
		SDL_Surface* formattedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
		SDL_DestroySurface(surface); // Free original

		unsigned int textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// 2. Upload the extracted pixel data to OpenGL
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, formattedSurface->w, formattedSurface->h, 0, 
					 GL_RGBA, GL_UNSIGNED_BYTE, formattedSurface->pixels);
		
		// 3. Ensure filtering is set, or the texture won't display[cite: 1]
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		SDL_DestroySurface(formattedSurface);
		return textureID;
	}
	
	// Inside MeshManager
	static void RemoveTexture(unsigned int texID) {
		if (texID != 0) {
		    glDeleteTextures(1, &texID); // Free the GPU memory
		    Logger::Log("GPU Texture " + std::to_string(texID) + " deleted.");
		}
	}
    
}
