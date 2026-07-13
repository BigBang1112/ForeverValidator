#include "engine/rendering/plug_tree.h"
#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>


bool SPlugTreeOptimCriteria::CopiesGroupProperties(void) const {
    return copyGroupProperties;
}

bool SPlugTreeOptimCriteria::EnforcesMergedVertexLimit(void) const {
    return mergedVertexLimit.has_value();
}

bool SPlugTreeOptimCriteria::EnforcesBoxExtentLimit(void) const {
    return boxExtentLimit.has_value();
}

bool SPlugTreeOptimCriteria::MarksGroupedVisualDirty(void) const {
    return markGroupedVisualDirty;
}

bool SPlugTreeOptimCriteria::WantsSelfWeld(void) const {
    return selfWeld;
}

bool SPlugTreeOptimCriteria::WantsProviderOptimization(void) const {
    return providerOptimization;
}

bool SPlugTreeOptimCriteria::NeedsProviderPostProcess(void) const {
    return WantsSelfWeld() || WantsProviderOptimization();
}

u32 SPlugTreeOptimCriteria::MergedVertexLimit(void) const {
    return mergedVertexLimit.value_or(0u);
}

u32 SPlugTreeOptimCriteria::StandaloneVertexLimit(void) const {
    return standaloneVertexLimit;
}

float SPlugTreeOptimCriteria::BoxExtentLimit(void) const {
    return boxExtentLimit.value_or(0.0f);
}

u32 SPlugTreeOptimCriteria::ProviderOptimizeMode(void) const {
    return static_cast<u32>(providerOptimizationMode);
}

float SPlugTreeOptimCriteria::SelfWeldAngle(void) const {
    return selfWeldAngle;
}

float SPlugTreeOptimCriteria::SelfWeldDistance(void) const {
    return selfWeldDistance;
}

float SPlugTreeOptimCriteria::SelfWeldPrecision(void) const {
    return selfWeldPrecision;
}

float SPlugTreeOptimCriteria::SelfWeldTolerance(void) const {
    return selfWeldTolerance;
}

u32 SPlugTreeOptimCriteria::SelfWeldFlags(void) const {
    return static_cast<u32>(selfWeldOptions);
}

PlugTreeOptimizationBatch::PlugTreeOptimizationBatch(
        unsigned long expectedGroupCount) {
    ownedGroups_.reserve(expectedGroupCount);
}

PlugTreeOptimizationBatch::~PlugTreeOptimizationBatch(void) = default;

SPlugTreeOptimGroup &PlugTreeOptimizationBatch::CreateGroup(
        std::vector<SPlugTreeOptimGroup *> &view) {
    auto group = std::make_unique<SPlugTreeOptimGroup>();
    SPlugTreeOptimGroup *groupView = group.get();
    ownedGroups_.push_back(std::move(group));
    view.push_back(groupView);
    return *groupView;
}

SPlugTreeOptimTravel::SPlugTreeOptimTravel(
        PlugTreeOptimizationBatch &batch)
    : batch_(&batch) {
}

SPlugTreeOptimTravel SPlugTreeOptimTravel::WithOptimizationBatch(
        PlugTreeOptimizationBatch &batch) const {
    SPlugTreeOptimTravel result = *this;
    result.batch_ = &batch;
    return result;
}

SPlugTreeOptimGroup &SPlugTreeOptimTravel::CreateGroup(
        std::vector<SPlugTreeOptimGroup *> &groups) const {
    return batch_->CreateGroup(groups);
}

SPlugTreeOptimGroup::SPlugTreeOptimGroup(void) = default;

SPlugTreeOptimGroup::~SPlugTreeOptimGroup(void) = default;

CPlugMaterial *SPlugTreeOptimGroup::Material(void) const {
    return material_;
}

CPlugShader *SPlugTreeOptimGroup::Shader(void) const {
    return shader_;
}

CPlugSurface *SPlugTreeOptimGroup::Surface(void) const {
    return surface_;
}

void SPlugTreeOptimGroup::SetMaterial(CPlugMaterial *material) {
    material_ = material;
}

