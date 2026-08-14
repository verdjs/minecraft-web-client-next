#pragma once
#include <vector>
#include <array>
#include <memory>
#include <atomic>
#include "Block.hpp"
#include <glad/glad.h>

namespace mc {

constexpr int CHUNK_SIZE_X = 16;
constexpr int CHUNK_SIZE_Z = 16;
constexpr int SECTION_HEIGHT = 16;
constexpr int CHUNK_SECTIONS = 24; // 384 blocks total height (1.20+ Minecraft standard)
constexpr int CHUNK_MIN_Y = -64;
constexpr int CHUNK_HEIGHT = CHUNK_SECTIONS * SECTION_HEIGHT;

struct BlockVertex {
    float x, y, z;
    float u, v;
    float normalX, normalY, normalZ;
    float ao; // Ambient occlusion factor (0.0 to 1.0)
    float light; // Sky & block light factor
};

struct ChunkMesh {
    GLuint vao{0};
    GLuint vbo{0};
    GLuint ebo{0};
    size_t indexCount{0};

    void upload(const std::vector<BlockVertex>& vertices, const std::vector<uint32_t>& indices);
    void render() const;
    void destroy();
};

class ChunkSection {
public:
    std::array<BlockType, CHUNK_SIZE_X * SECTION_HEIGHT * CHUNK_SIZE_Z> blocks{};
    std::array<uint8_t, CHUNK_SIZE_X * SECTION_HEIGHT * CHUNK_SIZE_Z> blockLight{};
    std::array<uint8_t, CHUNK_SIZE_X * SECTION_HEIGHT * CHUNK_SIZE_Z> skyLight{};

    ChunkSection();

    BlockType getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType type);
};

class Chunk {
public:
    int chunkX{0};
    int chunkZ{0};
    std::array<std::unique_ptr<ChunkSection>, CHUNK_SECTIONS> sections;
    ChunkMesh mesh;
    std::atomic<bool> isMeshDirty{true};
    std::atomic<bool> isMeshing{false};

    Chunk(int cx, int cz);
    ~Chunk();

    BlockType getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType type);

    int getBlockWorldX(int localX) const { return chunkX * CHUNK_SIZE_X + localX; }
    int getBlockWorldZ(int localZ) const { return chunkZ * CHUNK_SIZE_Z + localZ; }
};

} // namespace mc
