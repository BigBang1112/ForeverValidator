#include "engine/rendering/plug_vertex_stream.h"
#include <algorithm>
#include <limits>
#include <utility>

#include "engine/rendering/plug_visual.h"
CPlugVertexStream::SDataDecl::SDataDecl(
        EPlugVDcl declaration,
        EPlugVDclType type,
        EPlugVDclSpace space,
        bool skipVision,
        VertexAttributeValues values)
        : declaration_(declaration),
          type_(type),
          space_(space),
          skipVision_(skipVision),
          values_(std::move(values)) {}

EPlugVDcl CPlugVertexStream::SDataDecl::Declaration(void) const {
    return declaration_;
}

EPlugVDclType CPlugVertexStream::SDataDecl::Type(void) const { return type_; }

EPlugVDclSpace CPlugVertexStream::SDataDecl::Space(void) const {
    return space_;
}

bool CPlugVertexStream::SDataDecl::SkipVision(void) const {
    return skipVision_;
}

unsigned long CPlugVertexStream::SDataDecl::Count(void) const {
    return values_.Count();
}

const VertexAttributeValues &CPlugVertexStream::SDataDecl::Values(void) const {
    return values_;
}

VertexAttributeValues &CPlugVertexStream::SDataDecl::Values(void) {
    return values_;
}

VertexAttributeValue CPlugVertexStream::SDataDecl::ValueAt(
        unsigned long index) const {
    return values_.ValueAt(index);
}

void CPlugVertexStream::SDataDecl::Release(void) { values_.Clear(); }

void CPlugVertexStream::SDataDecl::MoveWholeStride(
        unsigned long destinationIndex,
        unsigned long sourceIndex,
        unsigned long count) {
    values_.MoveRange(destinationIndex, sourceIndex, count);
}

void CPlugVertexStream::SDataDecl::AllocateMoreAt(
        unsigned long prefixCount,
        unsigned long insertEndIndex,
        unsigned long tailCount) {
    if (insertEndIndex < prefixCount ||
        Count() != prefixCount + tailCount) {
        return;
    }
    values_.InsertDefault(prefixCount, insertEndIndex - prefixCount);
}

int CPlugVertexStream::SDataDecl::SortByDecl(
        const SDataDecl *left,
        const SDataDecl *right) {
    if (left == nullptr || right == nullptr) {
        return left == right ? 0 : (left == nullptr ? -1 : 1);
    }
    const auto lhs = static_cast<std::uint32_t>(left->declaration_);
    const auto rhs = static_cast<std::uint32_t>(right->declaration_);
    return lhs < rhs ? -1 : (lhs > rhs ? 1 : 0);
}

void CPlugVertexStream::SDataDecl::SetMult(
        unsigned long destinationIndex,
        const SDataDecl &source,
        unsigned long count,
        const GmIso4 &transform,
        GmBoxAligned *boundingBox) {
    if (type_ != source.type_ || destinationIndex > Count() ||
        count > Count() - destinationIndex || count > source.Count()) {
        return;
    }

    std::vector<VertexAttributeValue> copied;
    copied.reserve(count);
    for (unsigned long index = 0u; index < count; ++index) {
        copied.push_back(source.ValueAt(index));
    }

    if (space_ == EPlugVDclSpace::Point ||
        space_ == EPlugVDclSpace::Direction) {
        if (type_ != EPlugVDclType_Float3) return;
        const GmMat3 rotation = transform.RotationMatrix();
        for (VertexAttributeValue &value : copied) {
            const GmVec3 original = std::get<GmVec3>(value);
            GmVec3 transformed{};
            if (space_ == EPlugVDclSpace::Point) {
                transformed.SetMult(original, transform);
            } else {
                transformed.SetMult(original, rotation);
            }
            value = transformed;
        }
    }

    for (unsigned long index = 0u; index < count; ++index) {
        values_.SetValueAt(destinationIndex + index, copied[index]);
    }

    if (space_ != EPlugVDclSpace::Point || boundingBox == nullptr) return;
    const float highest = std::numeric_limits<float>::max();
    GmVec3 minimum{};
    GmVec3 maximum{};
    if (boundingBox->halfExtents.x < 0.0f) {
        minimum = {highest, highest, highest};
        maximum = {-highest, -highest, -highest};
    } else {
        minimum = {
            boundingBox->center.x - boundingBox->halfExtents.x,
            boundingBox->center.y - boundingBox->halfExtents.y,
            boundingBox->center.z - boundingBox->halfExtents.z,
        };
        maximum = {
            boundingBox->center.x + boundingBox->halfExtents.x,
            boundingBox->center.y + boundingBox->halfExtents.y,
            boundingBox->center.z + boundingBox->halfExtents.z,
        };
    }
    for (const VertexAttributeValue &value : copied) {
        const GmVec3 point = std::get<GmVec3>(value);
        minimum.x = std::min(minimum.x, point.x);
        minimum.y = std::min(minimum.y, point.y);
        minimum.z = std::min(minimum.z, point.z);
        maximum.x = std::max(maximum.x, point.x);
        maximum.y = std::max(maximum.y, point.y);
        maximum.z = std::max(maximum.z, point.z);
    }
    boundingBox->SetMinMax(minimum, maximum);
}