void SPlugTreeOptimGroup::SetShader(CPlugShader *shader) {
    shader_ = shader;
}

void SPlugTreeOptimGroup::SetSurface(CPlugSurface *surface) {
    surface_ = surface;
}

void SPlugTreeOptimGroup::AdoptMissingTreeState(const CPlugTree &tree) {
    if (surface_ == nullptr) {
        surface_ = tree.Surface();
    }
    if (material_ == nullptr) {
        material_ = tree.Material();
    }
    if (shader_ == nullptr) {
        shader_ = tree.Shader();
    }
}

namespace {
bool GroupingPropertiesMatch(const SPlugTreeOptimProperties &left,
                             const SPlugTreeOptimProperties &right) {
    return left.rooted == right.rooted &&
           left.visible == right.visible &&
           left.shaderInheritance == right.shaderInheritance &&
           left.materialInheritance == right.materialInheritance &&
           left.castsShadows == right.castsShadows;
}

void TransformVertexRange(GxVertex *vertices,
                          u32 vertexCount,
                          const GmIso4 &transform) {
    for (u32 index = 0u; index < vertexCount; index++) {
        GxVertex &vertex = vertices[index];
        vertex.position.Mult(transform);

        const float nx = vertex.normal.x;
        const float ny = vertex.normal.y;
        const float nz = vertex.normal.z;
        vertex.normal.x =
                nx * transform.Element(GmAxis::X, GmAxis::X) +
                ny * transform.Element(GmAxis::X, GmAxis::Y) +
                nz * transform.Element(GmAxis::X, GmAxis::Z);
        vertex.normal.y =
                ny * transform.Element(GmAxis::Y, GmAxis::Y) +
                nx * transform.Element(GmAxis::Y, GmAxis::X) +
                nz * transform.Element(GmAxis::Y, GmAxis::Z);
        vertex.normal.z =
                ny * transform.Element(GmAxis::Z, GmAxis::Y) +
                nx * transform.Element(GmAxis::Z, GmAxis::X) +
                nz * transform.Element(GmAxis::Z, GmAxis::Z);
    }
}
}

u32 SPlugTreeOptimGroup::TransformCount(void) const {
    return static_cast<u32>(transforms.size());
}

SPlugTreeOptimTransf *SPlugTreeOptimGroup::TransformAt(u32 index) {
    return &transforms[index];
}

const SPlugTreeOptimTransf *SPlugTreeOptimGroup::TransformAt(u32 index) const {
    return &transforms[index];
}

SPlugTreeOptimTransf *SPlugTreeOptimGroup::FirstTransform(void) {
    return transforms.empty() ? nullptr : &transforms.front();
}

CPlugVisual *SPlugTreeOptimGroup::FirstVisual(void) const {
    for (const SPlugTreeOptimTransf &transform : transforms) {
        if (transform.tree != nullptr && transform.tree->Visual() != nullptr) {
            return transform.tree->Visual();
        }
    }
    return nullptr;
}

CPlugVisual *SPlugTreeOptimGroup::FirstTransformVisual(void) const {
    return !transforms.empty() && transforms.front().tree != nullptr
            ? transforms.front().tree->Visual()
            : nullptr;
}

bool SPlugTreeOptimGroup::HasMatchingVertexFormat(
        const CPlugVisual *visual) const {
    CPlugVisual *firstVisual = FirstVisual();
    return visual == nullptr || firstVisual == nullptr ||
           visual->HasMatchingVertexFormat(*firstVisual);
}

bool SPlugTreeOptimGroup::IsSingleTreeGroup(void) const {
    return kind != Kind::MergedGeometry;
}

void SPlugTreeOptimGroup::AdoptGeneratedTree(
        std::unique_ptr<CPlugTree> tree) {
    generatedTree = std::move(tree);
}

std::unique_ptr<CPlugTree> SPlugTreeOptimGroup::TakeGeneratedTree(void) {
    return std::move(generatedTree);
}

bool SPlugTreeOptimGroup::HasSurfaceConflict(const CPlugTree &tree) const {
    return surface_ != nullptr && tree.Surface() != nullptr;
}

