#include "engine/scene/plug_solid.h"
#include <charconv>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <typeinfo>

#include "engine/core/fast_string.h"
CMwId CPlugSolid::s_ParamId_PackBrotherShaders =
        CMwId::CreateFromLocalName("PlugSolidPackBrotherShaders");

void CPlugSolid::MakeTreeIdsUnique(void) {
    struct TreeIdUse {
        u32 lastDuplicateSuffix = 0u;
    };
    struct TreeIdLess {
        bool operator()(const CMwId &left, const CMwId &right) const {
            return left < right;
        }
    };

    const auto duplicateName = [](const CFastString &baseName, u32 suffix) {
        const std::int64_t signedSuffix = suffix <= 0x7fffffffu
                ? static_cast<std::int64_t>(suffix)
                : static_cast<std::int64_t>(suffix) - 0x100000000ll;
        char digits[24];
        const auto converted = std::to_chars(
                digits,
                digits + sizeof(digits),
                signedSuffix);
        const std::size_t digitCount = static_cast<std::size_t>(
                converted.ptr - digits);

        std::string result(baseName.Data(), baseName.Length());
        result.push_back('_');
        if (digitCount < 2u) {
            result.append(2u - digitCount, ' ');
        }
        result.append(digits, digitCount);
        return result;
    };

    const unsigned long rootedTreeCount =
            this->collisionTree_->GetRecursiveTreeCount(0, 1);
    if (rootedTreeCount <= 1u) {
        if (this->collisionTree_->MwGetId()->IsInvalid()) {
            CMwId id;
            this->GivePlugId(id);
            this->collisionTree_->InternalSetMwId(id);
        }
        return;
    }

    std::map<CMwId, TreeIdUse, TreeIdLess> usedTreeIds;

    unsigned long treeCursor = this->collisionTree_->GetAllTreeStart();
    while (treeCursor != InvalidEngineIndex) {
        CPlugTree *tree = this->collisionTree_->GetAllTreeNext(treeCursor);
        if (tree->GetIsRooted()) {
            CMwId id = *tree->MwGetId();
            if (id.IsInvalid()) {
                this->GivePlugId(id);
                tree->InternalSetMwId(id);
            } else if (id.HasNumberName()) {
                u32 number = 0u;
                (void)id.TryGetNumber(number);
                if (number >= this->nextGeneratedPlugId_ ||
                    usedTreeIds.find(id) != usedTreeIds.end()) {
                    this->GivePlugId(id);
                    tree->InternalSetMwId(id);
                }
            } else {
                auto duplicate = usedTreeIds.find(id);
                if (duplicate != usedTreeIds.end()) {
                    CFastString baseName;
                    id.GetName(baseName);
                    u32 suffix = duplicate->second.lastDuplicateSuffix;
                    do {
                        suffix++;
                        const std::string renamed = duplicateName(
                                baseName,
                                suffix);
                        id.SetLocalName(renamed.c_str());
                    } while (usedTreeIds.find(id) != usedTreeIds.end());

                    duplicate->second.lastDuplicateSuffix = suffix;
                    tree->InternalSetMwId(id);
                }
            }
            usedTreeIds.emplace(id, TreeIdUse{});
        }
    }
}

void CPlugSolid::InternalDisconnectSubTree(
        CPlugTree *tree) {
    tree->InternalSetSolid(nullptr);
    const u32 childCount = tree->GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        this->InternalDisconnectSubTree(tree->GetChild(childIndex));
    }
}

void CPlugSolid::InternalConnectSubTree(
        CPlugTree *tree) {
    tree->SetOwningSolidRecursive(this);
    this->MakeTreeIdsUnique();
}

CPlugSolid::CPlugSolid(void) {
    this->nextGeneratedPlugId_ = 1u;
    this->exclusionEllipsoidRadius_ = 0.0f;
    this->useModel = true;
}

CPlugSolid::~CPlugSolid(void) {
    physical_.BindBoundsTree(nullptr);
    collisionTree_.reset();
}

