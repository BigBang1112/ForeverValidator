#include "engine/rendering/plug_tree.h"
#include <algorithm>
#include <limits>
#include <memory>

void CPlugTreeVisualMip::CopyFromModel(
        const CPlugTree *source,
        int deepCopy) {
    CPlugTree::CopyFromModel(source, deepCopy);
    if (source == nullptr) {
        return;
    }
    const auto *sourceMip = dynamic_cast<const CPlugTreeVisualMip *>(source);
    if (sourceMip == nullptr) {
        return;
    }

    for (MipLevel &level : mipLevels) {
        if (level.tree != nullptr) {
            DeconnectAsChild(level.tree.get());
        }
    }
    mipLevels.clear();
    mipLevels.reserve(sourceMip->mipLevels.size());
    for (const MipLevel &sourceLevel : sourceMip->mipLevels) {
        mipLevels.push_back(MipLevel{nullptr, sourceLevel.farZ});
    }

    currentLevel = sourceMip->currentLevel;
    SetDistributionFromFarZs();
}

CPlugTree *CPlugTreeVisualMip::InternalRecursiveCreate(
        CPlugTreeCloneFactory cloneFactory,
        int includeResolvable,
        CPlugTree *outRoot) const {
    std::unique_ptr<CPlugTree> ownedRoot;
    CPlugTree *root = outRoot;
    if (root == nullptr) {
        ownedRoot.reset((this->*cloneFactory)());
        root = ownedRoot.get();
    }
    auto *rootMip = dynamic_cast<CPlugTreeVisualMip *>(root);
    if (rootMip == nullptr || rootMip->mipLevels.size() != mipLevels.size()) {
        return nullptr;
    }

    for (u32 mipIndex = 0u; mipIndex < mipLevels.size(); mipIndex++) {
        CPlugTree *sourceTree = mipLevels[mipIndex].tree.get();
        if (sourceTree == nullptr) {
            return nullptr;
        }
        std::unique_ptr<CPlugTree> levelClone(
                sourceTree->InternalRecursiveCreate(
                        cloneFactory,
                        includeResolvable,
                        nullptr));
        if (!levelClone) {
            return nullptr;
        }
        rootMip->SetOwnedLevelTree(mipIndex, std::move(levelClone));
    }

    const u32 childCount = static_cast<u32>(CPlugTree::GetChildCount());
    for (u32 childIndex = 0u; childIndex < childCount; childIndex++) {
        CPlugTree *child = CPlugTree::GetChild(childIndex);
        if ((includeResolvable != 0) != (child->GetIsRooted() != 0)) {
            continue;
        }
        std::unique_ptr<CPlugTree> childClone(
                child->InternalRecursiveCreate(
                        cloneFactory,
                        includeResolvable,
                        nullptr));
        if (!childClone) {
            return nullptr;
        }
        root->AddOwnedChild(std::move(childClone));
    }
    return ownedRoot ? ownedRoot.release() : root;
}

void CPlugTreeVisualMip::DisconnectThisFromModel(
        int duplicatePolicy,
        unsigned long disconnectMask) {
    CPlugTree::DisconnectThisFromModel(duplicatePolicy, disconnectMask);
}

unsigned long CPlugTreeVisualMip::GetRecursiveVertexCount(
        int requireVisible,
        int lastVisualMipOnly) const {
    if (lastVisualMipOnly == 0) {
        return CPlugTree::GetRecursiveVertexCount(requireVisible, 0);
    }

    unsigned long count =
            visual != nullptr ? visual->GetTotalVertexCount() : 0ul;
    const u32 baseChildCount = static_cast<u32>(CPlugTree::GetChildCount());
    for (u32 childIndex = 0u; childIndex < baseChildCount; childIndex++) {
        CPlugTree *child = CPlugTree::GetChild(childIndex);
        if (requireVisible == 0 || child->IsVisible()) {
            count += child->GetRecursiveVertexCount(requireVisible, 1);
        }
    }
    if (!mipLevels.empty()) {
        CPlugTree *lastLevel = mipLevels.back().tree.get();
        if (lastLevel != nullptr &&
            (requireVisible == 0 || lastLevel->IsVisible())) {
            count += lastLevel->GetRecursiveVertexCount(requireVisible, 1);
        }
    }
    return count;
}

