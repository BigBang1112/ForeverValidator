#ifndef TMNF_REPLAY_CHALLENGE_MAP_INPUT_H
#define TMNF_REPLAY_CHALLENGE_MAP_INPUT_H

#include "engine/core/engine_types.h"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/game/block_placement_state.h"
#include "engine/core/gm_types.h"
class CGameCtnReplayMapIdentifier {
public:
    CGameCtnReplayMapIdentifier() = default;
    explicit CGameCtnReplayMapIdentifier(std::string name);

    const std::string &Name() const;
    bool HasName() const;
    bool NameEquals(std::string_view expected) const;

private:
    std::string name;
};

class CGameCtnReplayBlockPlacementId {
public:
    CGameCtnReplayBlockPlacementId() = default;
    explicit CGameCtnReplayBlockPlacementId(u32 ordinal) : ordinal_(ordinal) {}

    u32 Ordinal() const { return ordinal_; }
    bool operator==(const CGameCtnReplayBlockPlacementId &other) const {
        return ordinal_ == other.ordinal_;
    }
    bool operator!=(const CGameCtnReplayBlockPlacementId &other) const {
        return !(*this == other);
    }

private:
    u32 ordinal_ = 0u;
};

class CGameCtnReplayMapInputBlock {
public:
    CGameCtnReplayMapInputBlock() = default;
    CGameCtnReplayMapInputBlock(
            CGameCtnReplayBlockPlacementId id,
            CGameCtnReplayMapIdentifier identifier,
            CGameCtnReplayMapIdentifier collection,
            ECardinalDir direction,
            GmNat3 coordinate,
            BlockPlacementState placement);

    CGameCtnReplayBlockPlacementId Id() const;
    const CGameCtnReplayMapIdentifier &Identifier() const;
    const CGameCtnReplayMapIdentifier &Collection() const;
    ECardinalDir Direction() const;
    const GmNat3 &Coordinate() const;
    const BlockPlacementState &Placement() const;

private:
    CGameCtnReplayBlockPlacementId id;
    CGameCtnReplayMapIdentifier identifier;
    CGameCtnReplayMapIdentifier collection;
    ECardinalDir direction = ECardinalDir::North;
    GmNat3 coordinate{};
    BlockPlacementState placement;
};

struct CGameCtnReplayMapInput {
public:
    static bool Create(
            GmNat3 size,
            CGameCtnReplayMapIdentifier defaultCollection,
            CGameCtnReplayMapIdentifier decorationMood,
            CGameCtnReplayMapIdentifier decorationCollection,
            CGameCtnReplayMapIdentifier decorationAuthor,
            std::vector<CGameCtnReplayMapInputBlock> blocks,
            CGameCtnReplayMapInput *out);

    void Clear();

    const GmNat3 &Size() const;
    u32 BlockCount() const;
    bool Empty() const;
    const std::vector<CGameCtnReplayMapInputBlock> &Blocks() const;
    const CGameCtnReplayMapInputBlock *BlockAt(u32 index) const;
    const CGameCtnReplayMapInputBlock *FindBlock(
            CGameCtnReplayBlockPlacementId id) const;

    const std::string &DefaultCollectionName() const;
    const CGameCtnReplayMapIdentifier &DecorationMood() const;
    const CGameCtnReplayMapIdentifier &DecorationCollection() const;
    const CGameCtnReplayMapIdentifier &DecorationAuthor() const;
private:
    GmNat3 size{};
    CGameCtnReplayMapIdentifier defaultCollection;
    CGameCtnReplayMapIdentifier decorationMood;
    CGameCtnReplayMapIdentifier decorationCollection;
    CGameCtnReplayMapIdentifier decorationAuthor;
    std::vector<CGameCtnReplayMapInputBlock> blocks;
};

#endif