bool SPlugTreeOptimGroup::HasMaterialMismatch(const CPlugTree &tree) const {
    return material_ != tree.Material();
}

bool SPlugTreeOptimGroup::HasShaderMismatch(const CPlugTree &tree) const {
    return shader_ != nullptr && tree.Shader() != nullptr &&
           shader_ != tree.Shader();
}

void SPlugTreeOptimGroup::ExpandBoxWith(const CPlugTree &tree) {
    box.Union(tree.Box());
}

void SPlugTreeOptimGroup::AddVisualCounts(CPlugVisual &visual,
                                          u32 visualVertexCount) {
    indexCount += visual.GetVertexIndexCount();
    texCoordCount += visual.TexCoordVertexCount();
    vertexCount += visualVertexCount;
}

void SPlugTreeOptimGroup::AddTransform(
        const SPlugTreeOptimTransf &transform) {
    transforms.push_back(transform);
}

void SPlugTreeOptimGroup::CopyTreeBox(const CPlugTree &tree) {
    box = tree.Box();
}

void SPlugTreeOptimGroup::InitCommon(CPlugTree &tree,
                                     Kind newKind) {
    kind = newKind;
    material_ = tree.Material();
    shader_ = tree.Shader();
    surface_ = tree.Surface();
    properties = tree.OptimizationProperties();
    CopyTreeBox(tree);
    indexCount = 0u;
    texCoordCount = 0u;
    vertexCount = 0u;
}

void SPlugTreeOptimGroup::AddTreeTransform(
        CPlugTree &tree,
        const SPlugTreeOptimTravel &travel) {
    SPlugTreeOptimTransf transform;
    transform.tree = &tree;
    transform.hasIso = travel.hasIso;
    transform.iso = travel.iso;
    AddTransform(transform);
}

void SPlugTreeOptimGroup::InitAsSingleTree(
        CPlugTree &tree,
        const SPlugTreeOptimTravel &travel) {
    InitCommon(tree, Kind::SingleTree);
    AddTreeTransform(tree, travel);
}

void SPlugTreeOptimGroup::InitAsMergeTree(
        CPlugTree &tree,
        const SPlugTreeOptimTravel &travel) {
    InitCommon(tree, Kind::MergedGeometry);
    CPlugVisual *visual = tree.Visual();
    if (visual != nullptr) {
        indexCount = visual->GetVertexIndexCount();
        texCoordCount = visual->TexCoordVertexCount();
        vertexCount = visual->GetTotalVertexCount();
    }
    AddTreeTransform(tree, travel);
}

void SPlugTreeOptimGroup::InitAsGeneratedMip(
        std::unique_ptr<CPlugTree> tree,
        const SPlugTreeOptimProperties &newProperties) {
    kind = Kind::GeneratedMip;
    properties = newProperties;
    transforms.clear();
    SPlugTreeOptimTransf transform;
    transform.tree = tree.get();
    transform.iso.SetIdentity();
    AddTransform(transform);
    AdoptGeneratedTree(std::move(tree));
}

SPlugTreeOptimTravel SPlugTreeOptimTravel::WithTreeLocalIso(
        const CPlugTree &tree) const {
    SPlugTreeOptimTravel result = *this;
    if (tree.UsesLocation()) {
        result.hasIso = true;
        result.iso.SetMult(tree.LocalIso(), iso);
    }
    return result;
}

int CPlugTree::IsOptimizable(
        const SPlugTreeOptimCriteria &criteria) const {
    CPlugVisual *visual = Visual();
    if (ModelSolidReference() == nullptr || visual == nullptr) {
        return 1;
    }
    return criteria.StandaloneVertexLimit() >=
            visual->GetTotalVertexCount();
}

