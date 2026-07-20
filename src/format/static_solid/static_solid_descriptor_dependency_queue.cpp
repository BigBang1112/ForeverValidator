#include "format/static_solid/static_solid_descriptor_dependency_queue.h"
#include <stddef.h>

#include "engine/resources/catalog_asset_repository.h"
#include "engine/game/replay_challenge_map_input.h"
#include "format/archive/archive_class_ids.h"
#include "format/static_solid/static_scene_archive_loader.h"
#include "engine/scene/replay_scene_placements.h"
#include "engine/game/game_ctn_block.h"
#include "engine/game/game_ctn_block_info.h"
#include "format/pack/installed/plug_file_pack.h"
#include "format/pack/installed/scene_descriptor_folder_paths.h"
#include "format/static_solid/static_solid_archive_reference.h"
#include "format/replay/replay_static_descriptor_limits.h"
#include <new>
namespace {

size_t StaticDescriptorDependencyBoundedLength(const char *text,
                                               size_t capacity) {
    size_t length = 0u;
    while (length < capacity && text[length] != 0) {
        length++;
    }
    return length;
}

int RequireTypedDecorationDescriptor(
        const std::string &selectedPath,
        u32 expectedClassId,
        const StaticSolidArchiveCatalog &catalog,
        CGameCtnReplayStaticSolidDescriptorDependencyQueue &queue) {
    if (selectedPath.empty()) {
        return 0;
    }
    const StaticSolidArchiveCatalogEntry *entry =
            catalog.Find(selectedPath.c_str());
    if (entry == nullptr || entry->descriptorClassId != expectedClassId) {
        return 0;
    }
    return queue.RequireDescriptor(selectedPath.c_str());
}

int RequireDecorationDescriptors(
        const CatalogDecorationSizeDefinition &decoration,
        const StaticSolidArchiveCatalog &catalog,
        CGameCtnReplayStaticSolidDescriptorDependencyQueue &queue) {
    if ((decoration.decorationSizeEmbedded &&
         !decoration.selectedDecorationSizePath.empty()) ||
        (!decoration.decorationSizeEmbedded &&
         decoration.selectedDecorationSizePath.empty())) {
        return 0;
    }
    return RequireTypedDecorationDescriptor(
                   decoration.selectedDecorationPath,
                   TMNF_CLASS_CGameCtnDecoration,
                   catalog,
                   queue) &&
           (decoration.decorationSizeEmbedded ||
            RequireTypedDecorationDescriptor(
                    decoration.selectedDecorationSizePath,
                    TMNF_CLASS_CGameCtnDecorationSize,
                    catalog,
                    queue)) &&
           RequireTypedDecorationDescriptor(
                   decoration.selectedBaseScenePath,
                   TMNF_CLASS_CScene3d,
                   catalog,
                   queue);
}

const char *DescriptorPath(
        const CSceneMobil *mobil,
        const StaticSolidArchiveReferenceCatalog &references) {
    if (mobil == nullptr) {
        return "";
    }
    return references.SelectedDescriptorPath(
            mobil->StaticSolidAsset()).c_str();
}

const CSceneMobil *FamilyHelper(const CGameCtnBlock &block) {
    const CGameCtnBlockInfo *blockInfo = block.BlockInfoRef();
    if (blockInfo == nullptr) {
        return nullptr;
    }
    return blockInfo->FamilyHelperMobilSource(block.UsesGroundMobilSize());
}

int AddBlockPlacementDependencies(
        const ReplaySceneBlockPlacements &placements,
        const StaticSolidArchiveReferenceCatalog &references,
        CGameCtnReplayStaticSolidDescriptorDependencyQueue &queue) {
    for (const ReplaySceneBlockPlacement &placement :
         placements.CollisionPlacements()) {
        const CGameCtnBlock &block = placement.Block();
        if (!queue.RequireDescriptor(
                    DescriptorPath(block.MainMobil(), references))) {
            return 0;
        }
        CGameCtnBlockInfo *blockInfo = block.BlockInfoRef();
        if (!queue.RequestOptionalDescriptor(
                    DescriptorPath(FamilyHelper(block), references)) ||
            !queue.RequestOptionalDescriptor(
                    DescriptorPath(blockInfo != nullptr
                                           ? blockInfo->CommonHelperMobilSource()
                                           : nullptr,
                                   references)) ||
            !queue.RequestOptionalDescriptor(
                    DescriptorPath(block.TriggerMobil(), references))) {
            return 0;
        }
        for (const CMwNodRef<CSceneMobil> &clipSource :
             block.ClipSourceMobils()) {
            if (!queue.RequestOptionalDescriptor(
                        DescriptorPath(clipSource.Get(), references))) {
                return 0;
            }
        }
    }
    for (const ReplayScenePylonPlacement &placement :
         placements.PylonPlacements()) {
        if (!queue.RequireDescriptor(
                    DescriptorPath(&placement.Mobil(), references))) {
            return 0;
        }
    }
    return 1;
}

}  // namespace

