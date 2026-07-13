#include "engine/game/game_ctn_pylon_column.h"
#include "engine/game/game_ctn_pylon_column_grid.h"

void CGameCtnPylonColumnGrid::CreateColumns(
        Columns &columns,
        std::size_t count) {
    columns.reserve(count);
    for (std::size_t index = 0u; index < count; ++index) {
        columns.push_back(std::make_unique<CGameCtnPylonColumn>());
    }
}

CGameCtnPylonColumnGrid::CGameCtnPylonColumnGrid(void) = default;

CGameCtnPylonColumnGrid::~CGameCtnPylonColumnGrid(void) = default;

void CGameCtnPylonColumnGrid::Reset(
        std::uint32_t width,
        std::uint32_t depth) {
    Clear();
    width_ = width;
    depth_ = depth;

    const std::size_t northSouthCount =
            static_cast<std::size_t>(width_) *
            (static_cast<std::size_t>(depth_) + 1u);
    const std::size_t eastWestCount =
            static_cast<std::size_t>(depth_) *
            (static_cast<std::size_t>(width_) + 1u);

    CreateColumns(north_, northSouthCount);
    CreateColumns(south_, northSouthCount);
    CreateColumns(east_, eastWestCount);
    CreateColumns(west_, eastWestCount);
}

void CGameCtnPylonColumnGrid::Clear(void) {
    north_.clear();
    south_.clear();
    east_.clear();
    west_.clear();
    width_ = 0u;
    depth_ = 0u;
}

CGameCtnPylonColumn *CGameCtnPylonColumnGrid::North(GmNat3 coord) const {
    return north_[static_cast<std::size_t>(coord.x) +
                  static_cast<std::size_t>(coord.z) * width_]
            .get();
}

CGameCtnPylonColumn *CGameCtnPylonColumnGrid::South(GmNat3 coord) const {
    return south_[static_cast<std::size_t>(coord.x) +
                  static_cast<std::size_t>(coord.z) * width_]
            .get();
}

CGameCtnPylonColumn *CGameCtnPylonColumnGrid::East(GmNat3 coord) const {
    return east_[static_cast<std::size_t>(coord.x) +
                 static_cast<std::size_t>(coord.z) *
                         (static_cast<std::size_t>(width_) + 1u)]
            .get();
}

CGameCtnPylonColumn *CGameCtnPylonColumnGrid::West(GmNat3 coord) const {
    return west_[static_cast<std::size_t>(coord.x) +
                 static_cast<std::size_t>(coord.z) *
                         (static_cast<std::size_t>(width_) + 1u)]
            .get();
}

std::size_t CGameCtnPylonColumnGrid::NorthCount(void) const {
    return north_.size();
}

std::size_t CGameCtnPylonColumnGrid::SouthCount(void) const {
    return south_.size();
}

std::size_t CGameCtnPylonColumnGrid::EastCount(void) const {
    return east_.size();
}

std::size_t CGameCtnPylonColumnGrid::WestCount(void) const {
    return west_.size();
}