int CPlugTree::IsEqual(SPlugTreeOptimGroup *group,
                       const SPlugTreeOptimCriteria &criteria,
                       const SPlugTreeOptimTravel &travel) {
    (void)travel;
    if (group->IsSingleTreeGroup()) {
        return 0;
    }
    if (criteria.CopiesGroupProperties() &&
        !GroupingPropertiesMatch(OptimizationProperties(),
                                 group->properties)) {
        return 0;
    }

    u32 visualVertexCount = 0u;
    CPlugVisual *provider = Visual();
    if (provider != nullptr) {
        visualVertexCount = provider->GetTotalVertexCount();
        if (criteria.EnforcesMergedVertexLimit() &&
            visualVertexCount + group->vertexCount >
                    criteria.MergedVertexLimit()) {
            return 0;
        }
    }
    if (!group->HasMatchingVertexFormat(provider)) {
        return 0;
    }

    GmBoxAligned expandedBox = group->box;
    expandedBox.Union(Box());
    if (criteria.EnforcesBoxExtentLimit() &&
        (criteria.BoxExtentLimit() < expandedBox.halfExtents.x * 2.0f ||
         criteria.BoxExtentLimit() < expandedBox.halfExtents.y * 2.0f ||
         criteria.BoxExtentLimit() < expandedBox.halfExtents.z * 2.0f)) {
        return 0;
    }
    if (group->HasSurfaceConflict(*this) ||
        group->HasMaterialMismatch(*this) ||
        group->HasShaderMismatch(*this)) {
        return 0;
    }

    group->box = expandedBox;
    if (provider != nullptr) {
        group->AddVisualCounts(*provider, visualVertexCount);
    }
    return 1;
}