CPlugVertexStream::SStreamOrDeclType::SStreamOrDeclType(
        CPlugVertexStream *stream)
        : source(CMwNodRef<CPlugVertexStream>(stream)) {}

CPlugVertexStream::SStreamOrDeclType::SStreamOrDeclType(
        EPlugVDcl declaration,
        EPlugVDclType type)
        : source(StandaloneDeclSpec{declaration, type}) {}

CPlugVertexStream::CPlugVertexStream(void)
        : ownedContent_(),
          streamModel_(),
          isStatic_(true),
          dirtyVision_(false),
          dirty_(true) {}

CPlugVertexStream::~CPlugVertexStream(void) = default;

CMwNod *CPlugVertexStream::MwNewCPlugVertexStream(void) {
    return new CPlugVertexStream();
}

bool CPlugVertexStream::ContentIsValid(const VertexStreamContent &content) {
    bool havePrevious = false;
    EPlugVDcl previous = EPlugVDcl_Position;
    for (const SDataDecl &attribute : content.declarations) {
        if (!PlugVDclIsValid(attribute.declaration_) ||
            !PlugVDclTypeIsMaterialized(attribute.type_) ||
            attribute.space_ != PlugVDclSpace(attribute.declaration_) ||
            attribute.values_.Type() != attribute.type_ ||
            attribute.values_.Count() != content.vertexCount) {
            return false;
        }
        if (havePrevious &&
            static_cast<std::uint32_t>(previous) >=
                    static_cast<std::uint32_t>(attribute.declaration_)) {
            return false;
        }
        previous = attribute.declaration_;
        havePrevious = true;
    }
    return true;
}

CPlugVertexStream::SDataDecl *CPlugVertexStream::FindDeclaration(
        VertexStreamContent &content,
        EPlugVDcl declaration,
        unsigned long *outIndex) {
    const auto found = std::lower_bound(
            content.declarations.begin(), content.declarations.end(), declaration,
            [](const SDataDecl &attribute, EPlugVDcl value) {
                return static_cast<std::uint32_t>(attribute.declaration_) <
                       static_cast<std::uint32_t>(value);
            });
    if (outIndex != nullptr) {
        *outIndex = static_cast<unsigned long>(
                found - content.declarations.begin());
    }
    return found != content.declarations.end() &&
                   found->declaration_ == declaration
            ? &*found
            : nullptr;
}

const CPlugVertexStream::SDataDecl *CPlugVertexStream::FindDeclaration(
        const VertexStreamContent &content,
        EPlugVDcl declaration,
        unsigned long *outIndex) {
    const auto found = std::lower_bound(
            content.declarations.begin(), content.declarations.end(), declaration,
            [](const SDataDecl &attribute, EPlugVDcl value) {
                return static_cast<std::uint32_t>(attribute.declaration_) <
                       static_cast<std::uint32_t>(value);
            });
    if (outIndex != nullptr) {
        *outIndex = static_cast<unsigned long>(
                found - content.declarations.begin());
    }
    return found != content.declarations.end() &&
                   found->declaration_ == declaration
            ? &*found
            : nullptr;
}

bool CPlugVertexStream::WouldCreateModelCycle(
        const CPlugVertexStream *source) const {
    for (const CPlugVertexStream *cursor = source;
         cursor != nullptr;
         cursor = cursor->streamModel_.Get()) {
        if (cursor == this) return true;
    }
    return false;
}

bool CPlugVertexStream::CommitContent(VertexStreamContent content) {
    if (!ContentIsValid(content)) return false;
    AdoptValidatedContent(std::move(content));
    return true;
}