unsigned long CPlugTreeVisualMip::GetChildCount(void) const {
    return CPlugTree::GetChildCount() +
           static_cast<unsigned long>(mipLevels.size());
}

CPlugTree *CPlugTreeVisualMip::GetChild(unsigned long index) const {
    const unsigned long baseCount = CPlugTree::GetChildCount();
    if (index < baseCount) {
        return CPlugTree::GetChild(index);
    }
    const unsigned long mipIndex = index - baseCount;
    return mipIndex < mipLevels.size()
            ? mipLevels[mipIndex].tree.get()
            : nullptr;
}

CPlugTree *CPlugTreeVisualMip::GetChildToRenderMip(void) const {
    CPlugTree *selected = currentLevel.has_value()
            ? LevelTree(*currentLevel)
            : nullptr;
    return selected != nullptr
            ? selected->GetChildToRenderMip()
            : CPlugTree::GetChildToRenderMip();
}

unsigned long CPlugTreeVisualMip::LevelCount(void) const {
    return static_cast<unsigned long>(mipLevels.size());
}

CPlugTree *CPlugTreeVisualMip::LevelTree(unsigned long index) const {
    return index < mipLevels.size() ? mipLevels[index].tree.get() : nullptr;
}

float CPlugTreeVisualMip::LevelFarZ(unsigned long index) const {
    return index < mipLevels.size()
            ? mipLevels[index].farZ
            : std::numeric_limits<float>::max();
}

unsigned long CPlugTreeVisualMip::GetChildIndex(CPlugTree *child) {
    const unsigned long baseIndex = CPlugTree::GetChildIndex(child);
    if (baseIndex != InvalidEngineIndex) {
        return baseIndex;
    }

    const unsigned long mipCount = LevelCount();
    for (unsigned long mipIndex = 0ul;
         mipIndex < mipCount;
         mipIndex++) {
        if (mipLevels[mipIndex].tree.get() == child) {
            return mipIndex + CPlugTree::GetChildCount();
        }
    }
    return InvalidEngineIndex;
}

void CPlugTreeVisualMip::SetLevelTree(unsigned long index, CPlugTree *tree) {
    if (index >= mipLevels.size() || tree == nullptr) {
        return;
    }
    if (mipLevels[index].tree.get() == tree) {
        return;
    }
    SetOwnedLevelTree(index, std::unique_ptr<CPlugTree>(tree));
}

void CPlugTreeVisualMip::SetOwnedLevelTree(
        unsigned long index,
        std::unique_ptr<CPlugTree> tree) {
    if (index >= mipLevels.size() || !tree) {
        return;
    }
    CPlugTree *treeView = tree.get();
    std::unique_ptr<CPlugTree> oldTree = std::move(mipLevels[index].tree);
    if (oldTree != nullptr) {
        DeconnectAsChild(oldTree.get());
    }

    mipLevels[index].tree = std::move(tree);
    if (owningSolid != nullptr) {
        treeView->Generate(0);
    }
    ConnectAsChild(treeView, 1);
}

void CPlugTreeVisualMip::SetChild(CPlugTree *child, unsigned long index) {
    const u32 baseCount = CPlugTree::GetChildCount();
    if (index < baseCount) {
        CPlugTree::SetChild(child, index);
        return;
    }
    SetLevelTree(index - baseCount, child);
}

CPlugTree *CPlugTreeVisualMip::DetachChild(unsigned long index) {
    const unsigned long baseCount = CPlugTree::GetChildCount();
    if (index < baseCount) {
        return CPlugTree::DetachChild(index);
    }

    const unsigned long mipIndex = index - baseCount;
    if (mipIndex >= mipLevels.size()) {
        return nullptr;
    }

    std::unique_ptr<CPlugTree> detached =
            std::move(mipLevels[mipIndex].tree);
    mipLevels.erase(mipLevels.begin() + mipIndex);
    if (currentLevel.has_value() && *currentLevel > mipIndex) {
        --*currentLevel;
    }
    SetDistributionFromFarZs();
    if (detached != nullptr) {
        DeconnectAsChild(detached.get());
    }
    return detached.release();
}

