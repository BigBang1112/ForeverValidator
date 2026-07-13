#pragma once

#include <cstdint>

enum EPlugSurfaceMaterialId : std::uint8_t {
    EPlugSurfaceMaterialId_Concrete = 0u,
    EPlugSurfaceMaterialId_Pavement = 1u,
    EPlugSurfaceMaterialId_Grass = 2u,
    EPlugSurfaceMaterialId_Ice = 3u,
    EPlugSurfaceMaterialId_Metal = 4u,
    EPlugSurfaceMaterialId_Sand = 5u,
    EPlugSurfaceMaterialId_Dirt = 6u,
    EPlugSurfaceMaterialId_Turbo = 7u,
    EPlugSurfaceMaterialId_DirtRoad = 8u,
    EPlugSurfaceMaterialId_Rubber = 9u,
    EPlugSurfaceMaterialId_SlidingRubber = 10u,
    EPlugSurfaceMaterialId_Test = 11u,
    EPlugSurfaceMaterialId_Rock = 12u,
    EPlugSurfaceMaterialId_Water = 13u,
    EPlugSurfaceMaterialId_Wood = 14u,
    EPlugSurfaceMaterialId_Danger = 15u,
    EPlugSurfaceMaterialId_Asphalt = 16u,
    EPlugSurfaceMaterialId_WetDirtRoad = 17u,
    EPlugSurfaceMaterialId_WetAsphalt = 18u,
    EPlugSurfaceMaterialId_WetPavement = 19u,
    EPlugSurfaceMaterialId_WetGrass = 20u,
    EPlugSurfaceMaterialId_Snow = 21u,
    EPlugSurfaceMaterialId_ResonantMetal = 22u,
    EPlugSurfaceMaterialId_GolfBall = 23u,
    EPlugSurfaceMaterialId_GolfWall = 24u,
    EPlugSurfaceMaterialId_GolfGround = 25u,
    EPlugSurfaceMaterialId_Turbo2 = 26u,
    EPlugSurfaceMaterialId_Bumper = 27u,
    EPlugSurfaceMaterialId_NotCollidable = 28u,
    EPlugSurfaceMaterialId_FreeWheeling = 29u,
    EPlugSurfaceMaterialId_TurboRoulette = 30u,
    EPlugSurfaceMaterialId_Count = 31u,
};

class GmLocalMaterialIndex {
public:
    constexpr GmLocalMaterialIndex() = default;

    static constexpr GmLocalMaterialIndex FromIndex(std::uint16_t index) {
        return GmLocalMaterialIndex(index);
    }

    constexpr std::uint16_t Index(void) const { return index_; }

private:
    explicit constexpr GmLocalMaterialIndex(std::uint16_t index)
            : index_(index) {}

    std::uint16_t index_ = 0u;
};