void CPlugVertexStream::AdoptValidatedContent(VertexStreamContent content) {
    streamModel_.MwSetNod(nullptr);
    ownedContent_ = std::move(content);
}

void CPlugVertexStream::DetachModel(void) {
    if (!streamModel_) return;
    ownedContent_ = ResolvedContent();
    streamModel_.MwSetNod(nullptr);
}

unsigned long CPlugVertexStream::VertexCount(void) const {
    return ResolvedContent().vertexCount;
}

bool CPlugVertexStream::IsStatic(void) const { return isStatic_; }
bool CPlugVertexStream::DirtyVision(void) const { return dirtyVision_; }
bool CPlugVertexStream::IsDirty(void) const { return dirty_; }

bool CPlugVertexStream::HasDeclaration(EPlugVDcl declaration) const {
    return DataFindByDecl(declaration, nullptr) != nullptr;
}

unsigned long CPlugVertexStream::TexCoordDeclarationCount(void) const {
    unsigned long count = 0u;
    for (unsigned long index = 0u; index < 8u; ++index) {
        const EPlugVDcl declaration = static_cast<EPlugVDcl>(
                static_cast<std::uint32_t>(EPlugVDcl_TexCoord0) + index);
        if (HasDeclaration(declaration)) count = index + 1u;
    }
    return count;
}

const std::vector<CPlugVertexStream::SDataDecl> &
CPlugVertexStream::DataDeclarations(void) const {
    return ResolvedContent().declarations;
}

bool CPlugVertexStream::CopyAttributeValues(
        EPlugVDcl declaration,
        VertexAttributeValues &outValues) const {
    const SDataDecl *attribute = DataFindByDecl(declaration, nullptr);
    if (attribute == nullptr) return false;
    outValues = attribute->values_;
    return true;
}

bool CPlugVertexStream::AttributeValueAt(
        EPlugVDcl declaration,
        unsigned long index,
        VertexAttributeValue &outValue) const {
    const SDataDecl *attribute = DataFindByDecl(declaration, nullptr);
    if (attribute == nullptr || index >= attribute->Count()) return false;
    outValue = attribute->ValueAt(index);
    return true;
}

bool CPlugVertexStream::SetAttributeValueAt(
        EPlugVDcl declaration,
        unsigned long index,
        const VertexAttributeValue &value) {
    const SDataDecl *resolved = DataFindByDecl(declaration, nullptr);
    if (resolved == nullptr || index >= resolved->Count() ||
        resolved->ValueAt(index).index() != value.index()) {
        return false;
    }
    DetachModel();
    SDataDecl *attribute = FindDeclaration(ownedContent_, declaration, nullptr);
    if (attribute == nullptr || !attribute->values_.SetValueAt(index, value)) return false;
    SetDirty(1);
    return true;
}

const VertexAttributeValues *CPlugVertexStream::AttributeValues(
        EPlugVDcl declaration) const {
    const SDataDecl *attribute = DataFindByDecl(declaration, nullptr);
    return attribute != nullptr ? &attribute->values_ : nullptr;
}

VertexAttributeValues *CPlugVertexStream::MutableAttributeValues(
        EPlugVDcl declaration) {
    if (DataFindByDecl(declaration, nullptr) == nullptr) return nullptr;
    DetachModel();
    SDataDecl *attribute = FindDeclaration(ownedContent_, declaration, nullptr);
    return attribute != nullptr ? &attribute->values_ : nullptr;
}

const GmVec3 *CPlugVertexStream::GmVec3AttributeData(
        EPlugVDcl declaration) const {
    const VertexAttributeValues *values = AttributeValues(declaration);
    const std::vector<GmVec3> *typed =
            values != nullptr ? values->Values<GmVec3>() : nullptr;
    return typed != nullptr && !typed->empty() ? typed->data() : nullptr;
}

bool CPlugVertexStream::GetTexCoordSet(
        EPlugVDcl declaration,
        GxTexCoordSet &outSet) const {
    if (!PlugVDclIsTexCoord(declaration)) return false;
    const VertexAttributeValues *values = AttributeValues(declaration);
    return values != nullptr && values->ToTexCoordSet(outSet);
}

void CPlugVertexStream::Alloc(void) {
    DetachModel();
    for (SDataDecl &attribute : ownedContent_.declarations) {
        attribute.values_.Resize(ownedContent_.vertexCount);
    }
}

