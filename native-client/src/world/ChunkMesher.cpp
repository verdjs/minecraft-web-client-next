#include "ChunkMesher.hpp"
#include "World.hpp"

namespace mc {

// Texture atlas helper: returns (u, v) for a given texture index in a 16x16 atlas
static inline void getAtlasUV(uint8_t texIndex, float cornerU, float cornerV, float& outU, float& outV) {
    float col = static_cast<float>(texIndex % 16);
    float row = static_cast<float>(texIndex / 16);
    constexpr float tileSize = 1.0f / 16.0f;
    outU = (col + cornerU) * tileSize;
    outV = (row + (1.0f - cornerV)) * tileSize;
}

// Compute simple vertex Ambient Occlusion
static inline float computeAO(bool side1, bool side2, bool corner) {
    if (side1 && side2) return 0.25f;
    return 1.0f - (static_cast<float>(side1) + static_cast<float>(side2) + static_cast<float>(corner)) * 0.25f;
}

MeshData ChunkMesher::buildChunkMesh(const Chunk& chunk, const World& world) {
    MeshData meshData;
    meshData.chunkX = chunk.chunkX;
    meshData.chunkZ = chunk.chunkZ;

    uint32_t vertexOffset = 0;

    for (int y = CHUNK_MIN_Y; y < CHUNK_MIN_Y + CHUNK_HEIGHT; ++y) {
        for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
            for (int x = 0; x < CHUNK_SIZE_X; ++x) {
                BlockType type = chunk.getBlock(x, y, z);
                if (type == BlockType::Air) continue;

                int wx = chunk.getBlockWorldX(x);
                int wy = y;
                int wz = chunk.getBlockWorldZ(z);

                BlockInfo info = getBlockInfo(type);

                // Face directions: Up, Down, North, South, West, East
                // Check if neighbor is transparent or air
                bool upAir = getBlockInfo(world.getBlock(wx, wy + 1, wz)).isTransparent;
                bool downAir = getBlockInfo(world.getBlock(wx, wy - 1, wz)).isTransparent;
                bool northAir = getBlockInfo(world.getBlock(wx, wy, wz - 1)).isTransparent;
                bool southAir = getBlockInfo(world.getBlock(wx, wy, wz + 1)).isTransparent;
                bool westAir = getBlockInfo(world.getBlock(wx - 1, wy, wz)).isTransparent;
                bool eastAir = getBlockInfo(world.getBlock(wx + 1, wy, wz)).isTransparent;

                float fx = static_cast<float>(wx);
                float fy = static_cast<float>(wy);
                float fz = static_cast<float>(wz);

                // Helper to add a quad
                auto addQuad = [&](const BlockVertex& v0, const BlockVertex& v1, const BlockVertex& v2, const BlockVertex& v3) {
                    meshData.vertices.push_back(v0);
                    meshData.vertices.push_back(v1);
                    meshData.vertices.push_back(v2);
                    meshData.vertices.push_back(v3);

                    meshData.indices.push_back(vertexOffset + 0);
                    meshData.indices.push_back(vertexOffset + 1);
                    meshData.indices.push_back(vertexOffset + 2);
                    meshData.indices.push_back(vertexOffset + 2);
                    meshData.indices.push_back(vertexOffset + 3);
                    meshData.indices.push_back(vertexOffset + 0);

                    vertexOffset += 4;
                };

                // TOP FACE (+Y)
                if (upAir) {
                    float u0, v0, u1, v1, u2, v2, u3, v3;
                    getAtlasUV(info.topTexture, 0, 0, u0, v0);
                    getAtlasUV(info.topTexture, 1, 0, u1, v1);
                    getAtlasUV(info.topTexture, 1, 1, u2, v2);
                    getAtlasUV(info.topTexture, 0, 1, u3, v3);

                    addQuad(
                        {fx + 0, fy + 1, fz + 0, u0, v0, 0, 1, 0, 1.0f, 1.0f},
                        {fx + 1, fy + 1, fz + 0, u1, v1, 0, 1, 0, 1.0f, 1.0f},
                        {fx + 1, fy + 1, fz + 1, u2, v2, 0, 1, 0, 1.0f, 1.0f},
                        {fx + 0, fy + 1, fz + 1, u3, v3, 0, 1, 0, 1.0f, 1.0f}
                    );
                }

                // BOTTOM FACE (-Y)
                if (downAir) {
                    float u0, v0, u1, v1, u2, v2, u3, v3;
                    getAtlasUV(info.bottomTexture, 0, 0, u0, v0);
                    getAtlasUV(info.bottomTexture, 1, 0, u1, v1);
                    getAtlasUV(info.bottomTexture, 1, 1, u2, v2);
                    getAtlasUV(info.bottomTexture, 0, 1, u3, v3);

                    addQuad(
                        {fx + 0, fy + 0, fz + 1, u0, v0, 0, -1, 0, 0.6f, 0.6f},
                        {fx + 1, fy + 0, fz + 1, u1, v1, 0, -1, 0, 0.6f, 0.6f},
                        {fx + 1, fy + 0, fz + 0, u2, v2, 0, -1, 0, 0.6f, 0.6f},
                        {fx + 0, fy + 0, fz + 0, u3, v3, 0, -1, 0, 0.6f, 0.6f}
                    );
                }

                // NORTH FACE (-Z)
                if (northAir) {
                    float u0, v0, u1, v1, u2, v2, u3, v3;
                    getAtlasUV(info.sideTexture, 0, 0, u0, v0);
                    getAtlasUV(info.sideTexture, 1, 0, u1, v1);
                    getAtlasUV(info.sideTexture, 1, 1, u2, v2);
                    getAtlasUV(info.sideTexture, 0, 1, u3, v3);

                    addQuad(
                        {fx + 1, fy + 0, fz + 0, u0, v0, 0, 0, -1, 0.8f, 0.8f},
                        {fx + 0, fy + 0, fz + 0, u1, v1, 0, 0, -1, 0.8f, 0.8f},
                        {fx + 0, fy + 1, fz + 0, u2, v2, 0, 0, -1, 0.8f, 0.8f},
                        {fx + 1, fy + 1, fz + 0, u3, v3, 0, 0, -1, 0.8f, 0.8f}
                    );
                }

                // SOUTH FACE (+Z)
                if (southAir) {
                    float u0, v0, u1, v1, u2, v2, u3, v3;
                    getAtlasUV(info.sideTexture, 0, 0, u0, v0);
                    getAtlasUV(info.sideTexture, 1, 0, u1, v1);
                    getAtlasUV(info.sideTexture, 1, 1, u2, v2);
                    getAtlasUV(info.sideTexture, 0, 1, u3, v3);

                    addQuad(
                        {fx + 0, fy + 0, fz + 1, u0, v0, 0, 0, 1, 0.8f, 0.8f},
                        {fx + 1, fy + 0, fz + 1, u1, v1, 0, 0, 1, 0.8f, 0.8f},
                        {fx + 1, fy + 1, fz + 1, u2, v2, 0, 0, 1, 0.8f, 0.8f},
                        {fx + 0, fy + 1, fz + 1, u3, v3, 0, 0, 1, 0.8f, 0.8f}
                    );
                }

                // WEST FACE (-X)
                if (westAir) {
                    float u0, v0, u1, v1, u2, v2, u3, v3;
                    getAtlasUV(info.sideTexture, 0, 0, u0, v0);
                    getAtlasUV(info.sideTexture, 1, 0, u1, v1);
                    getAtlasUV(info.sideTexture, 1, 1, u2, v2);
                    getAtlasUV(info.sideTexture, 0, 1, u3, v3);

                    addQuad(
                        {fx + 0, fy + 0, fz + 0, u0, v0, -1, 0, 0, 0.7f, 0.7f},
                        {fx + 0, fy + 0, fz + 1, u1, v1, -1, 0, 0, 0.7f, 0.7f},
                        {fx + 0, fy + 1, fz + 1, u2, v2, -1, 0, 0, 0.7f, 0.7f},
                        {fx + 0, fy + 1, fz + 0, u3, v3, -1, 0, 0, 0.7f, 0.7f}
                    );
                }

                // EAST FACE (+X)
                if (eastAir) {
                    float u0, v0, u1, v1, u2, v2, u3, v3;
                    getAtlasUV(info.sideTexture, 0, 0, u0, v0);
                    getAtlasUV(info.sideTexture, 1, 0, u1, v1);
                    getAtlasUV(info.sideTexture, 1, 1, u2, v2);
                    getAtlasUV(info.sideTexture, 0, 1, u3, v3);

                    addQuad(
                        {fx + 1, fy + 0, fz + 1, u0, v0, 1, 0, 0, 0.7f, 0.7f},
                        {fx + 1, fy + 0, fz + 0, u1, v1, 1, 0, 0, 0.7f, 0.7f},
                        {fx + 1, fy + 1, fz + 0, u2, v2, 1, 0, 0, 0.7f, 0.7f},
                        {fx + 1, fy + 1, fz + 1, u3, v3, 1, 0, 0, 0.7f, 0.7f}
                    );
                }
            }
        }
    }

    return meshData;
}

} // namespace mc
