#include "engine/game/game_ctn_utils.h"
#include <cstdint>

ECardinalDir CGameCtnUtils::AddDirs(
        ECardinalDir lhs,
        ECardinalDir rhs) {
    std::uint32_t result = static_cast<std::uint32_t>(lhs) +
            static_cast<std::uint32_t>(rhs);
    if (result >= 4u) {
        result -= 4u;
    }
    return static_cast<ECardinalDir>(result);
}

ECardinalDir CGameCtnUtils::SubDirs(
        ECardinalDir lhs,
        ECardinalDir rhs) {
    const std::uint32_t lhsValue = static_cast<std::uint32_t>(lhs);
    const std::uint32_t rhsValue = static_cast<std::uint32_t>(rhs);
    const std::uint32_t adjusted = lhsValue < rhsValue
            ? lhsValue + 4u
            : lhsValue;
    return static_cast<ECardinalDir>(adjusted - rhsValue);
}

ECardinalDir CGameCtnUtils::GetOpposedDir(ECardinalDir direction) {
    std::uint32_t result = static_cast<std::uint32_t>(direction) + 2u;
    if (result >= 4u) {
        result -= 4u;
    }
    return static_cast<ECardinalDir>(result);
}

GmNat3 CGameCtnUtils::GetNeighbourCoord(
        GmNat3 coord,
        ECardinalDir direction) {
    switch (direction) {
    case ECardinalDir::North:
        ++coord.z;
        break;
    case ECardinalDir::West:
        --coord.x;
        break;
    case ECardinalDir::South:
        --coord.z;
        break;
    case ECardinalDir::East:
        ++coord.x;
        break;
    }
    return coord;
}

unsigned long CGameCtnUtils::CycleLeft8Bits(unsigned long value) {
    return ((value & 0x7ful) << 1u) |
            ((value & 0x80ul) != 0ul ? 1ul : 0ul);
}