void CPlugVertexStream::SetDirty(int dirty) { dirty_ = dirty != 0; }

void CPlugVertexStream::Release(void) {
    if (!streamModel_) ownedContent_ = VertexStreamContent{};
}

const CPlugVertexStream::SDataDecl *CPlugVertexStream::DataFindByDecl(
        EPlugVDcl declaration,
        unsigned long *outIndex) const {
    return FindDeclaration(ResolvedContent(), declaration, outIndex);
}

bool CPlugVertexStream::SupportsCanonicalVertices(
        int wantPositions,
        int wantNormals,
        int wantColors) const {
    const SDataDecl *position = DataFindByDecl(EPlugVDcl_Position, nullptr);
    const SDataDecl *normal = DataFindByDecl(EPlugVDcl_Normal, nullptr);
    const SDataDecl *color = DataFindByDecl(EPlugVDcl_Color0, nullptr);
    const bool positionUsable = position != nullptr &&
            position->Type() == EPlugVDclType_Float3;
    const bool normalUsable = normal != nullptr &&
            normal->Type() == EPlugVDclType_Float3;
    const bool colorUsable = color != nullptr &&
            color->Type() == EPlugVDclType_Float4;
    if ((wantPositions != 0 && !positionUsable) ||
        (wantNormals != 0 && normal != nullptr && !normalUsable) ||
        (wantColors != 0 && color != nullptr && !colorUsable) ||
        (!positionUsable && !normalUsable && !colorUsable)) {
        return false;
    }
    return true;
}

std::vector<GxVertex> CPlugVertexStream::CanonicalVertices(
        int wantPositions,
        int wantNormals,
        int wantColors) const {
    if (!SupportsCanonicalVertices(
                wantPositions, wantNormals, wantColors)) {
        return {};
    }

    const SDataDecl *position = DataFindByDecl(EPlugVDcl_Position, nullptr);
    const SDataDecl *normal = DataFindByDecl(EPlugVDcl_Normal, nullptr);
    const SDataDecl *color = DataFindByDecl(EPlugVDcl_Color0, nullptr);
    const bool positionUsable = position != nullptr &&
            position->Type() == EPlugVDclType_Float3;
    const bool normalUsable = normal != nullptr &&
            normal->Type() == EPlugVDclType_Float3;
    const bool colorUsable = color != nullptr &&
            color->Type() == EPlugVDclType_Float4;

    std::vector<GxVertex> vertices(VertexCount());
    for (unsigned long index = 0u; index < VertexCount(); ++index) {
        if (positionUsable) {
            vertices[index].position =
                    std::get<GmVec3>(position->ValueAt(index));
        }
        if (normalUsable) {
            vertices[index].normal =
                    std::get<GmVec3>(normal->ValueAt(index));
        }
        if (colorUsable) {
            const GmVec4 value = std::get<GmVec4>(color->ValueAt(index));
            vertices[index].color[0] = value.x;
            vertices[index].color[1] = value.y;
            vertices[index].color[2] = value.z;
            vertices[index].color[3] = value.w;
        }
    }
    return vertices;
}

const CFastBuffer<GxVertex> *CPlugVertexStream::GetGxVertexs(
        int wantPositions,
        int wantNormals,
        int wantColors) const {
    if (!SupportsCanonicalVertices(
                wantPositions, wantNormals, wantColors)) {
        return nullptr;
    }
    canonicalVertexView_.Values() =
            CanonicalVertices(wantPositions, wantNormals, wantColors);
    return &canonicalVertexView_;
}

void CPlugVertexStream::SetIsStatic(int isStatic) {
    const bool value = isStatic != 0;
    if (isStatic_ != value) {
        isStatic_ = value;
        SetDirty(1);
    }
}

void CPlugVertexStream::SetDirtyVision(int dirtyVision) {
    const bool value = dirtyVision != 0;
    if (dirtyVision_ != value) {
        dirtyVision_ = value;
        SetDirty(1);
    }
}

void CPlugVertexStream::SetVertexCount(unsigned long count) {
    DetachModel();
    for (SDataDecl &attribute : ownedContent_.declarations) attribute.values_.Resize(count);
    ownedContent_.vertexCount = count;
    SetDirty(1);
}

