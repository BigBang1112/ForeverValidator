#include "format/static_solid/static_solid_archive_graph.h"
#include <stddef.h>

#include "format/static_solid/static_scene_archive_loader.h"
#include "format/static_solid/static_solid_descriptor_dependency_queue.h"
#include "format/static_solid/static_solid_archive_node_kind.h"
#include "format/archive/tmnf_archive_ids.h"
#include "format/archive/fixed_c_string.h"
#include <new>
namespace {

}  // namespace

int CGameCtnReplayStaticSolidArchiveTreeIdName::Install(
        CGameCtnReplayStaticSolidArchiveNodeIdentity tree,
        const char *newName) {
    *this = CGameCtnReplayStaticSolidArchiveTreeIdName{};
    this->tree = tree;
    if (newName == nullptr || newName[0] == '\0') {
        return 1;
    }
    (TmnfFormat::FixedCString::CopyTruncated((name), (sizeof(name)), (newName)));
    return 1;
}

int CGameCtnReplayStaticSolidArchiveTreeIdName::Matches(
        CGameCtnReplayStaticSolidArchiveNodeIdentity tree) const {
    return this->tree.Matches(tree);
}

int CGameCtnReplayStaticSolidArchiveTreeIdName::HasName() const {
    return name[0] != '\0';
}

const char *CGameCtnReplayStaticSolidArchiveTreeIdName::Name() const {
    return name;
}

void CGameCtnReplayStaticSolidArchiveTreePath::Clear() {
    childSlots.fill(0u);
    childSlotCount = 0u;
}

int CGameCtnReplayStaticSolidArchiveTreePath::Assign(const u32 *path,
                                                     u32 length) {
    if (length > StaticSolidArchiveTreePathCapacity ||
        (length != 0u && path == nullptr)) {
        return 0;
    }
    Clear();
    for (u32 i = 0u; i < length; i++) {
        childSlots[i] = path[i];
    }
    childSlotCount = length;
    return 1;
}

u32 CGameCtnReplayStaticSolidArchiveTreePath::Length() const {
    return childSlotCount;
}

int CGameCtnReplayStaticSolidArchiveTreePath::CopyTo(
        u32 *path,
        u32 capacity,
        u32 *lengthOut) const {
    if (path == nullptr || lengthOut == nullptr ||
        capacity < childSlotCount) {
        return 0;
    }
    for (u32 i = 0u; i < childSlotCount; i++) {
        path[i] = childSlots[i];
    }
    *lengthOut = childSlotCount;
    return 1;
}