const char *CGameCtnReplayStaticSolidDescriptorDependency::SelectedDescriptorPath()
        const {
    return selectedDescriptorPath.c_str();
}

int CGameCtnReplayStaticSolidDescriptorDependency::IsRequired() const {
    return priority ==
           CGameCtnReplayStaticSolidDescriptorDependencyPriority::Required;
}

void CGameCtnReplayStaticSolidDescriptorDependencyQueue::Free() {
    dependencies.clear();
}

int CGameCtnReplayStaticSolidDescriptorDependencyQueue::Has(
        const char *selectedDescriptorPath) const {
    if (selectedDescriptorPath == nullptr ||
        selectedDescriptorPath[0] == '\0') {
        return 0;
    }
    for (const CGameCtnReplayStaticSolidDescriptorDependency &request :
         dependencies) {
        if (request.selectedDescriptorPath == selectedDescriptorPath) {
            return 1;
        }
    }
    return 0;
}

int CGameCtnReplayStaticSolidDescriptorDependencyQueue::QueueDescriptor(
        const char *selectedDescriptorPath,
        CGameCtnReplayStaticSolidDescriptorDependencyPriority priority) {
    if (selectedDescriptorPath == nullptr ||
        selectedDescriptorPath[0] == '\0' ||
        Has(selectedDescriptorPath)) {
        return 1;
    }
    if (StaticDescriptorDependencyBoundedLength(
                selectedDescriptorPath,
                CGameCtnReplayStaticSolidDescriptorPathCapacity) >=
        CGameCtnReplayStaticSolidDescriptorPathCapacity ||
        dependencies.size() >= UINT32_MAX) {
        return 0;
    }
    CGameCtnReplayStaticSolidDescriptorDependency dependency;
    try {
        dependency.selectedDescriptorPath = selectedDescriptorPath;
        dependency.priority = priority;
        dependencies.push_back(dependency);
        return 1;
    } catch (const std::bad_alloc &) {
        return 0;
    }
}

int CGameCtnReplayStaticSolidDescriptorDependencyQueue::RequireDescriptor(
        const char *selectedDescriptorPath) {
    return QueueDescriptor(
            selectedDescriptorPath,
            CGameCtnReplayStaticSolidDescriptorDependencyPriority::Required);
}

int CGameCtnReplayStaticSolidDescriptorDependencyQueue::RequestOptionalDescriptor(
        const char *selectedDescriptorPath) {
    return QueueDescriptor(
            selectedDescriptorPath,
            CGameCtnReplayStaticSolidDescriptorDependencyPriority::Optional);
}