void CPlugVertexStream::MoveAt(unsigned long destinationIndex,
                               unsigned long sourceIndex,
                               unsigned long count) {
    const unsigned long vertexCount = VertexCount();
    if (destinationIndex > vertexCount || sourceIndex > vertexCount ||
        count > vertexCount - destinationIndex ||
        count > vertexCount - sourceIndex) {
        return;
    }
    DetachModel();
    for (SDataDecl &attribute : ownedContent_.declarations) {
        attribute.MoveWholeStride(destinationIndex, sourceIndex, count);
    }
    SetDirty(1);
}

void CPlugVertexStream::RemoveAt(unsigned long index, unsigned long count) {
    const unsigned long vertexCount = VertexCount();
    if (index > vertexCount || count > vertexCount - index) return;
    DetachModel();
    for (SDataDecl &attribute : ownedContent_.declarations) {
        attribute.values_.Erase(index, count);
    }
    ownedContent_.vertexCount -= count;
    SetDirty(1);
}

void CPlugVertexStream::AllocateMoreAt(unsigned long insertIndex,
                                       unsigned long insertCount) {
    const unsigned long vertexCount = VertexCount();
    if (insertIndex > vertexCount ||
        insertCount > InvalidEngineIndex - vertexCount) {
        return;
    }
    DetachModel();
    for (SDataDecl &attribute : ownedContent_.declarations) {
        attribute.values_.InsertDefault(insertIndex, insertCount);
    }
    ownedContent_.vertexCount += insertCount;
    SetDirty(1);
}

const CPlugVertexStream::SDataDecl *CPlugVertexStream::DataFindSharedPtr(
        const SDataDecl *target) const {
    (void)target;
    return nullptr;
}

void CPlugVertexStream::DataSetSkipVision(EPlugVDcl declaration,
                                          int skipVision) {
    if (DataFindByDecl(declaration, nullptr) == nullptr) return;
    DetachModel();
    FindDeclaration(ownedContent_, declaration, nullptr)->skipVision_ =
            skipVision != 0;
    SetDirty(1);
}

void CPlugVertexStream::DataChgDecl(EPlugVDcl newDeclaration,
                                    EPlugVDcl oldDeclaration) {
    if (!PlugVDclIsValid(newDeclaration) ||
        !PlugVDclIsValid(oldDeclaration) ||
        PlugVDclDefaultType(newDeclaration) !=
                PlugVDclDefaultType(oldDeclaration) ||
        DataFindByDecl(oldDeclaration, nullptr) == nullptr ||
        DataFindByDecl(newDeclaration, nullptr) != nullptr) {
        return;
    }
    DetachModel();
    SDataDecl *attribute = FindDeclaration(ownedContent_, oldDeclaration, nullptr);
    attribute->declaration_ = newDeclaration;
    attribute->space_ = PlugVDclSpace(newDeclaration);
    SortDeclarations(ownedContent_);
    SetDirty(1);
}

void CPlugVertexStream::SetAttributeValues(
        EPlugVDcl declaration,
        VertexAttributeValues values,
        bool requestedSkipVision) {
    const EPlugVDclType type = values.Type();
    if (!PlugVDclIsValid(declaration) ||
        !PlugVDclTypeIsMaterialized(type) ||
        values.Count() != VertexCount()) {
        return;
    }
    DetachModel();
    unsigned long index = 0u;
    SDataDecl *existing = FindDeclaration(ownedContent_, declaration, &index);
    const bool skipVision = existing != nullptr
            ? existing->skipVision_
            : requestedSkipVision;
    SDataDecl replacement(declaration, type, PlugVDclSpace(declaration),
                          skipVision, std::move(values));
    if (existing != nullptr) {
        *existing = std::move(replacement);
    } else {
        ownedContent_.declarations.insert(
                ownedContent_.declarations.begin() + index,
                std::move(replacement));
    }
    SetDirty(1);
}

