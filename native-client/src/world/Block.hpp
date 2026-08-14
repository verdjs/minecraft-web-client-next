#pragma once
#include <cstdint>
#include <string>

namespace mc {

enum class BlockType : uint16_t {
    Air = 0,
    Stone = 1,
    Grass = 2,
    Dirt = 3,
    Cobblestone = 4,
    Planks = 5,
    Bedrock = 7,
    Water = 9,
    Lava = 11,
    Sand = 12,
    Gravel = 13,
    GoldOre = 14,
    IronOre = 15,
    CoalOre = 16,
    Log = 17,
    Leaves = 18,
    Glass = 20,
    DiamondOre = 56,
    Chest = 54,
    CraftingTable = 58,
    Deepslate = 100
};

enum Direction : uint8_t {
    Dir_Up = 0,
    Dir_Down = 1,
    Dir_North = 2,
    Dir_South = 3,
    Dir_West = 4,
    Dir_East = 5
};

struct BlockInfo {
    bool isSolid{true};
    bool isTransparent{false};
    uint8_t lightEmission{0};

    // Atlas UV coordinates (column, row in a 16x16 atlas)
    uint8_t topTexture{0};
    uint8_t bottomTexture{0};
    uint8_t sideTexture{0};
};

inline BlockInfo getBlockInfo(BlockType type) {
    switch (type) {
        case BlockType::Air:
            return { false, true, 0, 0, 0, 0 };
        case BlockType::Water:
            return { false, true, 0, 14, 14, 14 };
        case BlockType::Glass:
            return { true, true, 0, 49, 49, 49 };
        case BlockType::Grass:
            return { true, false, 0, 0, 2, 3 }; // Top: grass, bottom: dirt, side: grass_side
        case BlockType::Dirt:
            return { true, false, 0, 2, 2, 2 };
        case BlockType::Stone:
            return { true, false, 0, 1, 1, 1 };
        case BlockType::Cobblestone:
            return { true, false, 0, 16, 16, 16 };
        case BlockType::Planks:
            return { true, false, 0, 4, 4, 4 };
        case BlockType::Log:
            return { true, false, 0, 21, 21, 20 };
        case BlockType::Leaves:
            return { true, true, 0, 52, 52, 52 };
        case BlockType::Bedrock:
            return { true, false, 0, 17, 17, 17 };
        default:
            return { true, false, 0, 1, 1, 1 };
    }
}

} // namespace mc