CPlugVisual *CPlugTree::CreateGroupVisual(SPlugTreeOptimGroup *group) {
    if (group->Shader() == nullptr) {
        return nullptr;
    }

    const u32 transformCount = group->TransformCount();
    std::vector<GxVertex> vertices(group->vertexCount);
    std::vector<std::uint16_t> indices(group->indexCount);
    bool needsVertexColor = false;
    bool needsVertexNormal = false;
    u32 vertexBase = 0u;
    u32 indexBase = 0u;

    for (u32 transformIndex = 0u;
         transformIndex < transformCount;
         transformIndex++) {
        SPlugTreeOptimTransf *transform = group->TransformAt(transformIndex);
        CPlugVisual *provider = transform->tree != nullptr
                ? transform->tree->Visual()
                : nullptr;
        if (provider == nullptr) {
            continue;
        }

        needsVertexColor = needsVertexColor || provider->HasVertexColor();
        needsVertexNormal = needsVertexNormal || provider->HasVertexNormal();
        const u32 providerVertexCount = provider->GetTotalVertexCount();
        GxVertex *vertexOut = vertices.data() + vertexBase;
        provider->GetTotalVertexs(vertexOut);
        if (transform->hasIso && providerVertexCount != 0u) {
            TransformVertexRange(vertexOut,
                                 providerVertexCount,
                                 transform->iso);
        }

        unsigned long providerIndexCount = 0ul;
        unsigned short *providerIndices = nullptr;
        provider->GetVertexIndexation(providerIndexCount, providerIndices);
        if (providerIndices != nullptr) {
            std::copy(providerIndices,
                      providerIndices + providerIndexCount,
                      indices.begin() + indexBase);
        }
        for (u32 index = 0u; index < providerIndexCount; index++) {
            indices[indexBase + index] = static_cast<std::uint16_t>(
                    indices[indexBase + index] + vertexBase);
        }
        vertexBase += providerVertexCount;
        indexBase += providerIndexCount;
    }

    vertices.resize(vertexBase);
    indices.resize(indexBase);
    auto visual = std::make_unique<CPlugVisualIndexedTriangles>();
    visual->SetOwnedGeometry(std::move(vertices), std::move(indices));
    visual->EnableVertexColor(needsVertexColor);
    visual->EnableVertexNormal(needsVertexNormal);

    const u32 requiredTexCoordSets = group->Shader()->RequiredTexCoordCount();
    for (u32 texCoordIndex = 0u;
         texCoordIndex < requiredTexCoordSets;
         texCoordIndex++) {
        GxTexCoordDimension mergedDimension = GxTexCoordDimension::Two;
        for (u32 transformIndex = 0u;
             transformIndex < transformCount;
             transformIndex++) {
            const SPlugTreeOptimTransf *transform =
                    group->TransformAt(transformIndex);
            CPlugVisual *provider = transform->tree != nullptr
                    ? transform->tree->Visual()
                    : nullptr;
            if (provider == nullptr ||
                texCoordIndex >= provider->TexCoordSetCount()) {
                continue;
            }
            const GxTexCoordDimension providerDimension =
                    provider->TexCoordSetAt(texCoordIndex).Dimension();
            if (static_cast<unsigned long>(providerDimension) >
                static_cast<unsigned long>(mergedDimension)) {
                mergedDimension = providerDimension;
            }
        }

        GxTexCoordSet mergedSet = GxTexCoordSet::Create(
                mergedDimension, group->texCoordCount);
        u32 texCoordBase = 0u;
        for (u32 transformIndex = 0u;
             transformIndex < transformCount;
             transformIndex++) {
            const SPlugTreeOptimTransf *transform =
                    group->TransformAt(transformIndex);
            CPlugVisual *provider = transform->tree != nullptr
                    ? transform->tree->Visual()
                    : nullptr;
            if (provider == nullptr) {
                continue;
            }

            const u32 providerTexCoordCount =
                    provider->TexCoordVertexCount();
            GxTexCoordSet generated =
                    GxTexCoordSet::Create(mergedDimension, 0u);
            const GxTexCoordSet *sourceSet = nullptr;
            if (texCoordIndex < provider->TexCoordSetCount()) {
                sourceSet = &provider->TexCoordSetAt(texCoordIndex);
            } else {
                provider->GenerateDefaultTexCoord(generated, texCoordIndex);
                sourceSet = &generated;
            }

            const u32 copyCount = std::min(
                    providerTexCoordCount,
                    static_cast<u32>(sourceSet->Count()));
            for (u32 index = 0u; index < copyCount; index++) {
                const u32 destination = texCoordBase + index;
                if (destination < mergedSet.Count()) {
                    mergedSet.SetCoordinate(
                            destination,
                            sourceSet->Coordinate4At(index));
                }
            }
            texCoordBase += providerTexCoordCount;
        }
        visual->AddTexCoordSet(mergedSet);
    }

    if (group->Shader()->RequiresShadowTexCoords()) {
        visual->tangents.assign(group->vertexCount, GmVec3{});
        u32 tangentBase = 0u;
        for (u32 transformIndex = 0u;
             transformIndex < transformCount;
             transformIndex++) {
            const SPlugTreeOptimTransf *transform =
                    group->TransformAt(transformIndex);
            CPlugVisual *provider = transform->tree != nullptr
                    ? transform->tree->Visual()
                    : nullptr;
            if (provider == nullptr) {
                continue;
            }
            const u32 providerVertexCount =
                    provider->GetTotalVertexCount();
            if (provider->TangentData() == nullptr) {
                provider->ComputeTangents(0u, 0, 1, 0);
            }
            const GmVec3 *providerTangents = provider->TangentData();
            if (providerTangents != nullptr) {
                std::copy(providerTangents,
                          providerTangents + providerVertexCount,
                          visual->tangents.begin() + tangentBase);
            }
            tangentBase += providerVertexCount;
        }
    }
    return visual.release();
}

CPlugSurface *CPlugTree::CreateGroupSurface(SPlugTreeOptimGroup *group) {
    CPlugSurface *source = group->Surface();
    if (source == nullptr) {
        return nullptr;
    }

    const SPlugTreeOptimTransf *surfaceTransform = nullptr;
    for (const SPlugTreeOptimTransf &transform : group->transforms) {
        if (transform.tree != nullptr &&
            transform.tree->Surface() == source) {
            surfaceTransform = &transform;
            break;
        }
    }
    if (surfaceTransform == nullptr || !surfaceTransform->hasIso) {
        return source;
    }

    const auto *sourceMesh = dynamic_cast<const GmSurfMesh *>(
            source->Geometry());
    if (sourceMesh == nullptr) {
        return source;
    }

    auto surface = std::make_unique<CPlugSurface>();
    auto geom = std::make_unique<CPlugSurfaceGeom>();
    if (source->GeometryNode() != nullptr) {
        geom->SetId(source->GeometryNode()->Id());
        geom->SetHasDefaultData(
                source->GeometryNode()->HasDefaultData());
    }
    geom->SetGmSurf(std::make_unique<GmSurfMesh>(*sourceMesh));
    geom->TransformByNOMat(surfaceTransform->iso);
    surface->SetGeometry(geom.get());
    geom.release();
    surface->SetMaterialCount(source->MaterialCount());
    for (u32 materialIndex = 0u;
         materialIndex < source->MaterialCount();
         materialIndex++) {
        surface->SetMaterialAt(materialIndex,
                               source->MaterialAt(materialIndex));
    }
    return surface.release();
}