void CPlugVertexStream::SetCanonicalGxVertices(
        std::vector<GxVertex> vertices,
        bool hasNormals,
        bool hasColors) {
    VertexStreamContent content;
    content.vertexCount = static_cast<unsigned long>(vertices.size());
    std::vector<GmVec3> positions(vertices.size());
    std::vector<GmVec3> normals(hasNormals ? vertices.size() : 0u);
    std::vector<GmVec4> colors(hasColors ? vertices.size() : 0u);
    for (std::size_t index = 0u; index < vertices.size(); ++index) {
        positions[index] = vertices[index].position;
        if (hasNormals) normals[index] = vertices[index].normal;
        if (hasColors) {
            colors[index] = {vertices[index].color[0], vertices[index].color[1],
                             vertices[index].color[2], vertices[index].color[3]};
        }
    }
    content.declarations.emplace_back(
            EPlugVDcl_Position, EPlugVDclType_Float3,
            EPlugVDclSpace::Point, false,
            VertexAttributeValues(std::move(positions)));
    if (hasNormals) {
        content.declarations.emplace_back(
                EPlugVDcl_Normal, EPlugVDclType_Float3,
                EPlugVDclSpace::Direction, false,
                VertexAttributeValues(std::move(normals)));
    }
    if (hasColors) {
        content.declarations.emplace_back(
                EPlugVDcl_Color0, EPlugVDclType_Float4,
                EPlugVDclSpace::Invariant, false,
                VertexAttributeValues(std::move(colors)));
    }
    SortDeclarations(content);
    if (CommitContent(std::move(content))) SetDirty(1);
}

bool CPlugVertexStream::ReplaceContent(
        unsigned long vertexCount,
        std::vector<SDataDecl> declarations) {
    VertexStreamContent content;
    content.vertexCount = vertexCount;
    content.declarations = std::move(declarations);
    SortDeclarations(content);
    return CommitContent(std::move(content));
}

void CPlugVertexStream::SetStreamModel(CPlugVertexStream *source) {
    if (source == streamModel_.Get()) return;
    if (source == nullptr) {
        DetachModel();
        return;
    }
    if (WouldCreateModelCycle(source)) return;
    isStatic_ = source->isStatic_;
    dirtyVision_ = source->dirtyVision_;
    dirty_ = source->dirty_;
    streamModel_.MwSetNod(source);
    ownedContent_ = VertexStreamContent{};
}

CPlugVertexStream *CPlugVertexStream::StreamModel(void) const {
    return streamModel_.Get();
}

unsigned long CPlugVertexStream::GetStrideTotal(void) const {
    unsigned long total = 0u;
    for (const SDataDecl &attribute : DataDeclarations()) {
        total += static_cast<unsigned long>(PlugVDclTypeByteCount(attribute.Type()));
    }
    return total;
}

void CPlugVertexStream::AllocateFromStreams(
        CFastBuffer<SStreamOrDeclType> &sources,
        unsigned long vertexCount) {
    AllocateFromSources(sources.Values(), vertexCount);
}

void CPlugVertexStream::AllocateFromSources(
        const std::vector<SStreamOrDeclType> &sources,
        unsigned long vertexCount) {
    VertexStreamContent content;
    content.vertexCount = vertexCount;
    bool inheritedFlags = false;
    bool isStatic = isStatic_;
    bool dirtyVision = dirtyVision_;

    for (const SStreamOrDeclType &entry : sources) {
        if (const auto *streamRef =
                    std::get_if<CMwNodRef<CPlugVertexStream>>(&entry.source)) {
            CPlugVertexStream *source = streamRef->Get();
            if (source == nullptr) continue;
            inheritedFlags = true;
            isStatic = source->IsStatic();
            dirtyVision = source->DirtyVision();
            for (const SDataDecl &sourceRow : source->DataDeclarations()) {
                SDataDecl *existing = FindDeclaration(
                        content, sourceRow.Declaration(), nullptr);
                if (existing != nullptr) {
                    if (existing->Type() != sourceRow.Type() ||
                        existing->SkipVision() != sourceRow.SkipVision()) {
                        return;
                    }
                    continue;
                }
                content.declarations.emplace_back(
                        sourceRow.Declaration(), sourceRow.Type(),
                        sourceRow.Space(), sourceRow.SkipVision(),
                        VertexAttributeValues(sourceRow.Type(), vertexCount));
                SortDeclarations(content);
            }
            continue;
        }

        const StandaloneDeclSpec &spec =
                std::get<StandaloneDeclSpec>(entry.source);
        if (!PlugVDclIsValid(spec.declaration) ||
            !PlugVDclTypeIsMaterialized(spec.type)) {
            return;
        }
        SDataDecl *existing = FindDeclaration(
                content, spec.declaration, nullptr);
        if (existing != nullptr) {
            if (existing->Type() != spec.type) return;
            continue;
        }
        content.declarations.emplace_back(
                spec.declaration, spec.type, PlugVDclSpace(spec.declaration),
                false, VertexAttributeValues(spec.type, vertexCount));
        SortDeclarations(content);
    }

    if (!CommitContent(std::move(content))) return;
    if (inheritedFlags) {
        isStatic_ = isStatic;
        dirtyVision_ = dirtyVision;
    }
    SetDirty(1);
}

