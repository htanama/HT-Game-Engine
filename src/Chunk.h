// Chunk.h
#pragma once
#include <vector>
#include <memory>
#include "Mesh.h"

struct Chunk {
    // The raw data
    uint8_t blocks[16][16][16];
    
    // The GPU representation
    std::shared_ptr<Mesh> mesh;
    
    // Helper to generate the mesh from the block data
    void GenerateMesh();
};