void CGameCtnReplayStaticSolidArchiveNamedTree::Install(
        CGameCtnReplayStaticSolidArchiveNodeIdentity treeIn,
        const char *nameIn,
        const CGameCtnReplayStaticSolidArchiveTreeStateDefinition *stateIn,
        const CGameCtnReplayStaticSolidArchiveSurfaceGeometryDefinition *geomIn) {
    tree = treeIn;
    name = nameIn;
    state = stateIn;
    geom = geomIn;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveNamedTree::Tree() const {
    return tree;
}

const char *CGameCtnReplayStaticSolidArchiveNamedTree::Name() const {
    return name;
}

const CGameCtnReplayStaticSolidArchiveTreeStateDefinition *
CGameCtnReplayStaticSolidArchiveNamedTree::State() const {
    return state;
}

const CGameCtnReplayStaticSolidArchiveSurfaceGeometryDefinition *
CGameCtnReplayStaticSolidArchiveNamedTree::SurfaceGeom() const {
    return geom;
}

int CGameCtnReplayStaticSolidArchiveNamedTree::
HasSurfaceGeometry() const {
    return state != nullptr && geom != nullptr && geom->SurfType() == 1u;
}

void CGameCtnReplayStaticSolidArchiveNode::Install(
        CGameCtnReplayStaticSolidArchiveNodeIdentity identity,
        u32 newClassId) {
    *this = CGameCtnReplayStaticSolidArchiveNode{};
    this->identity = identity;
    classId = newClassId;
}

void CGameCtnReplayStaticSolidArchiveNode::SetTreeId(const CMwId &id) {
    hasTreeId = 1u;
    treeId = id;
}

int CGameCtnReplayStaticSolidArchiveNode::Matches(
        CGameCtnReplayStaticSolidArchiveNodeIdentity identity) const {
    return this->identity.Matches(identity);
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveNode::Identity() const {
    return identity;
}

u32 CGameCtnReplayStaticSolidArchiveNode::ClassId() const {
    return classId;
}

int CGameCtnReplayStaticSolidArchiveNode::IsSolid() const {
    return classId == TMNF_CLASS_CPlugSolid;
}

int CGameCtnReplayStaticSolidArchiveNode::IsTree() const {
    return StaticSolidArchiveNodeKind::IsTree(classId);
}

int CGameCtnReplayStaticSolidArchiveNode::IsSurface() const {
    return StaticSolidArchiveNodeKind::IsSurface(classId);
}

int CGameCtnReplayStaticSolidArchiveNode::IsSurfaceGeom() const {
    return StaticSolidArchiveNodeKind::IsSurfaceGeom(classId);
}

int CGameCtnReplayStaticSolidArchiveNode::IsMaterial() const {
    return StaticSolidArchiveNodeKind::IsMaterial(classId);
}

int CGameCtnReplayStaticSolidArchiveNode::IsVisualIndexedTriangles() const {
    return StaticSolidArchiveNodeKind::IsVisualIndexedTriangles(classId);
}

int CGameCtnReplayStaticSolidArchiveNode::HasTreeId() const {
    return hasTreeId != 0u;
}

CMwId CGameCtnReplayStaticSolidArchiveNode::TreeId() const {
    return treeId;
}

void CGameCtnReplayStaticSolidArchiveTreeChildLink::Install(
        CGameCtnReplayStaticSolidArchiveNodeIdentity parentTree,
        CGameCtnReplayStaticSolidArchiveNodeIdentity childTree,
        u32 newLinkKind) {
    *this = CGameCtnReplayStaticSolidArchiveTreeChildLink{};
    this->parentTree = parentTree;
    this->childTree = childTree;
    linkKind = newLinkKind;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveTreeChildLink::ParentTree() const {
    return parentTree;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveTreeChildLink::ChildTree() const {
    return childTree;
}

u32 CGameCtnReplayStaticSolidArchiveTreeChildLink::LinkKind() const {
    return linkKind;
}

void CGameCtnReplayStaticSolidArchiveTreeSurfaceLink::Install(
        CGameCtnReplayStaticSolidArchiveNodeIdentity tree,
        CGameCtnReplayStaticSolidArchiveNodeIdentity surface) {
    *this = CGameCtnReplayStaticSolidArchiveTreeSurfaceLink{};
    this->tree = tree;
    this->surface = surface;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveTreeSurfaceLink::Tree() const {
    return tree;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveTreeSurfaceLink::Surface() const {
    return surface;
}

void CGameCtnReplayStaticSolidDecoratorTreeDeclaration::Reset(
        CGameCtnReplayStaticSolidArchiveNodeIdentity decorator,
        u32 newChunkId) {
    *this = CGameCtnReplayStaticSolidDecoratorTreeDeclaration{};
    this->decorator = decorator;
    chunkId = newChunkId;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidDecoratorTreeDeclaration::Decorator() const {
    return decorator;
}

void CGameCtnReplayStaticSolidDecoratorTreeDeclaration::SetTreeId(CMwId id) {
    treeId = id;
}

void CGameCtnReplayStaticSolidDecoratorTreeDeclaration::SetMaterialNode(
        ArchiveNodeReference nodeRef) {
    material =
            CGameCtnReplayStaticSolidArchiveNodeIdentity::FromPayloadAndLocalNode(
                    decorator.Payload(),
                    nodeRef);
}

void CGameCtnReplayStaticSolidDecoratorTreeDeclaration::SetVisualNode(
        ArchiveNodeReference nodeRef) {
    visual =
            CGameCtnReplayStaticSolidArchiveNodeIdentity::FromPayloadAndLocalNode(
                    decorator.Payload(),
                    nodeRef);
}

void CGameCtnReplayStaticSolidDecoratorTreeDeclaration::SetSolidSurfaceFromVisual(
        u32 enabled) {
    solidSurfaceFromVisual = enabled;
}

void CGameCtnReplayStaticSolidDecoratorTreeDeclaration::SetVisibilityConditions(
        u32 show,
        u32 visible,
        u32 applyVisibleChildren,
        u32 caster,
        u32 applyCasterChildren,
        u32 solidFromVisual) {
    showCondition = show;
    visibleCondition = visible;
    applyVisibleToChildren = applyVisibleChildren;
    casterCondition = caster;
    applyCasterToChildren = applyCasterChildren;
    solidSurfaceFromVisual = solidFromVisual;
}

void CGameCtnReplayStaticSolidDecoratorTreeDeclaration::
        SetNearlyEqualIdentityCondition(u32 enabled) {
    nearlyEqualIdentityCondition = enabled;
}

void CGameCtnReplayStaticSolidDecoratorTreeDeclaration::SetCollisionCondition(
        u32 condition) {
    collisionCondition = condition;
}

int CGameCtnReplayStaticSolidDecoratorTreeDeclaration::SetExternalDescriptorPaths(
        const char *plainPath,
        const char *selectedPath) {
    if (plainPath != nullptr &&
        !(TmnfFormat::FixedCString::Copy((sourcePlainPath), (sizeof(sourcePlainPath)), (plainPath)))) {
        return 0;
    }
    if (selectedPath != nullptr &&
        !(TmnfFormat::FixedCString::Copy((sourceSelectedDescriptorPath), (sizeof(sourceSelectedDescriptorPath)), (selectedPath)))) {
        return 0;
    }
    return 1;
}

CMwId CGameCtnReplayStaticSolidDecoratorTreeDeclaration::TreeId() const {
    return treeId;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidDecoratorTreeDeclaration::Material() const {
    return material;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidDecoratorTreeDeclaration::Visual() const {
    return visual;
}

u32 CGameCtnReplayStaticSolidDecoratorTreeDeclaration::ChunkId() const {
    return chunkId;
}

int CGameCtnReplayStaticSolidDecoratorTreeDeclaration::HasSelectedDescriptorPath()
        const {
    return sourceSelectedDescriptorPath[0] != '\0';
}

const char *CGameCtnReplayStaticSolidDecoratorTreeDeclaration::SelectedDescriptorPath()
        const {
    return sourceSelectedDescriptorPath;
}

const char *CGameCtnReplayStaticSolidDecoratorTreeDeclaration::PlainDescriptorPath()
        const {
    return sourcePlainPath;
}

int CGameCtnReplayStaticSolidDecoratorTreeDeclaration::IsConditionEnabled(
        u32 condition,
        u32 quality) {
    switch (condition) {
        case 1u:
            return quality == 0u;
        case 2u:
            return quality <= 1u;
        case 3u:
            return quality == 1u;
        case 4u:
            return quality == 1u || quality == 2u;
        case 5u:
            return quality == 2u;
        case 6u:
            return 1;
        default:
            return 0;
    }
}

int CGameCtnReplayStaticSolidDecoratorTreeDeclaration::IsShownAtQuality(
        u32 quality) const {
    return IsConditionEnabled(showCondition, quality);
}

int CGameCtnReplayStaticSolidDecoratorTreeDeclaration::UsesVisualSurfaceAtQuality(
        u32 quality) const {
    return solidSurfaceFromVisual != 0u && IsShownAtQuality(quality);
}

bool CGameCtnReplayStaticSolidDecoratorTreeDeclaration::IsVisibleAtQuality(
        u32 quality) const {
    return IsConditionEnabled(visibleCondition, quality) != 0;
}

bool CGameCtnReplayStaticSolidDecoratorTreeDeclaration::CastsShadowsAtQuality(
        u32 quality) const {
    return IsConditionEnabled(casterCondition, quality) != 0;
}

bool CGameCtnReplayStaticSolidDecoratorTreeDeclaration::HasCollisionAtQuality(
        u32 quality) const {
    return IsConditionEnabled(collisionCondition, quality) != 0;
}

int CGameCtnReplayStaticSolidDecoratorTreeDeclaration::AppliesVisibleToChildren()
        const {
    return applyVisibleToChildren != 0u;
}

int CGameCtnReplayStaticSolidDecoratorTreeDeclaration::AppliesCasterToChildren()
        const {
    return applyCasterToChildren != 0u;
}

void CGameCtnReplayStaticSolidDecoratorTrees::Clear() {
    declarations.clear();
}

u32 CGameCtnReplayStaticSolidDecoratorTrees::Count() const {
    return declarations.size() <= UINT32_MAX ? (u32)declarations.size()
                                             : UINT32_MAX;
}

int CGameCtnReplayStaticSolidDecoratorTrees::Empty() const {
    return declarations.empty();
}

CGameCtnReplayStaticSolidDecoratorTreeCursor
CGameCtnReplayStaticSolidDecoratorTrees::End() const {
    return CGameCtnReplayStaticSolidDecoratorTreeCursor::FromDeclarationCount(
            Count());
}

const CGameCtnReplayStaticSolidDecoratorTreeDeclaration *
CGameCtnReplayStaticSolidDecoratorTrees::At(u32 index) const {
    return index < declarations.size() ? &declarations[index] : nullptr;
}

int CGameCtnReplayStaticSolidDecoratorTrees::AddParsedTree(
        const CGameCtnReplayStaticSolidDecoratorTreeDeclaration &declaration) {
    if (declarations.size() >= UINT32_MAX) {
        return 0;
    }
    try {
        declarations.push_back(declaration);
        return 1;
    } catch (const std::bad_alloc &) {
        return 0;
    }
}

int CGameCtnReplayStaticSolidDecoratorTrees::ResizePrefix(
        CGameCtnReplayStaticSolidDecoratorTreeCursor cursor) {
    const u32 count = cursor.DeclarationCountForGraphLookup();
    if (count > declarations.size()) {
        return 0;
    }
    declarations.resize(count);
    return 1;
}

int CGameCtnReplayStaticSolidDecoratorTrees::
        RequestExternalDescriptorDependencies(
                const StaticSolidArchiveLoadSession *archive,
                CGameCtnReplayStaticSolidDescriptorDependencyQueue *dependencyQueue,
                CGameCtnReplayStaticSolidDecoratorTreeCursor first) const {
    if (dependencyQueue == nullptr) {
        return 0;
    }
    const u32 count = Count();
    for (u32 index = first.DeclarationCountForGraphLookup(); index < count; index++) {
        const CGameCtnReplayStaticSolidDecoratorTreeDeclaration *dependency =
                At(index);
        if (dependency == nullptr ||
            !dependencyQueue->RequireMissingDescriptor(
                    archive,
                    dependency->SelectedDescriptorPath())) {
            return 0;
        }
    }
    return 1;
}

void CGameCtnReplayStaticSolidArchiveTreeGraph::Clear() {
    childLinks.Clear();
    treeStates.Clear();
    solidRootLinks.Clear();
    solidPhysicalDefinitions.Clear();
    treeSurfaceLinks.Clear();
    treeSourceLinks.Clear();
}

u32 CGameCtnReplayStaticSolidArchiveTreeGraph::ChildLinkCount() const {
    return childLinks.Count();
}

u32 CGameCtnReplayStaticSolidArchiveTreeGraph::TreeStateCount() const {
    return treeStates.Count();
}

u32 CGameCtnReplayStaticSolidArchiveTreeGraph::SolidRootLinkCount() const {
    return solidRootLinks.Count();
}

u32 CGameCtnReplayStaticSolidArchiveTreeGraph::
        SolidPhysicalDefinitionCount() const {
    return solidPhysicalDefinitions.Count();
}

u32 CGameCtnReplayStaticSolidArchiveTreeGraph::TreeSurfaceLinkCount()
        const {
    return treeSurfaceLinks.Count();
}

u32 CGameCtnReplayStaticSolidArchiveTreeGraph::TreeSourceLinkCount() const {
    return treeSourceLinks.Count();
}

int CGameCtnReplayStaticSolidArchiveTreeGraph::HasSolidRootLinks() const {
    return solidRootLinks.Count() != 0u;
}

int CGameCtnReplayStaticSolidArchiveTreeGraph::HasTreeSourceLinks() const {
    return treeSourceLinks.Count() != 0u;
}

int CGameCtnReplayStaticSolidArchiveTreeGraph::AddChildLink(
        const CGameCtnReplayStaticSolidArchiveTreeChildLink &link) {
    return childLinks.Append(link);
}

int CGameCtnReplayStaticSolidArchiveTreeGraph::AddTreeState(
        const CGameCtnReplayStaticSolidArchiveTreeStateDefinition &definition) {
    return treeStates.Append(definition);
}

int CGameCtnReplayStaticSolidArchiveTreeGraph::AddSolidRootLink(
        const CGameCtnReplayStaticSolidArchiveSolidTreeLink &link) {
    return solidRootLinks.Append(link);
}

int CGameCtnReplayStaticSolidArchiveTreeGraph::AddSolidPhysicalDefinition(
        const CGameCtnReplayStaticSolidArchiveSolidPhysicsDefinition
                &definition) {
    return solidPhysicalDefinitions.Append(definition);
}

int CGameCtnReplayStaticSolidArchiveTreeGraph::AddTreeSurfaceLink(
        const CGameCtnReplayStaticSolidArchiveTreeSurfaceLink &link) {
    return treeSurfaceLinks.Append(link);
}

int CGameCtnReplayStaticSolidArchiveTreeGraph::AddTreeSourceLink(
        const CGameCtnReplayStaticSolidArchiveTreeSourceLink &link) {
    return treeSourceLinks.Append(link);
}

int CGameCtnReplayStaticSolidArchiveTreeGraph::TruncateChildLinks(
        u32 count) {
    return childLinks.ResizePrefix(count);
}

int CGameCtnReplayStaticSolidArchiveTreeGraph::TruncateTreeStates(
        u32 count) {
    return treeStates.ResizePrefix(count);
}

int CGameCtnReplayStaticSolidArchiveTreeGraph::TruncateSolidRootLinks(
        u32 count) {
    return solidRootLinks.ResizePrefix(count);
}

int CGameCtnReplayStaticSolidArchiveTreeGraph::
        TruncateSolidPhysicalDefinitions(u32 count) {
    return solidPhysicalDefinitions.ResizePrefix(count);
}

int CGameCtnReplayStaticSolidArchiveTreeGraph::TruncateTreeSurfaceLinks(
        u32 count) {
    return treeSurfaceLinks.ResizePrefix(count);
}

int CGameCtnReplayStaticSolidArchiveTreeGraph::TruncateTreeSourceLinks(
        u32 count) {
    return treeSourceLinks.ResizePrefix(count);
}

void CGameCtnReplayStaticSolidArchiveSurfaceGraph::Clear() {
    surfaceGeometryLinks.Clear();
    surfaceMaterialEntries.Clear();
    materialDefinitions.Clear();
    surfaceGeometryDefinitions.Clear();
    visualProviderDefinitions.Clear();
}

u32 CGameCtnReplayStaticSolidArchiveSurfaceGraph::
        SurfaceGeometryLinkCount() const {
    return surfaceGeometryLinks.Count();
}

u32 CGameCtnReplayStaticSolidArchiveSurfaceGraph::
        SurfaceMaterialEntryCount() const {
    return surfaceMaterialEntries.Count();
}

u32 CGameCtnReplayStaticSolidArchiveSurfaceGraph::MaterialDefinitionCount()
        const {
    return materialDefinitions.Count();
}

u32 CGameCtnReplayStaticSolidArchiveSurfaceGraph::
        SurfaceGeometryDefinitionCount() const {
    return surfaceGeometryDefinitions.Count();
}

u32 CGameCtnReplayStaticSolidArchiveSurfaceGraph::
        VisualProviderDefinitionCount() const {
    return visualProviderDefinitions.Count();
}

int CGameCtnReplayStaticSolidArchiveSurfaceGraph::HasMaterialDefinitions()
        const {
    return materialDefinitions.Count() != 0u;
}

int CGameCtnReplayStaticSolidArchiveSurfaceGraph::AddSurfaceGeometryLink(
        const CGameCtnReplayStaticSolidArchiveSurfaceGeomLink &link) {
    return surfaceGeometryLinks.Append(link);
}

int CGameCtnReplayStaticSolidArchiveSurfaceGraph::AddSurfaceMaterialEntry(
        const CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry &entry) {
    return surfaceMaterialEntries.Append(entry);
}

int CGameCtnReplayStaticSolidArchiveSurfaceGraph::AddMaterialDefinition(
        const CGameCtnReplayStaticSolidArchiveMaterialDefinition &definition) {
    return materialDefinitions.Append(definition);
}

int CGameCtnReplayStaticSolidArchiveSurfaceGraph::AddSurfaceGeometryDefinition(
        const CGameCtnReplayStaticSolidArchiveSurfaceGeometryDefinition &definition) {
    return surfaceGeometryDefinitions.Append(definition);
}

static CGameCtnReplayStaticSolidArchiveVisualGeometryDefinition *
FindMutableVisualProviderDefinition(
        CGameCtnReplayStaticSolidArchiveSequence<
                CGameCtnReplayStaticSolidArchiveVisualGeometryDefinition>
                *definitions,
        CGameCtnReplayStaticSolidArchiveNodeIdentity visual) {
    if (definitions == nullptr) {
        return nullptr;
    }
    for (u32 i = definitions->Count(); i != 0u; i--) {
        CGameCtnReplayStaticSolidArchiveVisualGeometryDefinition *definition =
                definitions->MutableAt(i - 1u);
        if (definition == nullptr) {
            continue;
        }
        if (definition->MatchesVisualProvider(visual)) {
            return definition;
        }
    }
    return nullptr;
}

int CGameCtnReplayStaticSolidArchiveSurfaceGraph::
        AddVisualProviderDefinition(
                const CGameCtnReplayStaticSolidArchiveVisualGeometryDefinition
                        &definition) {
    return visualProviderDefinitions.Append(definition);
}

int CGameCtnReplayStaticSolidArchiveSurfaceGraph::
        UpdateVisualProviderDefinition(
                const CGameCtnReplayStaticSolidArchiveVisualGeometryDefinition
                        &newDefinition) {
    CGameCtnReplayStaticSolidArchiveVisualGeometryDefinition *definition =
            FindMutableVisualProviderDefinition(&visualProviderDefinitions,
                                                newDefinition.VisualProvider());
    if (definition != nullptr) {
        *definition = newDefinition;
    }
    return 1;
}

int CGameCtnReplayStaticSolidArchiveSurfaceGraph::
        TruncateSurfaceGeometryLinks(u32 count) {
    return surfaceGeometryLinks.ResizePrefix(count);
}

int CGameCtnReplayStaticSolidArchiveSurfaceGraph::
        TruncateSurfaceMaterialEntries(u32 count) {
    return surfaceMaterialEntries.ResizePrefix(count);
}

int CGameCtnReplayStaticSolidArchiveSurfaceGraph::
        TruncateMaterialDefinitions(u32 count) {
    return materialDefinitions.ResizePrefix(count);
}

int CGameCtnReplayStaticSolidArchiveSurfaceGraph::
        TruncateSurfaceGeometryDefinitions(u32 count) {
    return surfaceGeometryDefinitions.ResizePrefix(count);
}

int CGameCtnReplayStaticSolidArchiveSurfaceGraph::
        TruncateVisualProviderDefinitions(u32 count) {
    return visualProviderDefinitions.ResizePrefix(count);
}

void CGameCtnReplayStaticSolidArchiveTreeSourceLink::Install(
        CGameCtnReplayStaticSolidArchiveNodeIdentity newTree,
        CGameCtnReplayStaticSolidArchiveNodeIdentity newVisual,
        CGameCtnReplayStaticSolidArchiveNodeIdentity newShader,
        CGameCtnReplayStaticSolidArchiveNodeIdentity newMaterial,
        CGameCtnReplayStaticSolidArchiveNodeIdentity newSurfaceSource,
        CGameCtnReplayStaticSolidArchiveNodeIdentity newGenerator,
        u32 newChunkId) {
    *this = CGameCtnReplayStaticSolidArchiveTreeSourceLink{};
    tree = newTree;
    visual = newVisual;
    shader = newShader;
    material = newMaterial;
    surfaceSource = newSurfaceSource;
    generator = newGenerator;
    chunkId = newChunkId;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveTreeSourceLink::Tree() const {
    return tree;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveTreeSourceLink::Visual() const {
    return visual;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveTreeSourceLink::Shader() const {
    return shader;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveTreeSourceLink::Material() const {
    return material;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveTreeSourceLink::SurfaceSource() const {
    return surfaceSource;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveTreeSourceLink::Generator() const {
    return generator;
}

int CGameCtnReplayStaticSolidArchiveTreeSourceLink::HasVisual()
        const {
    return visual.IsValid();
}

int CGameCtnReplayStaticSolidArchiveTreeSourceLink::HasMaterialNode() const {
    return material.IsValid();
}

int CGameCtnReplayStaticSolidArchiveTreeSourceLink::HasShaderNode() const {
    return shader.IsValid();
}

int CGameCtnReplayStaticSolidArchiveTreeSourceLink::HasMaterialOrShader()
        const {
    return HasMaterialNode() || HasShaderNode();
}

int CGameCtnReplayStaticSolidArchiveTreeSourceLink::HasSurfaceSource() const {
    return surfaceSource.IsValid();
}

int CGameCtnReplayStaticSolidArchiveTreeSourceLink::HasGenerator() const {
    return generator.IsValid();
}

u32 CGameCtnReplayStaticSolidArchiveTreeSourceLink::ChunkId() const {
    return chunkId;
}

void CGameCtnReplayStaticSolidArchiveSolidTreeLink::Install(
        CGameCtnReplayStaticSolidArchiveNodeIdentity solid,
        CGameCtnReplayStaticSolidArchiveNodeIdentity tree,
        u32 newChunkId) {
    *this = CGameCtnReplayStaticSolidArchiveSolidTreeLink{};
    this->solid = solid;
    this->tree = tree;
    chunkId = newChunkId;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveSolidTreeLink::Solid() const {
    return solid;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveSolidTreeLink::Tree() const {
    return tree;
}

u32 CGameCtnReplayStaticSolidArchiveSolidTreeLink::ChunkId() const {
    return chunkId;
}

int CGameCtnReplayStaticSolidArchiveSolidTreeLink::MatchesPayload(
        StaticSolidArchiveId payload) const {
    return solid.MatchesPayload(payload);
}

void CGameCtnReplayStaticSolidArchiveSurfaceGeomLink::Install(
        CGameCtnReplayStaticSolidArchiveNodeIdentity surface,
        CGameCtnReplayStaticSolidArchiveNodeIdentity geom,
        u32 newMaterialEntryCount,
        u32 newExplicitMaterialRefCount,
        u32 newDefaultMaterialIdCount) {
    *this = CGameCtnReplayStaticSolidArchiveSurfaceGeomLink{};
    this->surface = surface;
    this->geom = geom;
    materialEntryCount = newMaterialEntryCount;
    explicitMaterialRefCount = newExplicitMaterialRefCount;
    defaultMaterialIdCount = newDefaultMaterialIdCount;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveSurfaceGeomLink::Surface() const {
    return surface;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveSurfaceGeomLink::Geom() const {
    return geom;
}

u32 CGameCtnReplayStaticSolidArchiveSurfaceGeomLink::MaterialEntryCount()
        const {
    return materialEntryCount;
}

u32 CGameCtnReplayStaticSolidArchiveSurfaceGeomLink::ExplicitMaterialRefCount()
        const {
    return explicitMaterialRefCount;
}

u32 CGameCtnReplayStaticSolidArchiveSurfaceGeomLink::DefaultMaterialIdCount()
        const {
    return defaultMaterialIdCount;
}

void CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry::
        InstallExplicitRef(
                CGameCtnReplayStaticSolidArchiveNodeIdentity surface,
                u32 newMaterialSlotIndex,
                CGameCtnReplayStaticSolidArchiveNodeIdentity material) {
    *this = CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry{};
    this->surface = surface;
    materialSlotIndex = newMaterialSlotIndex;
    usesExplicitRef = 1u;
    this->material = material;
}

void CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry::
        InstallDefaultMaterial(
                CGameCtnReplayStaticSolidArchiveNodeIdentity surface,
                u32 newMaterialSlotIndex,
                uint16_t newDefaultMaterialId) {
    *this = CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry{};
    this->surface = surface;
    materialSlotIndex = newMaterialSlotIndex;
    defaultMaterialId = newDefaultMaterialId;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry::Surface() const {
    return surface;
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry::Material() const {
    return material;
}

int CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry::UsesExplicitRef()
        const {
    return usesExplicitRef != 0u;
}

u32 CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry::MaterialSlotIndex()
        const {
    return materialSlotIndex;
}

uint16_t CGameCtnReplayStaticSolidArchiveSurfaceMaterialEntry::
        DefaultMaterialId() const {
    return defaultMaterialId;
}

void CGameCtnReplayStaticSolidArchiveNodeDirectory::Clear() {
    nodes.Clear();
    treeIdNames.Clear();
}

u32 CGameCtnReplayStaticSolidArchiveNodeDirectory::Count() const {
    return nodes.Count();
}

int CGameCtnReplayStaticSolidArchiveNodeDirectory::Empty() const {
    return nodes.Count() == 0u;
}

const CGameCtnReplayStaticSolidArchiveNode *
CGameCtnReplayStaticSolidArchiveNodeDirectory::At(u32 index) const {
    return nodes.At(index);
}

int CGameCtnReplayStaticSolidArchiveNodeDirectory::Add(
        const CGameCtnReplayStaticSolidArchiveNode &node) {
    return nodes.Append(node);
}

int CGameCtnReplayStaticSolidArchiveNodeDirectory::ResizePrefix(u32 count) {
    return nodes.ResizePrefix(count);
}

u32 CGameCtnReplayStaticSolidArchiveNodeDirectory::FindRecord(
        CGameCtnReplayStaticSolidArchiveNodeIdentity identity) const {
    for (u32 i = 0; i < nodes.Count(); i++) {
        const CGameCtnReplayStaticSolidArchiveNode *node = nodes.At(i);
        if (node != nullptr && node->Matches(identity)) {
            return i;
        }
    }
    return UINT32_MAX;
}

const CGameCtnReplayStaticSolidArchiveNode *
CGameCtnReplayStaticSolidArchiveNodeDirectory::Find(
        CGameCtnReplayStaticSolidArchiveNodeIdentity identity) const {
    const u32 record = FindRecord(identity);
    return record != UINT32_MAX ? At(record) : nullptr;
}

int CGameCtnReplayStaticSolidArchiveNodeDirectory::FindClassId(
        CGameCtnReplayStaticSolidArchiveNodeIdentity identity,
        u32 *classIdOut) const {
    const CGameCtnReplayStaticSolidArchiveNode *node = Find(identity);
    if (node == nullptr || classIdOut == nullptr) {
        return 0;
    }
    *classIdOut = node->ClassId();
    return 1;
}

void CGameCtnReplayStaticSolidArchiveNodeDirectory::SetTreeId(
        CGameCtnReplayStaticSolidArchiveNodeIdentity tree,
        const CMwId &treeId,
        const char *treeIdName) {
    for (u32 i = nodes.Count(); i != 0u; i--) {
        CGameCtnReplayStaticSolidArchiveNode *node =
                nodes.MutableAt(i - 1u);
        if (node == nullptr) {
            return;
        }
        if (node->Matches(tree)) {
            node->SetTreeId(treeId);
            if (treeIdName != nullptr && treeIdName[0] != '\0') {
                CGameCtnReplayStaticSolidArchiveTreeIdName name{};
                if (name.Install(tree, treeIdName)) {
                    (void)AddTreeIdName(name);
                }
            }
            return;
        }
    }
}

const char *CGameCtnReplayStaticSolidArchiveNodeDirectory::FindTreeIdName(
        CGameCtnReplayStaticSolidArchiveNodeIdentity tree) const {
    for (const CGameCtnReplayStaticSolidArchiveTreeIdName &name :
         treeIdNames) {
        if (name.Matches(tree) && name.HasName()) {
            return name.Name();
        }
    }
    return nullptr;
}

const CGameCtnReplayStaticSolidArchiveTreeStateDefinition *
CGameCtnReplayStaticSolidArchiveNodeDirectory::FindTreeStateWithLocalIso(
        const CGameCtnReplayStaticSolidArchiveTreeGraph &treeGraph,
        CGameCtnReplayStaticSolidArchiveNodeIdentity tree) const {
    const CGameCtnReplayStaticSolidArchiveTreeStateDefinition *found = nullptr;
    treeGraph.ForEachTreeState([&](
            const CGameCtnReplayStaticSolidArchiveTreeStateDefinition
                    &state) {
        if (state.MatchesTree(tree) && state.HasLocalIso()) {
            found = &state;
        }
        return 1;
    });
    return found;
}

const CGameCtnReplayStaticSolidArchiveSurfaceGeometryDefinition *
CGameCtnReplayStaticSolidArchiveNodeDirectory::FindTreeSurfaceGeometryDefinition(
        const CGameCtnReplayStaticSolidArchiveTreeGraph &treeGraph,
        const CGameCtnReplayStaticSolidArchiveSurfaceGraph &surfaceGraph,
        CGameCtnReplayStaticSolidArchiveNodeIdentity tree) const {
    CGameCtnReplayStaticSolidArchiveNodeIdentity surface =
            CGameCtnReplayStaticSolidArchiveNodeIdentity::Invalid();
    CGameCtnReplayStaticSolidArchiveNodeIdentity geom =
            CGameCtnReplayStaticSolidArchiveNodeIdentity::Invalid();
    treeGraph.ForEachTreeSurfaceLink([&](
            const CGameCtnReplayStaticSolidArchiveTreeSurfaceLink &link) {
        if (link.Tree().Matches(tree)) {
            surface = link.Surface();
        }
        return 1;
    });
    if (!surface.IsValid()) {
        return nullptr;
    }
    surfaceGraph.ForEachSurfaceGeometryLink([&](
            const CGameCtnReplayStaticSolidArchiveSurfaceGeomLink &link) {
        if (link.Surface().Matches(surface)) {
            geom = link.Geom();
        }
        return 1;
    });
    if (!geom.IsValid()) {
        return nullptr;
    }
    const CGameCtnReplayStaticSolidArchiveSurfaceGeometryDefinition *found = nullptr;
    surfaceGraph.ForEachSurfaceGeometryDefinition([&](
            const CGameCtnReplayStaticSolidArchiveSurfaceGeometryDefinition
                    &definition) {
        if (definition.MatchesGeom(geom)) {
            found = &definition;
            return 0;
        }
        return 1;
    });
    return found;
}

void CGameCtnReplayStaticSolidArchiveGraph::Free() {
    nodeDirectory.Clear();
    treeGraph.Clear();
    surfaceGraph.Clear();
    decoratorTrees.Clear();
}

int CGameCtnReplayStaticSolidArchiveGraph::HasArchiveNodes() const {
    return !nodeDirectory.Empty();
}

CGameCtnReplayStaticSolidArchiveNodeDirectory &
CGameCtnReplayStaticSolidArchiveGraph::Nodes() {
    return nodeDirectory;
}

const CGameCtnReplayStaticSolidArchiveNodeDirectory &
CGameCtnReplayStaticSolidArchiveGraph::Nodes() const {
    return nodeDirectory;
}

int CGameCtnReplayStaticSolidArchiveGraph::FindNodeClassId(
        CGameCtnReplayStaticSolidArchiveNodeIdentity identity,
        u32 *classIdOut) const {
    return nodeDirectory.FindClassId(identity, classIdOut);
}

CGameCtnReplayStaticSolidArchiveTreeGraph &
CGameCtnReplayStaticSolidArchiveGraph::TreeGraph() {
    return treeGraph;
}

const CGameCtnReplayStaticSolidArchiveTreeGraph &
CGameCtnReplayStaticSolidArchiveGraph::TreeGraph() const {
    return treeGraph;
}

CGameCtnReplayStaticSolidArchiveSurfaceGraph &
CGameCtnReplayStaticSolidArchiveGraph::SurfaceGraph() {
    return surfaceGraph;
}

const CGameCtnReplayStaticSolidArchiveSurfaceGraph &
CGameCtnReplayStaticSolidArchiveGraph::SurfaceGraph() const {
    return surfaceGraph;
}

const CGameCtnReplayStaticSolidDecoratorTrees &
CGameCtnReplayStaticSolidArchiveGraph::DecoratorTrees() const {
    return decoratorTrees;
}

int CGameCtnReplayStaticSolidArchiveGraph::HasDecoratorTrees() const {
    return !decoratorTrees.Empty();
}
CGameCtnReplayStaticSolidDecoratorTreeCursor
CGameCtnReplayStaticSolidArchiveGraph::DecoratorTreeEnd() const {
    return decoratorTrees.End();
}

int CGameCtnReplayStaticSolidArchiveGraph::QueueDecoratorTreeDependencies(
        const StaticSolidArchiveLoadSession *archive,
        CGameCtnReplayStaticSolidDescriptorDependencyQueue *dependencyQueue,
        CGameCtnReplayStaticSolidDecoratorTreeCursor first) const {
    return decoratorTrees.RequestExternalDescriptorDependencies(archive,
                                                             dependencyQueue,
                                                             first);
}

int CGameCtnReplayStaticSolidArchiveGraph::AddDecoratorTree(
        const CGameCtnReplayStaticSolidDecoratorTreeDeclaration
                &declaration) {
    return decoratorTrees.AddParsedTree(declaration);
}

CGameCtnReplayStaticSolidArchiveNodeIdentity
CGameCtnReplayStaticSolidArchiveGraph::FindTreeForSolid(
        CGameCtnReplayStaticSolidArchiveNodeIdentity solid) const {
    if (!solid.IsValid()) {
        return CGameCtnReplayStaticSolidArchiveNodeIdentity::Invalid();
    }
    CGameCtnReplayStaticSolidArchiveNodeIdentity tree =
            CGameCtnReplayStaticSolidArchiveNodeIdentity::Invalid();
    treeGraph.ForEachSolidRootLink([&](
            const CGameCtnReplayStaticSolidArchiveSolidTreeLink &link) {
        if (link.Solid().Matches(solid)) {
            tree = link.Tree();
        }
        return 1;
    });
    return tree;
}

int CGameCtnReplayStaticSolidArchiveGraph::FindSolidRootTree(
        StaticSolidArchiveId payload,
        CGameCtnReplayStaticSolidArchiveNodeIdentity *treeOut) const {
    if (treeOut == nullptr) {
        return 0;
    }
    int found = 0;
    treeGraph.ForEachSolidRootLink([&](
            const CGameCtnReplayStaticSolidArchiveSolidTreeLink &link) {
        if (link.MatchesPayload(payload) && link.Tree().IsValid()) {
            *treeOut = link.Tree();
            found = 1;
            return 0;
        }
        return 1;
    });
    if (!found) {
        *treeOut = CGameCtnReplayStaticSolidArchiveNodeIdentity::Invalid();
    }
    return found;
}

int CGameCtnReplayStaticSolidArchiveGraph::BuildTreePathRec(
        CGameCtnReplayStaticSolidArchiveNodeIdentity current,
        CGameCtnReplayStaticSolidArchiveNodeIdentity target,
        u32 depth,
        u32 *path,
        u32 pathCapacity,
        u32 *pathLengthOut) const {
    if (current.Matches(target)) {
        *pathLengthOut = depth;
        return 1;
    }
    if (depth >= pathCapacity) {
        return 0;
    }
    u32 childSlot = 0u;
    int found = 0;
    treeGraph.ForEachChildLink([&](
            const CGameCtnReplayStaticSolidArchiveTreeChildLink &link) {
        if (!link.ParentTree().Matches(current) ||
            link.LinkKind() != 0u) {
            return 1;
        }
        path[depth] = childSlot;
        if (BuildTreePathRec(link.ChildTree(),
                             target,
                             depth + 1u,
                             path,
                             pathCapacity,
                             pathLengthOut)) {
            found = 1;
            return 0;
        }
        childSlot++;
        return 1;
    });
    return found;
}

int CGameCtnReplayStaticSolidArchiveGraph::BuildTreePath(
        CGameCtnReplayStaticSolidArchiveNodeIdentity rootTree,
        CGameCtnReplayStaticSolidArchiveNodeIdentity targetTree,
        CGameCtnReplayStaticSolidArchiveTreePath *pathOut) const {
    if (pathOut == nullptr ||
        !rootTree.IsValid() ||
        !targetTree.IsValid() ||
        !rootTree.MatchesPayload(targetTree.Payload())) {
        return 0;
    }
    u32 path[StaticSolidArchiveTreePathCapacity]{};
    u32 pathLength = 0u;
    if (!BuildTreePathRec(rootTree,
                          targetTree,
                          0u,
                          path,
                          StaticSolidArchiveTreePathCapacity,
                          &pathLength)) {
        return 0;
    }
    return pathOut->Assign(path, pathLength);
}

u32 CGameCtnReplayStaticSolidArchiveGraph::TailCount(
        CGameCtnReplayStaticSolidArchiveGraphTail tail) const {
    switch (tail) {
        case CGameCtnReplayStaticSolidArchiveGraphTail::Nodes:
            return nodeDirectory.Count();
        case CGameCtnReplayStaticSolidArchiveGraphTail::TreeChildLinks:
            return treeGraph.ChildLinkCount();
        case CGameCtnReplayStaticSolidArchiveGraphTail::TreeStateDefinitions:
            return treeGraph.TreeStateCount();
        case CGameCtnReplayStaticSolidArchiveGraphTail::SolidTreeLinks:
            return treeGraph.SolidRootLinkCount();
        case CGameCtnReplayStaticSolidArchiveGraphTail::TreeSurfaceLinks:
            return treeGraph.TreeSurfaceLinkCount();
        case CGameCtnReplayStaticSolidArchiveGraphTail::TreeSourceLinks:
            return treeGraph.TreeSourceLinkCount();
        case CGameCtnReplayStaticSolidArchiveGraphTail::SurfaceGeomLinks:
            return surfaceGraph.SurfaceGeometryLinkCount();
        case CGameCtnReplayStaticSolidArchiveGraphTail::
                SurfaceMaterialEntries:
            return surfaceGraph.SurfaceMaterialEntryCount();
        case CGameCtnReplayStaticSolidArchiveGraphTail::MaterialDefinitions:
            return surfaceGraph.MaterialDefinitionCount();
        case CGameCtnReplayStaticSolidArchiveGraphTail::SurfaceGeometryDefinitions:
            return surfaceGraph.SurfaceGeometryDefinitionCount();
        case CGameCtnReplayStaticSolidArchiveGraphTail::VisualGeometryDefinitions:
            return surfaceGraph.VisualProviderDefinitionCount();
        case CGameCtnReplayStaticSolidArchiveGraphTail::DecoratorTrees:
            return decoratorTrees.Count();
        case CGameCtnReplayStaticSolidArchiveGraphTail::Count:
            break;
    }
    return 0u;
}

int CGameCtnReplayStaticSolidArchiveGraph::RestoreTail(
        CGameCtnReplayStaticSolidArchiveGraphTail tail,
        u32 recordCount) {
    switch (tail) {
        case CGameCtnReplayStaticSolidArchiveGraphTail::Nodes:
            return nodeDirectory.ResizePrefix(recordCount);
        case CGameCtnReplayStaticSolidArchiveGraphTail::TreeChildLinks:
            return treeGraph.TruncateChildLinks(recordCount);
        case CGameCtnReplayStaticSolidArchiveGraphTail::TreeStateDefinitions:
            return treeGraph.TruncateTreeStates(recordCount);
        case CGameCtnReplayStaticSolidArchiveGraphTail::SolidTreeLinks:
            return treeGraph.TruncateSolidRootLinks(recordCount);
        case CGameCtnReplayStaticSolidArchiveGraphTail::TreeSurfaceLinks:
            return treeGraph.TruncateTreeSurfaceLinks(recordCount);
        case CGameCtnReplayStaticSolidArchiveGraphTail::TreeSourceLinks:
            return treeGraph.TruncateTreeSourceLinks(recordCount);
        case CGameCtnReplayStaticSolidArchiveGraphTail::SurfaceGeomLinks:
            return surfaceGraph.TruncateSurfaceGeometryLinks(recordCount);
        case CGameCtnReplayStaticSolidArchiveGraphTail::
                SurfaceMaterialEntries:
            return surfaceGraph.TruncateSurfaceMaterialEntries(recordCount);
        case CGameCtnReplayStaticSolidArchiveGraphTail::MaterialDefinitions:
            return surfaceGraph.TruncateMaterialDefinitions(recordCount);
        case CGameCtnReplayStaticSolidArchiveGraphTail::SurfaceGeometryDefinitions:
            return surfaceGraph.TruncateSurfaceGeometryDefinitions(recordCount);
        case CGameCtnReplayStaticSolidArchiveGraphTail::VisualGeometryDefinitions:
            return surfaceGraph.TruncateVisualProviderDefinitions(recordCount);
        case CGameCtnReplayStaticSolidArchiveGraphTail::DecoratorTrees:
            return decoratorTrees.ResizePrefix(
                    CGameCtnReplayStaticSolidDecoratorTreeCursor::
                            FromDeclarationCount(recordCount));
        case CGameCtnReplayStaticSolidArchiveGraphTail::Count:
            break;
    }
    return 0;
}

CGameCtnReplayStaticSolidArchiveGraphRollbackMark
CGameCtnReplayStaticSolidArchiveGraph::MarkRollback() const {
    CGameCtnReplayStaticSolidArchiveGraphRollbackMark mark;
    for (u32 i = 0u;
         i < static_cast<u32>(
                     CGameCtnReplayStaticSolidArchiveGraphTail::Count);
         i++) {
        const auto tail =
                static_cast<CGameCtnReplayStaticSolidArchiveGraphTail>(i);
        mark.tailCounts[(static_cast<size_t>((tail)))] = TailCount(tail);
    }
    return mark;
}

int CGameCtnReplayStaticSolidArchiveGraph::RestoreRollback(
        const CGameCtnReplayStaticSolidArchiveGraphRollbackMark &mark) {
    for (u32 i = 0u;
         i < static_cast<u32>(
                     CGameCtnReplayStaticSolidArchiveGraphTail::Count);
         i++) {
        const auto tail =
                static_cast<CGameCtnReplayStaticSolidArchiveGraphTail>(i);
        if (!RestoreTail(tail, mark.tailCounts[(static_cast<size_t>((tail)))])) {
            return 0;
        }
    }
    return 1;
}