void CPlugSolid::CreateDefaultData(void) {
    physical_ = CPlugPhysicalObject();
    nextGeneratedPlugId_ = 1u;
    collisionTree_.reset();
    model_.MwSetNod(nullptr);
    exclusionEllipsoidRadius_ = 0.0f;
    useModel = true;
}

CPlugTree *CPlugSolid::CollisionTree(void) const {
    return collisionTree_.get();
}

CPlugSolid *CPlugSolid::CreateModelInstance(void) {
    auto clone = std::make_unique<CPlugSolid>();
    clone->SetModel(this);
    return clone.release();
}

void CPlugSolid::SetModel(CPlugSolid *model) {
    this->model_.MwSetNod(model);
    if (model != nullptr && useModel) {
        this->nextGeneratedPlugId_ = model->nextGeneratedPlugId_;
        this->physical_.CopyFrom(model->physical_);
        CPlugTree *tree = model->CollisionTree() != nullptr
                ? model->CollisionTree()->InternalCreateSolidModelInstance()
                : nullptr;
        this->SetTree(tree, 0);
    }
}

CPlugTree *CPlugSolid::GetPlugFromId(const CMwId &id) const {
    if (this->collisionTree_ == nullptr) {
        return nullptr;
    }
    if (id.IsInvalid()) {
        return nullptr;
    }

    CPlugTree *root = this->collisionTree_.get();
    return root->GetPlugFromId(id);
}

void CPlugSolid::GivePlugId(CMwId &idOut) const {
    CMwId id;
    id.SetNumber(nextGeneratedPlugId_++);
    idOut = id;
}

void CPlugSolid::DisconnectFromModel(int keepResolvedTree) {
    if (keepResolvedTree == 0 && collisionTree_ != nullptr) {
        this->collisionTree_->DisconnectFromModel(1, InvalidEngineIndex);
        this->collisionTree_->Generate(0);
        this->collisionTree_->UpdateBoundingBox(1);
    }

    this->model_.MwSetNod(nullptr);
}

void CPlugSolid::SetUseModel(int enabled) {
    const bool requested = enabled != 0;
    if (requested == useModel) {
        return;
    }
    useModel = requested;
    if (model_ == nullptr) {
        return;
    }
    if (useModel) {
        SetModel(model_.Get());
    } else if (collisionTree_ != nullptr) {
        collisionTree_->DisconnectFromModel(1, InvalidEngineIndex);
    }
}


void CPlugSolid::SetTree(
        CPlugTree *tree,
        int refreshFlags) {
    CPlugTree *oldTree = this->collisionTree_.get();
    if (tree == oldTree) {
        return;
    }

    this->collisionTree_.reset(tree);
    this->physical_.BindBoundsTree(tree);
    if (tree != nullptr) {
        tree->Generate(0);
        this->InternalConnectSubTree(this->collisionTree_.get());
        this->collisionTree_->UpdateBoundingBox(refreshFlags);
    }
}

void CPlugSolid::SetOwnedTree(
        std::unique_ptr<CPlugTree> tree,
        int refreshFlags) {
    if (tree.get() == collisionTree_.get()) {
        return;
    }

    collisionTree_ = std::move(tree);
    physical_.BindBoundsTree(collisionTree_.get());
    if (collisionTree_ != nullptr) {
        collisionTree_->Generate(0);
        InternalConnectSubTree(collisionTree_.get());
        collisionTree_->UpdateBoundingBox(refreshFlags);
    }
}

void CPlugSolid::AddTree(CPlugTree *tree) {
    if (this->collisionTree_ == nullptr) {
        this->SetOwnedTree(std::make_unique<CPlugTree>(), 1);
    }

    if (typeid(*collisionTree_) != typeid(CPlugTree)) {
        std::unique_ptr<CPlugTree> oldRoot = std::move(this->collisionTree_);
        this->physical_.BindBoundsTree(nullptr);
        this->SetOwnedTree(std::make_unique<CPlugTree>(), 1);
        this->collisionTree_->AddChild(oldRoot.release());
    }

    this->collisionTree_->AddChild(tree);
    tree->UpdateBoundingBox(1u);
    this->collisionTree_->UpdateBoundingBox(0u);
}
