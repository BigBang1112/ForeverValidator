#include "engine/game/replay_challenge_map_input.h"
#include <limits>
#include <new>

CGameCtnReplayMapIdentifier::CGameCtnReplayMapIdentifier(
        std::string identifierName)
    : name(std::move(identifierName)) {}

const std::string &CGameCtnReplayMapIdentifier::Name() const {
    return name;
}

bool CGameCtnReplayMapIdentifier::HasName() const {
    return !name.empty();
}

bool CGameCtnReplayMapIdentifier::NameEquals(std::string_view expected) const {
    return std::string_view(name) == expected;
}

CGameCtnReplayBlockPlacementId CGameCtnReplayMapInputBlock::Id() const {
    return id;
}

const CGameCtnReplayMapIdentifier &
CGameCtnReplayMapInputBlock::Identifier() const {
    return identifier;
}

const CGameCtnReplayMapIdentifier &
CGameCtnReplayMapInputBlock::Collection() const {
    return collection;
}

ECardinalDir CGameCtnReplayMapInputBlock::Direction() const {
    return direction;
}

const GmNat3 &CGameCtnReplayMapInputBlock::Coordinate() const {
    return coordinate;
}

const BlockPlacementState &CGameCtnReplayMapInputBlock::Placement() const {
    return placement;
}

CGameCtnReplayMapInputBlock::CGameCtnReplayMapInputBlock(
        CGameCtnReplayBlockPlacementId placementId,
        CGameCtnReplayMapIdentifier blockIdentifier,
        CGameCtnReplayMapIdentifier blockCollection,
        ECardinalDir blockDirection,
        GmNat3 blockCoordinate,
        BlockPlacementState blockPlacement)
    : id(placementId),
      identifier(std::move(blockIdentifier)),
      collection(std::move(blockCollection)),
      direction(blockDirection),
      coordinate(blockCoordinate),
      placement(std::move(blockPlacement)) {}

bool CGameCtnReplayMapInput::Create(
        GmNat3 mapSize,
        CGameCtnReplayMapIdentifier mapDefaultCollection,
        CGameCtnReplayMapIdentifier mapDecorationMood,
        CGameCtnReplayMapIdentifier mapDecorationCollection,
        CGameCtnReplayMapIdentifier mapDecorationAuthor,
        std::vector<CGameCtnReplayMapInputBlock> mapBlocks,
        CGameCtnReplayMapInput *out) {
    if (out == nullptr) {
        return false;
    }
    try {
        CGameCtnReplayMapInput input;
        input.size = mapSize;
        input.defaultCollection = std::move(mapDefaultCollection);
        input.decorationMood = std::move(mapDecorationMood);
        input.decorationCollection = std::move(mapDecorationCollection);
        input.decorationAuthor = std::move(mapDecorationAuthor);
        input.blocks = std::move(mapBlocks);
        *out = std::move(input);
    } catch (const std::bad_alloc &) {
        return false;
    }
    return true;
}

void CGameCtnReplayMapInput::Clear() {
    size = {};
    defaultCollection = {};
    decorationMood = {};
    decorationCollection = {};
    decorationAuthor = {};
    blocks.clear();
}

const GmNat3 &CGameCtnReplayMapInput::Size() const {
    return size;
}

u32 CGameCtnReplayMapInput::BlockCount() const {
    return blocks.size() <= std::numeric_limits<u32>::max()
            ? static_cast<u32>(blocks.size())
            : std::numeric_limits<u32>::max();
}

bool CGameCtnReplayMapInput::Empty() const {
    return blocks.empty();
}

const std::vector<CGameCtnReplayMapInputBlock> &
CGameCtnReplayMapInput::Blocks() const {
    return blocks;
}

const CGameCtnReplayMapInputBlock *
CGameCtnReplayMapInput::BlockAt(u32 index) const {
    return index < blocks.size() ? &blocks[index] : nullptr;
}

const CGameCtnReplayMapInputBlock *
CGameCtnReplayMapInput::FindBlock(CGameCtnReplayBlockPlacementId id) const {
    for (const CGameCtnReplayMapInputBlock &block : blocks) {
        if (block.Id() == id) {
            return &block;
        }
    }
    return nullptr;
}

const std::string &CGameCtnReplayMapInput::DefaultCollectionName() const {
    if (defaultCollection.HasName()) {
        return defaultCollection.Name();
    }
    for (const CGameCtnReplayMapInputBlock &block : blocks) {
        if (block.Collection().HasName()) {
            return block.Collection().Name();
        }
    }
    static const std::string empty;
    return empty;
}

const CGameCtnReplayMapIdentifier &
CGameCtnReplayMapInput::DecorationMood() const {
    return decorationMood;
}

const CGameCtnReplayMapIdentifier &
CGameCtnReplayMapInput::DecorationCollection() const {
    return decorationCollection;
}

const CGameCtnReplayMapIdentifier &
CGameCtnReplayMapInput::DecorationAuthor() const {
    return decorationAuthor;
}

bool ReplayMapAuthoredCoordinatesFit(
        const CGameCtnReplayMapInput &mapInput,
        const GmNat3 &mapSize) {
    if (mapSize.x == 0u || mapSize.y == 0u || mapSize.z == 0u) {
        return false;
    }
    for (const CGameCtnReplayMapInputBlock &block : mapInput.Blocks()) {
        const GmNat3 &coordinate = block.Coordinate();
        if (coordinate.x >= mapSize.x || coordinate.y >= mapSize.y ||
            coordinate.z >= mapSize.z) {
            return false;
        }
    }
    return true;
}

bool ReplayDecorationSizeMatchesMap(
        const CGameCtnReplayMapInput &mapInput,
        const GmNat3 &decorationSize) {
    const GmNat3 &serializedSize = mapInput.Size();
    return (serializedSize.x == decorationSize.x &&
            serializedSize.y == decorationSize.y &&
            serializedSize.z == decorationSize.z) ||
           ReplayMapAuthoredCoordinatesFit(mapInput, decorationSize);
}