CPlugTree *CPlugTree::MakeGroupTree(
        SPlugTreeOptimGroup *group,
        const SPlugTreeOptimCriteria &criteria) {
    std::unique_ptr<CPlugTree> groupTree;
    SPlugTreeOptimTransf *firstTransform = nullptr;

    switch (group->kind) {
    case SPlugTreeOptimGroup::Kind::MergedGeometry: {
        groupTree = std::make_unique<CPlugTree>();
        groupTree->SetSurface(CreateGroupSurface(group));
        CPlugVisual *visual = CreateGroupVisual(group);
        groupTree->SetVisual(visual, group->Shader(), nullptr, 0);
        if (group->Material() != nullptr) {
            groupTree->SetMaterial(group->Material());
        }
        break;
    }
    case SPlugTreeOptimGroup::Kind::SingleTree:
        firstTransform = group->FirstTransform();
        groupTree.reset(firstTransform->tree->DuplicateThis());
        groupTree->SetUseLocation(firstTransform->hasIso);
        groupTree->SetLocation(firstTransform->iso);
        break;
    case SPlugTreeOptimGroup::Kind::RecursiveTree:
        firstTransform = group->FirstTransform();
        groupTree.reset(firstTransform->tree->DuplicateRecursive());
        groupTree->SetUseLocation(firstTransform->hasIso);
        groupTree->SetLocation(firstTransform->iso);
        break;
    case SPlugTreeOptimGroup::Kind::GeneratedMip:
        groupTree = group->TakeGeneratedTree();
        if (!groupTree) {
            return nullptr;
        }
        break;
    default:
        return nullptr;
    }

    groupTree->UpdateBoundingBox(1);
    if (groupTree->Surface() != nullptr) {
        groupTree->SetCollisionEnabled(true);
    }
    if (criteria.MarksGroupedVisualDirty() &&
        groupTree->Visual() != nullptr) {
        groupTree->Visual()->MarkVisualDirty();
    }
    if (criteria.CopiesGroupProperties()) {
        groupTree->ApplyOptimizationProperties(group->properties);
    }
    return groupTree.release();
}

void CPlugTreeVisualMip::GetMipOptimizedGroups(
        int mip,
        CFastBuffer<SPlugTreeOptimGroup *> &groups,
        const SPlugTreeOptimCriteria &criteria,
        const SPlugTreeOptimTravel &travel) {
    if (mip == 0) {
        CPlugTree::GetOptimizedGroups(groups, criteria, travel);
    } else if (visual == nullptr) {
        (void)CPlugTree::GetChildCount();
    }
}

void CPlugTreeVisualMip::GetOptimizedGroups(
        CFastBuffer<SPlugTreeOptimGroup *> &groups,
        const SPlugTreeOptimCriteria &criteria,
        const SPlugTreeOptimTravel &travel) {
    GetMipOptimizedGroups(0, groups, criteria, travel);
}

std::unique_ptr<CPlugTree> CPlugTree::MakeTreeFromGroups(
        const std::vector<SPlugTreeOptimGroup *> &groups,
        const SPlugTreeOptimCriteria &criteria) {
    if (groups.size() == 1u) {
        return std::unique_ptr<CPlugTree>(
                MakeGroupTree(groups.front(), criteria));
    }

    auto root = std::make_unique<CPlugTree>();
    for (SPlugTreeOptimGroup *group : groups) {
        root->AddOwnedChild(std::unique_ptr<CPlugTree>(
                MakeGroupTree(group, criteria)));
    }
    return root;
}