void CPlugVertexStream::InitDeclFromDataDecl(void) {
    (void)ContentIsValid(ResolvedContent());
}

void CPlugVertexStream::SetMultAt(
        unsigned long destinationIndex,
        const CPlugVertexStream *source,
        const GmIso4 &transform,
        GmBoxAligned *boundingBox) {
    if (source == nullptr || source->VertexCount() == 0u ||
        destinationIndex > VertexCount() ||
        source->VertexCount() > VertexCount() - destinationIndex) {
        return;
    }
    for (const SDataDecl &sourceRow : source->DataDeclarations()) {
        const SDataDecl *destinationRow =
                DataFindByDecl(sourceRow.Declaration(), nullptr);
        if (destinationRow == nullptr ||
            destinationRow->Type() != sourceRow.Type() ||
            destinationRow->Space() != sourceRow.Space()) {
            return;
        }
    }
    DetachModel();
    for (const SDataDecl &sourceRow : source->DataDeclarations()) {
        SDataDecl *destinationRow = FindDeclaration(
                ownedContent_, sourceRow.Declaration(), nullptr);
        destinationRow->SetMult(destinationIndex, sourceRow,
                                source->VertexCount(), transform, boundingBox);
    }
    SetDirty(1);
}

void CPlugVertexStream::CopyAndClearVisual(CPlugVisual *visual) {
    CPlugVisual3D *visual3D = dynamic_cast<CPlugVisual3D *>(visual);
    if (visual3D == nullptr) return;
    const unsigned long count =
            static_cast<unsigned long>(visual3D->positions.size());
    if ((!visual3D->normals.empty() && visual3D->normals.size() != count) ||
        (!visual3D->colors.empty() && visual3D->colors.size() != count) ||
        (!visual3D->tangents.empty() && visual3D->tangents.size() != count) ||
        (!visual3D->binormals.empty() && visual3D->binormals.size() != count) ||
        visual->texCoordSets.size() > 8u) {
        return;
    }
    for (const GxTexCoordSet &set : visual->texCoordSets) {
        if (set.Count() != count) return;
    }

    VertexStreamContent content;
    content.vertexCount = count;
    content.declarations.emplace_back(
            EPlugVDcl_Position, EPlugVDclType_Float3,
            EPlugVDclSpace::Point, false,
            VertexAttributeValues(std::move(visual3D->positions)));
    if (!visual3D->normals.empty()) {
        content.declarations.emplace_back(
                EPlugVDcl_Normal, EPlugVDclType_Float3,
                EPlugVDclSpace::Direction, false,
                VertexAttributeValues(std::move(visual3D->normals)));
    }
    if (!visual3D->colors.empty()) {
        content.declarations.emplace_back(
                EPlugVDcl_Color0, EPlugVDclType_Float4,
                EPlugVDclSpace::Invariant, false,
                VertexAttributeValues(std::move(visual3D->colors)));
    }
    for (unsigned long index = 0u; index < visual->texCoordSets.size(); ++index) {
        const EPlugVDcl declaration = static_cast<EPlugVDcl>(
                static_cast<std::uint32_t>(EPlugVDcl_TexCoord0) + index);
        VertexAttributeValues values(visual->texCoordSets[index]);
        content.declarations.emplace_back(
                declaration, values.Type(), EPlugVDclSpace::Invariant, false,
                std::move(values));
    }
    if (!visual3D->tangents.empty()) {
        content.declarations.emplace_back(
                EPlugVDcl_TangentU, EPlugVDclType_Float3,
                EPlugVDclSpace::Direction, false,
                VertexAttributeValues(std::move(visual3D->tangents)));
    }
    if (!visual3D->binormals.empty()) {
        content.declarations.emplace_back(
                EPlugVDcl_TangentV, EPlugVDclType_Float3,
                EPlugVDclSpace::Direction, false,
                VertexAttributeValues(std::move(visual3D->binormals)));
    }
    SortDeclarations(content);
    if (!CommitContent(std::move(content))) return;
    visual3D->positions.clear();
    visual3D->normals.clear();
    visual3D->colors.clear();
    visual3D->tangents.clear();
    visual3D->binormals.clear();
    visual->texCoordSets.clear();
    SetDirty(1);
}

void CPlugVertexStream::StreamModelUpdateDataPtrs(void) const {}
