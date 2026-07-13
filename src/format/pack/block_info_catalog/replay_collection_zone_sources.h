#ifndef TMNF_REPLAY_COLLECTION_ZONE_SOURCES_H
#define TMNF_REPLAY_COLLECTION_ZONE_SOURCES_H

#include "engine/core/engine_types.h"
#include <stddef.h>
#include <optional>
#include <string>
#include <vector>

#include "format/archive/archive_node_reference.h"
#include "format/replay/replay_static_descriptor_limits.h"
struct BlockInfoDescriptorExternalRefs;
struct CPlugFilePack;

enum class CGameCtnReplayConstructionZoneKind : u32 {
    None = 0u,
    Flat = 1u,
    Frontier = 2u,
    Other = 3u
};

class CGameCtnReplayCollectionZoneSourceInfo {
public:
    void Clear();
    int Configure(ArchiveNodeReference sourceNode,
                  CGameCtnReplayConstructionZoneKind kind,
                  u32 loadable,
                  const char *plainPath);

    int IsConstructionZone() const;
    int IsFlat() const;
    int IsFrontier() const;
    int IdentifierFromPlainPath(char *out, size_t outSize) const;

    ArchiveNodeReference SourceNode() const;
    CGameCtnReplayConstructionZoneKind Kind() const;
    u32 IsLoadable() const;
    const char *PlainPath() const;

private:
    ArchiveNodeReference sourceNode =
            ArchiveNodeReference::Invalid();
    CGameCtnReplayConstructionZoneKind kind =
            CGameCtnReplayConstructionZoneKind::None;
    u32 loadable = 0u;
    char plainPath[CGameCtnReplayStaticDescriptorPathCapacity]{};
};

class CGameCtnReplayZoneDescriptorSourceSet {
public:
    void Clear();
    int SetZoneIdName(const char *name);
    char *ZoneIdNameArchiveBuffer();
    size_t ZoneIdNameArchiveBufferSize() const;
    const char *ZoneIdName() const;
    char *DescriptorPathArchiveBuffer(u32 slot);
    size_t DescriptorPathArchiveBufferSize() const;
    int NoteDescriptorPathIfPresent(u32 slot);
    int SetSingleDescriptorPath(const char *path);

private:
    static constexpr u32 SourcePathCount = 4u;

    u32 parsedSourceCount = 0u;
    char zoneIdName[64]{};
    char descriptorPath[SourcePathCount]
            [CGameCtnReplayStaticDescriptorPathCapacity]{};
};

class CGameCtnReplayCollectionZoneSourceRef {
public:
    int Configure(const CGameCtnReplayCollectionZoneSourceInfo &source);
    int IsFlat() const;
    int IsFrontier() const;
    int IdentifierFromPlainPath(char *out, size_t outSize) const;
    int ParseZoneDescriptorSources(const unsigned char *bytes, u32 byteCount);
    int FillCatalogDefaultSource();

    ArchiveNodeReference SourceNode() const;
    CGameCtnReplayConstructionZoneKind Kind() const;
    u32 IsLoadable() const;
    const char *PlainPath() const;

private:
    CGameCtnReplayCollectionZoneSourceInfo source;
    CGameCtnReplayZoneDescriptorSourceSet descriptorSources;
};

class CGameCtnReplayCollectionZoneSources {
public:
    void Clear();
    int CopyFrom(const CGameCtnReplayCollectionZoneSources &source);
    int LoadFromCatalogCollection(
            const CPlugFilePack *installedPack,
            const char *collectionName);
    std::optional<std::string> DefaultBaseIdentifier() const;
    CGameCtnReplayConstructionZoneKind DefaultZoneKind() const;

private:
    std::vector<CGameCtnReplayCollectionZoneSourceRef> refs;
    u32 externalRefCount = 0u;
    u32 flatExternalRefCount = 0u;
    u32 frontierExternalRefCount = 0u;
    u32 loadableExternalRefCount = 0u;
    u32 nonLoadableExternalRefCount = 0u;
    u32 bufferArchiveTag = 0u;
    u32 bufferRefCount = 0u;
    u32 bufferConstructionRefCount = 0u;
    u32 bufferFlatRefCount = 0u;
    u32 bufferFrontierRefCount = 0u;
    ArchiveNodeReference defaultZoneNode =
            ArchiveNodeReference::Invalid();
    CGameCtnReplayConstructionZoneKind defaultZoneKind =
            CGameCtnReplayConstructionZoneKind::None;

    int AppendRef(const CGameCtnReplayCollectionZoneSourceInfo &source);
    int ParseZoneBufferRefsChunk(
            const unsigned char *bytes,
            u32 byteCount,
            const BlockInfoDescriptorExternalRefs &externalRefs);
    int LoadZoneDescriptors(const CPlugFilePack *installedPack);
    int CountExternalConstructionZones(
            const BlockInfoDescriptorExternalRefs &externalRefs);
};

#endif
