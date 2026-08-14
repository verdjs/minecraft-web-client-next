#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include "Chunk.hpp"
#include "ChunkMesher.hpp"
#include "../engine/ThreadPool.hpp"
#include "../math/Math.hpp"

namespace mc {

struct ChunkCoordHash {
    size_t operator()(const std::pair<int, int>& p) const {
        return (static_cast<size_t>(p.first) * 73856093) ^ (static_cast<size_t>(p.second) * 83492791);
    }
};

class World {
public:
    World();
    ~World();

    BlockType getBlock(int worldX, int worldY, int worldZ) const;
    void setBlock(int worldX, int worldY, int worldZ, BlockType type);

    Chunk* getChunk(int chunkX, int chunkZ) const;
    Chunk* getOrCreateChunk(int chunkX, int chunkZ);
    void removeChunk(int chunkX, int chunkZ);

    void update(const Vec3& playerPos, ThreadPool& threadPool);
    void render(const Mat4& viewProjMatrix);

    void generateTestWorld(int radiusChunks = 6);

private:
    mutable std::mutex worldMutex;
    std::unordered_map<std::pair<int, int>, std::unique_ptr<Chunk>, ChunkCoordHash> chunks;

    std::mutex pendingMeshesMutex;
    std::vector<MeshData> pendingMeshes;
};

} // namespace mc