bool CPlugTree::ShouldForceSingleOptimGroup(
        const SPlugTreeOptimCriteria &criteria) const {
    CPlugVisual *visual = Visual();
    return IsOptimizable(criteria) == 0 ||
           (visual != nullptr && visual->GetVertexIndexCount() == 0u);
}

void CPlugTree::GetOptimizedGroups(
        CFastBuffer<SPlugTreeOptimGroup *> &groups,
        const SPlugTreeOptimCriteria &criteria,
        const SPlugTreeOptimTravel &travel) {
    std::vector<SPlugTreeOptimGroup *> &groupList = groups.Values();
    SPlugTreeOptimTravel localTravel = travel.WithTreeLocalIso(*this);
    const bool forceSingle = ShouldForceSingleOptimGroup(criteria);
    if (forceSingle) {
        SPlugTreeOptimGroup &single = localTravel.CreateGroup(groupList);
        single.InitAsSingleTree(*this, localTravel);
    }

    bool generatorHandled = false;
    if (Generator() != nullptr) {
        Generator()->GetOptimizedGroups(groups, criteria, localTravel, this);
        generatorHandled = true;
    } else if (Shader() != nullptr || Surface() != nullptr) {
        SPlugTreeOptimGroup *matchedGroup = nullptr;
        for (SPlugTreeOptimGroup *candidate : groupList) {
            if (IsEqual(candidate, criteria, localTravel) != 0) {
                matchedGroup = candidate;
                break;
            }
        }

        if (matchedGroup != nullptr) {
            matchedGroup->AddTreeTransform(*this, localTravel);
            matchedGroup->AdoptMissingTreeState(*this);
        } else {
            SPlugTreeOptimGroup &newGroup =
                    localTravel.CreateGroup(groupList);
            newGroup.InitAsMergeTree(*this, localTravel);
        }
    }

    const u32 childCount = GetChildCount();
    if (childCount == 0u) {
        return;
    }

    PlugTreeOptimizationBatch nearBatch(10ul);
    PlugTreeOptimizationBatch farBatch(10ul);
    CFastBuffer<SPlugTreeOptimGroup *> nearGroups;
    CFastBuffer<SPlugTreeOptimGroup *> farGroups;
    nearGroups.reserve(10ul);
    farGroups.reserve(10ul);
    float splitFarZ = 0.0f;
    SPlugTreeOptimProperties splitProperties{};

    for (u32 childIndex = 0u; childIndex < childCount; childIndex++) {
        CPlugTree *child = GetChild(childIndex);
        if (generatorHandled && child->GetIsRooted() == 0) {
            continue;
        }

        auto *mip = dynamic_cast<CPlugTreeVisualMip *>(child);
        if (mip == nullptr) {
            child->GetOptimizedGroups(groups, criteria, localTravel);
            continue;
        }

        const unsigned long mipChildCount = mip->LevelCount();
        CPlugTree *lastMipChild = mipChildCount != 0u
                ? mip->LevelTree(mipChildCount - 1ul)
                : nullptr;
        if (mipChildCount <= 1u || lastMipChild == nullptr ||
            lastMipChild->IsOptimizable(criteria) == 0) {
            child->GetOptimizedGroups(groups, criteria, localTravel);
            continue;
        }

        mip->GetMipOptimizedGroups(1, groups, criteria, localTravel);
        const float candidateFarZ =
                mip->LevelFarZ(mipChildCount - 2ul);
        if (!nearGroups.empty()) {
            splitFarZ = std::max(splitFarZ, candidateFarZ);
        } else {
            splitFarZ = candidateFarZ;
            splitProperties = child->OptimizationProperties();
        }

        SPlugTreeOptimTravel childTravel =
                localTravel.WithTreeLocalIso(*child);
        childTravel = childTravel.WithOptimizationBatch(nearBatch);
        lastMipChild->GetOptimizedGroups(
                nearGroups, criteria, childTravel);
        SPlugTreeOptimTravel farTravel =
                localTravel.WithOptimizationBatch(farBatch);
        mip->GetMipOptimizedGroups(0, farGroups, criteria, farTravel);
    }

    if (nearGroups.empty()) {
        return;
    }

    auto splitMip = std::make_unique<CPlugTreeVisualMip>();
    splitMip->AddOwnedLevel(
            MakeTreeFromGroups(farGroups.Values(), criteria), splitFarZ);
    std::unique_ptr<CPlugTree> nearTree =
            MakeTreeFromGroups(nearGroups.Values(), criteria);
    CPlugTree *nearTreeView = nearTree.get();
    splitMip->AddOwnedLevel(
            std::move(nearTree), std::numeric_limits<float>::max());
    nearTreeView->SetCollisionEnabled(false);

    SPlugTreeOptimGroup &mipGroup = localTravel.CreateGroup(groupList);
    mipGroup.InitAsGeneratedMip(std::move(splitMip), splitProperties);
}