CPlugTree *CPlugTreeVisualMip::DetachChildPtr(CPlugTree *child) {
    const unsigned long index = GetChildIndex(child);
    return index != InvalidEngineIndex ? DetachChild(index) : nullptr;
}

void CPlugTreeVisualMip::DeleteChild(unsigned long index) {
    const u32 baseCount = CPlugTree::GetChildCount();
    if (index < baseCount) {
        CPlugTree::DeleteChild(index);
        return;
    }
    std::unique_ptr<CPlugTree> removed(DetachChild(index));
}

void CPlugTreeVisualMip::DeleteChildPtr(CPlugTree *child) {
    const unsigned long index = GetChildIndex(child);
    if (index != InvalidEngineIndex) {
        DeleteChild(index);
    }
}

void CPlugTreeVisualMip::DeleteAllChilds(void) {
    while (GetChildCount() != 0u) {
        DeleteChild(0u);
    }
}

void CPlugTreeVisualMip::SetLevelFarZ(unsigned long index, float farZ) {
    if (index >= mipLevels.size()) {
        return;
    }
    if ((index == 0u || farZ > mipLevels[index - 1u].farZ) &&
        (index + 1u >= mipLevels.size() ||
         farZ < mipLevels[index + 1u].farZ)) {
        mipLevels[index].farZ = farZ;
    }
    SetDistributionFromFarZs();
}

CPlugTreeVisualMip::CPlugTreeVisualMip(void) {
    MarkRenderBeforeFidParametrization();
}

CPlugTreeVisualMip::~CPlugTreeVisualMip(void) {
    for (MipLevel &level : mipLevels) {
        if (level.tree != nullptr) {
            DeconnectAsChild(level.tree.get());
        }
    }
}

void CPlugTreeVisualMip::SetDistributionFromFarZs(void) {
    if (mipLevels.empty()) {
        currentLevel.reset();
        return;
    }
    if (!currentLevel.has_value() || *currentLevel >= mipLevels.size()) {
        currentLevel = static_cast<unsigned long>(mipLevels.size() - 1u);
    }
}

void CPlugTreeVisualMip::SetQualityInternal(unsigned long quality) {
    if (mipLevels.empty()) {
        currentLevel.reset();
        return;
    }
    currentLevel = std::min(
            quality,
            static_cast<unsigned long>(mipLevels.size() - 1u));
}

void CPlugTreeVisualMip::SetQuality(unsigned long quality) {
    SetQualityInternal(quality);
}

unsigned long CPlugTreeVisualMip::SetQualityFromMinZ(float minZ) {
    const auto selected = std::lower_bound(
            mipLevels.begin(),
            mipLevels.end(),
            minZ,
            [](const MipLevel &level, float value) {
                return level.farZ < value;
            });
    const unsigned long quality = selected != mipLevels.end()
            ? static_cast<unsigned long>(selected - mipLevels.begin())
            : (mipLevels.empty()
                       ? InvalidEngineIndex
                       : static_cast<unsigned long>(mipLevels.size() - 1u));
    SetQualityInternal(quality);
    return currentLevel.value_or(InvalidEngineIndex);
}

void CPlugTreeVisualMip::AddLevel(CPlugTree *tree, float farZ) {
    if (tree == nullptr) {
        return;
    }
    AddOwnedLevel(std::unique_ptr<CPlugTree>(tree), farZ);
}

void CPlugTreeVisualMip::AddOwnedLevel(
        std::unique_ptr<CPlugTree> tree,
        float farZ) {
    if (!tree) {
        return;
    }

    const unsigned long farZCount = LevelCount();
    unsigned long insertIndex = 0ul;
    while (insertIndex < farZCount &&
           mipLevels[insertIndex].farZ <= farZ) {
        insertIndex++;
    }

    CPlugTree *treeView = tree.get();
    MipLevel level{std::move(tree), farZ};
    mipLevels.insert(mipLevels.begin() + insertIndex, std::move(level));
    if (owningSolid != nullptr) {
        treeView->Generate(0);
    }
    ConnectAsChild(treeView, 1);
    if (!currentLevel.has_value()) {
        currentLevel = 0ul;
    } else if (insertIndex <= *currentLevel) {
        ++*currentLevel;
    }
    SetDistributionFromFarZs();
}