int CGameCtnReplayStaticSolidDescriptorDependencyQueue::
RequestExternalRefDescriptors(
        const SceneDescriptorFolderPaths &externalFolders,
        const CPlugFilePack &pack,
        const StaticSolidArchiveLoadSession &archive,
        const std::vector<CGameCtnReplayStaticSolidExternalNodeRef>
                &nodeRefs) {
    for (const CGameCtnReplayStaticSolidExternalNodeRef &nodeRef : nodeRefs) {
        if (!nodeRef.CanResolvePath(1)) {
            continue;
        }

        CGameCtnReplayStaticSolidExternalNodePathResult path;
        if (!CGameCtnReplayStaticSolidExternalNodePathResolver::
                    BuildPlainAndTrySelectedPath(
                            &externalFolders,
                            &pack,
                            nodeRef,
                            1,
                            &path)) {
            return 0;
        }
        if (path.HasSelectedPath() &&
            !archive.HasDescriptor(path.SelectedPath()) &&
            !RequestOptionalDescriptor(path.SelectedPath())) {
            return 0;
        }
    }
    return 1;
}

int CGameCtnReplayStaticSolidDescriptorDependencyQueue::
SeedFromReplayStaticInputs(
        const CGameCtnReplayMapInput *mapInput,
        const ReplaySceneBlockPlacements *placements,
        const CGameCtnReplayArchiveStaticModelCollection *archiveModels,
        const StaticSolidArchiveCatalog *inventory,
        const StaticSolidArchiveReferenceCatalog *references,
        const CatalogDecorationSizeDefinition *decorationSize) {
    return mapInput != nullptr &&
           SeedFromReplayMapStaticInputs(
                   placements, archiveModels, references) &&
           SeedFromReplayDecorationStaticInputs(
                   inventory, decorationSize);
}

int CGameCtnReplayStaticSolidDescriptorDependencyQueue::
SeedFromReplayMapStaticInputs(
        const ReplaySceneBlockPlacements *placements,
        const CGameCtnReplayArchiveStaticModelCollection *archiveModels,
        const StaticSolidArchiveReferenceCatalog *references) {
    return placements != nullptr && references != nullptr &&
           AddBlockPlacementDependencies(*placements, *references, *this) &&
           archiveModels != nullptr &&
           archiveModels->ContributeStaticSolidDescriptorDependencies(*this);
}

int CGameCtnReplayStaticSolidDescriptorDependencyQueue::
SeedFromReplayDecorationStaticInputs(
        const StaticSolidArchiveCatalog *inventory,
        const CatalogDecorationSizeDefinition *decorationSize) {
    return inventory != nullptr && decorationSize != nullptr &&
           RequireDecorationDescriptors(
                   *decorationSize, *inventory, *this) &&
           inventory->RequestDecoratorSolidDependencies(*this);
}

int CGameCtnReplayStaticSolidDescriptorDependencyQueue::
RequireMissingDescriptor(
        const StaticSolidArchiveLoadSession *store,
        const char *selectedDescriptorPath) {
    if (store == nullptr) {
        return 0;
    }
    if (selectedDescriptorPath == nullptr ||
        selectedDescriptorPath[0] == '\0' ||
        store->HasDescriptor(selectedDescriptorPath)) {
        return 1;
    }
    return RequireDescriptor(selectedDescriptorPath);
}

int CGameCtnReplayStaticSolidDescriptorDependencyQueue::DecodeReachablePayloadGraph(
        const StaticSolidArchiveCatalog *inventory,
        StaticSolidArchiveLoadSession *store,
        CGameCtnReplayArchiveStaticModelCollection *archiveModels,
        u32 *selectedMissingOut) {
    if (inventory == nullptr || store == nullptr) {
        return 0;
    }
    for (u32 decodeCursor = 0u; decodeCursor < dependencies.size();
         decodeCursor++) {
        const CGameCtnReplayStaticSolidDescriptorDependency dependency =
                dependencies[decodeCursor];
        if (!store->DecodeDescriptorDependency(dependency,
                                               inventory,
                                               archiveModels,
                                               this,
                                               selectedMissingOut)) {
            return 0;
        }
    }
    return 1;
}