void CPlugTree::FoldChildState(const CPlugTree &child) {
    state_.collisionEnabled =
            state_.collisionEnabled || child.state_.collisionEnabled;
    state_.visible = state_.visible || child.state_.visible;
    state_.castsShadows = state_.castsShadows || child.state_.castsShadows;
    state_.requiresMatchingShaderState =
            state_.requiresMatchingShaderState ||
            child.state_.requiresMatchingShaderState;
    state_.requiresMatchingMaterialState =
            state_.requiresMatchingMaterialState ||
            child.state_.requiresMatchingMaterialState;
    state_.modelInstance = state_.modelInstance || child.state_.modelInstance;
}

void CPlugTree::GenerateOptimizedTree(
        CPlugTree *&outTree,
        const SPlugTreeOptimCriteria &criteria) {
    PlugTreeOptimizationBatch batch(50ul);
    CFastBuffer<SPlugTreeOptimGroup *> groups;
    groups.reserve(50ul);
    SPlugTreeOptimTravel travel(batch);
    travel.iso.SetIdentity();
    GetOptimizedGroups(groups, criteria, travel);

    std::unique_ptr<CPlugTree> optimizedTree;
    if (groups.size() == 1u) {
        optimizedTree.reset(MakeGroupTree(groups.front(), criteria));
    } else {
        auto root = std::make_unique<CPlugTree>();
        root->SetIsRooted(1);
        for (SPlugTreeOptimGroup *group : groups) {
            std::unique_ptr<CPlugTree> child(
                    MakeGroupTree(group, criteria));
            root->FoldChildState(*child);
            root->AddOwnedChild(std::move(child));
        }
        optimizedTree = std::move(root);
    }

    if (criteria.NeedsProviderPostProcess()) {
        unsigned long cursor = optimizedTree->GetAllTreeStart();
        while (cursor != InvalidEngineIndex) {
            CPlugTree *tree = optimizedTree->GetAllTreeNext(cursor);
            CPlugVisual *provider = tree->Visual();
            if (provider == nullptr || tree->ModelSolidReference() != nullptr) {
                continue;
            }

            if (criteria.WantsSelfWeld()) {
                if (tree->Shader() != nullptr) {
                    const CPlugShader *shader = tree->Shader();
                    provider->UpdateVisualFromShaderRequirement(shader, nullptr);
                }
                provider->SelfWeld(criteria.SelfWeldAngle(),
                                   criteria.SelfWeldDistance(),
                                   criteria.SelfWeldPrecision(),
                                   criteria.SelfWeldTolerance(),
                                   criteria.SelfWeldFlags());
                if (provider->HasEmptyIndexBuffer()) {
                    tree->SetIsVisible(0);
                    continue;
                }
            }

            if (criteria.WantsProviderOptimization()) {
                CPlugVisual *newProvider = provider->Optimize(
                        criteria.ProviderOptimizeMode(), 16u);
                if (newProvider != nullptr) {
                    CPlugMaterial *material = tree->Material();
                    tree->SetVisual(newProvider, tree->Shader(), nullptr, 0);
                    if (material != nullptr) {
                        tree->SetMaterial(material);
                    }
                }
            }
        }
    }
    optimizedTree->UpdateBoundingBox(1);
    outTree = optimizedTree.release();
}
