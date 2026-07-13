#include "engine/rendering/plug_tree.h"
#include <memory>
#include <vector>

namespace {

std::unique_ptr<CPlugTree> MergeSolidCopies(
        const std::vector<CPlugTree *> &trees) {
    std::vector<std::unique_ptr<CPlugTree>> copies;
    copies.reserve(trees.size());
    for (CPlugTree *tree : trees) {
        if (tree != nullptr) {
            copies.emplace_back(tree->DuplicateRecursive());
        }
    }

    if (copies.empty()) {
        return nullptr;
    }
    if (copies.size() == 1u) {
        return std::move(copies.front());
    }

    auto merged = std::make_unique<CPlugTree>();
    for (std::unique_ptr<CPlugTree> &copy : copies) {
        merged->AddOwnedChild(std::move(copy));
    }
    merged->UpdateBoundingBox(1);
    return merged;
}

}  // namespace

void CPlugTree::Generate(int flags) {
    CPlugTreeGenerator *generator = this->generator;
    if (generator != nullptr) {
        const int shouldGenerate =
                flags != 0 ||
                (this->visual == nullptr &&
                 this->GetVolatileChildCount() == 0u) ||
                (dynamic_cast<CPlugTreeGenSolid *>(generator) != nullptr &&
                 this->GetRootedChildCount() != 0u);
        if (shouldGenerate) {
            generator->GenerateTree(this);
            this->UpdateBoundingBox(1u);
        }
    }

    const u32 childCount = this->GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        CPlugTree *child = this->GetChild(childIndex);
        child->Generate(flags);
    }
}

void CPlugTreeGenerator::CopyGeneratedTree(
        const CPlugTree *source,
        CPlugTree *destination) {
    if (source == nullptr || destination == nullptr) {
        return;
    }

    destination->DeleteAllVolatileChilds();
    const unsigned long childCount = source->GetChildCount();
    for (unsigned long childIndex = 0ul;
         childIndex < childCount;
         childIndex++) {
        const CPlugTree *child = source->GetChild(childIndex);
        if (child == nullptr || !child->IsVolatileChild()) {
            continue;
        }
        std::unique_ptr<CPlugTree> copy(child->DuplicateRecursive());
        copy->SetIsRooted(0);
        destination->AddOwnedChild(std::move(copy));
    }
}

CPlugTree *CPlugTreeGenSolid::MergeSolidInTree(
        const CFastBuffer<CPlugTree *> &trees) {
    return MergeSolidCopies(trees.Values()).release();
}

void CPlugTreeGenSolid::GenerateTree(CPlugTree *tree) {
    if (tree == nullptr) {
        return;
    }

    CleanTree(tree);
    std::vector<CPlugTree *> sourceTrees;
    const unsigned long childCount = tree->GetChildCount();
    sourceTrees.reserve(childCount);
    for (unsigned long childIndex = 0ul;
         childIndex < childCount;
         childIndex++) {
        CPlugTree *child = tree->GetChild(childIndex);
        if (child != nullptr && !child->IsVolatileChild()) {
            sourceTrees.push_back(child);
        }
    }

    std::unique_ptr<CPlugTree> generatedTree = MergeSolidCopies(sourceTrees);
    if (generatedTree) {
        generatedTree->SetIsRooted(0);
        tree->AddOwnedChild(std::move(generatedTree));
    }
}

void CPlugTreeGenSolid::CleanTree(CPlugTree *tree) {
    if (tree != nullptr) {
        tree->DeleteAllVolatileChilds();
    }
}

void CPlugTreeGenSolid::GetOptimizedGroups(
        CFastBuffer<SPlugTreeOptimGroup *> &groups,
        const SPlugTreeOptimCriteria &criteria,
        const SPlugTreeOptimTravel &travel,
        CPlugTree *tree) {
    if (tree == nullptr) {
        return;
    }
    if (tree->GetVolatileChildCount() == 0ul) {
        GenerateTree(tree);
    }

    const unsigned long childCount = tree->GetChildCount();
    for (unsigned long childIndex = 0ul;
         childIndex < childCount;
         childIndex++) {
        CPlugTree *child = tree->GetChild(childIndex);
        if (child != nullptr && child->IsVolatileChild()) {
            child->GetOptimizedGroups(groups, criteria, travel);
        }
    }
}

void CPlugTree::RecursiveSetUnassigned(void) {
    const u32 childCount = GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        CPlugTree *child = GetChild(childIndex);
        if (child->state_.rooted) {
            continue;
        }
        CMwId unassigned;
        unassigned.SetInvalid();
        child->SetPlugId(unassigned);
        child->RecursiveSetUnassigned();
    }
}

void CPlugTree::RecursiveSetRooted(void) {
    SetIsRooted(1);
    const u32 childCount = GetChildCount();
    for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
        CPlugTree *child = GetChild(childIndex);
        if (child->state_.rooted) {
            continue;
        }
        child->RecursiveSetRooted();
    }
}

void CPlugTree::SetGenerator(CPlugTreeGenerator *generator,
                                        int updateGeneratorLinks) {
    CPlugTreeGenerator *oldGenerator = this->generator;
    if (oldGenerator != nullptr) {
        if (updateGeneratorLinks != 0 || generator != nullptr) {
            oldGenerator->CleanTree(this);
        }
    }

    if (updateGeneratorLinks == 0 && generator == nullptr && this->generator != nullptr) {
        RecursiveSetUnassigned();
        RecursiveSetRooted();
    }

    this->generator.MwSetNod(generator);

    if (this->generator != nullptr && updateGeneratorLinks != 0) {
        this->generator->GenerateTree(this);
    }
}
