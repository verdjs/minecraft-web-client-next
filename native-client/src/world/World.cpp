#include "World.hpp"
#include <cmath>

namespace mc {

World::World() = default;

World::~World() = default;

BlockType World::getBlock(int worldX, int worldY, int worldZ) const {
    int cx = std::floor(static_cast<float>(worldX) / CHUNK_SIZE_X);
    int cz = std::floor(static_cast<float>(worldZ) / CHUNK_SIZE_Z);

    int lx = worldX - cx * CHUNK_SIZE_X;
    int lz = worldZ - cz * CHUNK_SIZE_Z;

    std::lock_guard<std::mutex> lock(worldMutex);
    auto it = chunks.find({cx, cz});
    if (it != chunks.end() && it->second) {
        return it->second->getBlock(lx, worldY, lz);
    }
    return BlockType::Air;
}

void World::setBlock(int worldX, int worldY, int worldZ, BlockType type) {
    int cx = std::floor(static_cast<float>(worldX) / CHUNK_SIZE_X);
    int cz = std::floor(static_cast<float>(worldZ) / CHUNK_SIZE_Z);

    int lx = worldX - cx * CHUNK_SIZE_X;
    int lz = worldZ - cz * CHUNK_SIZE_Z;

    std::lock_guard<std::mutex> lock(worldMutex);
    auto it = chunks.find({cx, cz});
    if (it != chunks.end() && it->second) {
        it->second->setBlock(lx, worldY, lz, type);
    }
}

Chunk* World::getChunk(int chunkX, int chunkZ) const {
    std::lock_guard<std::mutex> lock(worldMutex);
    auto it = chunks.find({chunkX, chunkZ});
    if (it != chunks.end()) return it->second.get();
    return nullptr;
}

Chunk* World::getOrCreateChunk(int chunkX, int chunkZ) {
    std::lock_guard<std::mutex> lock(worldMutex);
    auto it = chunks.find({chunkX, chunkZ});
    if (it != chunks.end()) return it->second.get();

    auto chunk = std::make_unique<Chunk>(chunkX, chunkZ);
    Chunk* ptr = chunk.get();
    chunks[{chunkX, chunkZ}] = std::move(chunk);
    return ptr;
}

void World::removeChunk(int chunkX, int chunkZ) {
    std::lock_guard<std::mutex> lock(worldMutex);
    chunks.erase({chunkX, chunkZ});
}

void World::update(const Vec3& playerPos, ThreadPool& threadPool) {
    // 1. Upload ready meshes to GPU
    {
        std::lock_guard<std::mutex> lock(pendingMeshesMutex);
        for (const auto& meshData : pendingMeshes) {
            std::lock_guard<std::mutex> wlock(worldMutex);
            auto it = chunks.find({meshData.chunkX, meshData.chunkZ});
            if (it != chunks.end() && it->second) {
                it->second->mesh.upload(meshData.vertices, meshData.indices);
                it->second->isMeshing = false;
            }
        }
        pendingMeshes.clear();
    }

    // 2. Schedule dirty chunks for background meshing on multi-core CPU workers
    std::lock_guard<std::mutex> lock(worldMutex);
    for (auto& [coord, chunk] : chunks) {
        if (chunk->isMeshDirty.load() && !chunk->isMeshing.load()) {
            chunk->isMeshDirty = false;
            chunk->isMeshing = true;

            // Capture raw pointer and coordinates
            Chunk* c = chunk.get();
            threadPool.enqueue([this, c]() {
                MeshData data = ChunkMesher::buildChunkMesh(*c, *this);
                std::lock_guard<std::mutex> mlock(pendingMeshesMutex);
                pendingMeshes.push_back(std::move(data));
            });
        }
    }
}

void World::render(const Mat4& viewProjMatrix) {
    std::lock_guard<std::mutex> lock(worldMutex);
    for (const auto& [coord, chunk] : chunks) {
        chunk->mesh.render();
    }
}

void World::generateTestWorld(int radiusChunks) {
    for (int cx = -radiusChunks; cx <= radiusChunks; ++cx) {
        for (int cz = -radiusChunks; cz <= radiusChunks; ++cz) {
            Chunk* chunk = getOrCreateChunk(cx, cz);
            for (int x = 0; x < CHUNK_SIZE_X; ++x) {
                for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
                    int wx = cx * CHUNK_SIZE_X + x;
                    int wz = cz * CHUNK_SIZE_Z + z;

                    // Simple undulating terrain height function
                    float heightF = 64.0f + std::sin(wx * 0.05f) * 8.0f + std::cos(wz * 0.05f) * 8.0f;
                    int groundY = static_cast<int>(heightF);

                    // Bedrock
                    chunk->setBlock(x, CHUNK_MIN_Y, z, BlockType::Bedrock);

                    // Stone & Deepslate
                    for (int y = CHUNK_MIN_Y + 1; y < 0; ++y) {
                        chunk->setBlock(x, y, z, BlockType::Deepslate);
                    }
                    for (int y = 0; y < groundY - 3; ++y) {
                        chunk->setBlock(x, y, z, BlockType::Stone);
                    }
                    // Dirt
                    for (int y = groundY - 3; y < groundY; ++y) {
                        chunk->setBlock(x, y, z, BlockType::Dirt);
                    }
                    // Grass top
                    chunk->setBlock(x, groundY, z, BlockType::Grass);

                    // Trees
                    if ((wx % 14 == 0) && (wz % 14 == 0) && groundY > 50) {
                        for (int ty = 1; ty <= 5; ++ty) {
                            chunk->setBlock(x, groundY + ty, z, BlockType::Log);
                        }
                        for (int lx = -2; lx <= 2; ++lx) {
                            for (int lz = -2; lz <= 2; ++lz) {
                                for (int ly = 4; ly <= 6; ++ly) {
                                    if (std::abs(lx) == 2 && std::abs(lz) == 2 && ly == 6) continue;
                                    setBlock(wx + lx, groundY + ly, wz + lz, BlockType::Leaves);
                                }
                            }
                        }
                    }
                }
            }
            chunk->isMeshDirty = true;
        }
    }
}

} // namespace mc
