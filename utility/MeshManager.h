#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "CubeBuilder.h"
#include "Mesh.h"

namespace MeshManager 
{
    // Use 'inline' for the vector so it is shared across all files
    inline std::vector<std::shared_ptr<Mesh>> meshLibrary;

    // Use 'inline' for the functions to avoid "multiple definition" errors
    inline std::shared_ptr<Mesh> CreateNewCubeMesh() {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        GetCubeData(vertices, indices); 
        
        auto newMesh = std::make_shared<Mesh>(vertices, indices);

        meshLibrary.push_back(newMesh);
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
}