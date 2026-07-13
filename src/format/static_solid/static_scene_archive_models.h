#pragma once

#include <optional>
#include <string>
#include <vector>

#include "engine/physics/dynamics/hms_item.h"
#include "engine/resources/static_solid_asset.h"
class CGameCtnBlockInfo;
class CSceneMobil;
class InstalledPackAssetRepository;
class StaticSolidArchiveReferenceCatalog;
struct CGameCtnReplayStaticSolidDescriptorDependencyQueue;

class CGameCtnReplayArchiveStaticModel {
public:
    const GmIso4 &WorldIso() const;
    const std::optional<CHmsItem::Properties> &ItemProperties() const;
    void BindPrototype(StaticSolidPrototype prototype);
    const StaticSolidPrototype &Prototype() const;

private:
    friend struct StaticSceneArchiveModelRecord;

    GmIso4 worldIso_{};
    std::optional<CHmsItem::Properties> itemProperties_;
    StaticSolidPrototype prototype_;
};

struct StaticSceneArchiveModelSource {
    std::string selectedDescriptorPath;
    std::string sceneObjectId;
    std::optional<u32> treeNodeIndex;

    bool IsWarp() const;
};

struct StaticSceneArchiveModelRecord {
    StaticSceneArchiveModelSource source;
    CGameCtnReplayArchiveStaticModel model;

    int ConfigureFromBlockInfoMobil(const char *blockName,
                                    CGameCtnBlockInfo *blockInfo,
                                    CSceneMobil *mobil,
                                    const StaticSolidArchiveReferenceCatalog
                                            &references);
    int ConfigureFromSceneObject(const float sceneIso[12],
                                 u32 treeNodeIndex,
                                 const char *sceneObjectId,
                                 const char *selectedDescriptorPath);
};

class CGameCtnReplayArchiveStaticModelCollection {
public:
    u32 Count() const;

    template<typename Visitor>
    int ForEachModel(Visitor visitor) const {
        for (const StaticSceneArchiveModelRecord &record : records) {
            if (!visitor(record.model)) {
                return 0;
            }
        }
        return 1;
    }

    template<typename Visitor>
    int ForEachMutableRecord(Visitor visitor) {
        for (StaticSceneArchiveModelRecord &record : records) {
            if (!visitor(record)) {
                return 0;
            }
        }
        return 1;
    }

    int AppendDecodedSceneObject(const StaticSceneArchiveModelRecord &record);
    int AppendIfDescriptorMissing(const StaticSceneArchiveModelRecord &record);
    int ContributeStaticSolidDescriptorDependencies(
            CGameCtnReplayStaticSolidDescriptorDependencyQueue &queue) const;
    void Free();

private:
    bool HasDescriptor(const char *selectedDescriptorPath) const;
    int Append(const StaticSceneArchiveModelRecord &record);

    std::vector<StaticSceneArchiveModelRecord> records;
};

bool LoadCatalogArchiveStaticModels(
        InstalledPackAssetRepository &assets,
        CGameCtnReplayArchiveStaticModelCollection &archiveModels);
