#include "Chunk.hpp"

namespace mc {

void ChunkMesh::upload(const std::vector<BlockVertex>& vertices, const std::vector<uint32_t>& indices) {
    if (vao == 0) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
    }

    indexCount = indices.size();
    if (indexCount == 0) return;

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(BlockVertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Position (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BlockVertex), (void*)offsetof(BlockVertex, x));

    // UV (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(BlockVertex), (void*)offsetof(BlockVertex, u));

    // Normal (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(BlockVertex), (void*)offsetof(BlockVertex, normalX));

    // AO (location 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(BlockVertex), (void*)offsetof(BlockVertex, ao));

    // Light (location 4)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(BlockVertex), (void*)offsetof(BlockVertex, light));

    glBindVertexArray(0);
}

void ChunkMesh::render() const {
    if (vao == 0 || indexCount == 0) return;
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void ChunkMesh::destroy() {
    if (ebo) glDeleteBuffers(1, &ebo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    vao = vbo = ebo = 0;
    indexCount = 0;
}

ChunkSection::ChunkSection() {
    blocks.fill(BlockType::Air);
    blockLight.fill(0);
    skyLight.fill(15);
}

BlockType ChunkSection::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= SECTION_HEIGHT || z < 0 || z >= CHUNK_SIZE_Z) {
        return BlockType::Air;
    }
    return blocks[y * CHUNK_SIZE_X * CHUNK_SIZE_Z + z * CHUNK_SIZE_X + x];
}

void ChunkSection::setBlock(int x, int y, int z, BlockType type) {
    if (x >= 0 && x < CHUNK_SIZE_X && y >= 0 && y < SECTION_HEIGHT && z >= 0 && z < CHUNK_SIZE_Z) {
        blocks[y * CHUNK_SIZE_X * CHUNK_SIZE_Z + z * CHUNK_SIZE_X + x] = type;
    }
}

Chunk::Chunk(int cx, int cz) : chunkX(cx), chunkZ(cz) {
    for (int i = 0; i < CHUNK_SECTIONS; ++i) {
        sections[i] = std::make_unique<ChunkSection>();
    }
}

Chunk::~Chunk() {
    mesh.destroy();
}

BlockType Chunk::getBlock(int x, int y, int z) const {
    int sectionIdx = (y - CHUNK_MIN_Y) / SECTION_HEIGHT;
    if (sectionIdx < 0 || sectionIdx >= CHUNK_SECTIONS) return BlockType::Air;
    int localY = (y - CHUNK_MIN_Y) % SECTION_HEIGHT;
    return sections[sectionIdx]->getBlock(x, localY, z);
}

void Chunk::setBlock(int x, int y, int z, BlockType type) {
    int sectionIdx = (y - CHUNK_MIN_Y) / SECTION_HEIGHT;
    if (sectionIdx < 0 || sectionIdx >= CHUNK_SECTIONS) return;
    int localY = (y - CHUNK_MIN_Y) % SECTION_HEIGHT;
    sections[sectionIdx]->setBlock(x, localY, z, type);
    isMeshDirty = true;
}

} // namespace mc
