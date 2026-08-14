#pragma once
#include <vector>
#include <functional>
#include "Chunk.hpp"

namespace mc {

class World;

struct MeshData {
    int chunkX;
    int chunkZ;
    std::vector<BlockVertex> vertices;
    std::vector<uint32_t> indices;
};

class ChunkMesher {
public:
    static MeshData buildChunkMesh(const Chunk& chunk, const World& world);
};

} // namespace mc
