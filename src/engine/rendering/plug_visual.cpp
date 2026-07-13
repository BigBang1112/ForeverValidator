#include "engine/rendering/plug_visual.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "engine/core/binary32_math.h"
#include "engine/physics/geometry/geometry_helpers.h"
#include "engine/rendering/plug_vertex_stream.h"
#include "engine/rendering/plug_visual_skin.h"
CPlugVisual::CPlugVisual(void)
        : id(),
          boundingBox{},
          skinData(nullptr),
          vertexStreams(),
          texCoordSets(),
          state_() {
    SetBoundingNull();
}

CPlugVisual::CPlugVisual(const CPlugVisual &source)
        : CPlug(),
          id(),
          boundingBox{},
          skinData(nullptr),
          vertexStreams(),
          texCoordSets(),
          state_(State::ForDuplicate(source.state_)) {
    SetBoundingNull();
}

CPlugVisual::~CPlugVisual(void) = default;

CPlugVisual::State CPlugVisual::State::ForDuplicate(
        const State &source) {
    State result;
    result.geometryStatic = source.geometryStatic;
    result.indexationStatic = source.indexationStatic;
    result.vertexNormal = source.vertexNormal;
    result.vertexColor = source.vertexColor;
    result.optimizeInVision = false;
    result.skinIndexCount = 0u;
    result.featureDirty = true;
    result.visualDirty = true;
    return result;
}

void SPlugVDcls::Clear(void) {
    declarations_.fill(false);
}

void SPlugVDcls::Require(EPlugVDcl declaration) {
    if (!PlugVDclIsValid(declaration)) {
        return;
    }
    declarations_[static_cast<std::size_t>(declaration)] = true;
}

bool SPlugVDcls::Requires(EPlugVDcl declaration) const {
    return PlugVDclIsValid(declaration) &&
           declarations_[static_cast<std::size_t>(declaration)];
}

void SPlugVDcls::Merge(const SPlugVDcls &other) {
    for (std::size_t index = 0u; index < declarations_.size(); ++index) {
        declarations_[index] = declarations_[index] ||
                               other.declarations_[index];
    }
}

bool SPlugVDcls::Empty(void) const {
    return std::none_of(
            declarations_.begin(), declarations_.end(),
            [](bool required) { return required; });
}

int CPlugVisual::IsValidForRender(void) const {
    return !CanonicalVertices(1, 1, 1).empty();
}


CPlugVisual *CPlugVisual::Optimize(int arg,
                                   unsigned long optimizeFlags) const {
    (void)arg;
    (void)optimizeFlags;
    return const_cast<CPlugVisual *>(this);
}

void CPlugVisual::SetDefaultShaderProperties(CPlugShader *shader) {
    (void)shader;
}

void CPlugVisual::ResetToEmptyVisual(int value) {
    (void)value;
    SetBoundingNull();
}

void CPlugVisual::ComputeBoundingBox(unsigned long computeFlags,
                                     unsigned long subVisual) {
    (void)computeFlags;
    (void)subVisual;
}

int CPlugVisual::ComputeTangents(unsigned long texCoordSetIndex,
                                int arg1,
                                int computeTangents,
                                int computeBinormals) {
    (void)texCoordSetIndex;
    (void)arg1;
    (void)computeTangents;
    (void)computeBinormals;
    return 0;
}

int CPlugVisual::SelfWeld(float x,
                          float y,
                          float z,
                          float w,
                          int weldFlags) {
    (void)x;
    (void)y;
    (void)z;
    (void)w;
    (void)weldFlags;
    return 0;
}

bool CPlugVisual::HasVertexNormal(void) const {
    return state_.vertexNormal;
}

bool CPlugVisual::HasVertexColor(void) const {
    return state_.vertexColor;
}

bool CPlugVisual::IsGeometryStatic(void) const {
    return state_.geometryStatic;
}

bool CPlugVisual::IsIndexationStatic(void) const {
    return state_.indexationStatic;
}

bool CPlugVisual::IsOptimizedInVision(void) const {
    return state_.optimizeInVision;
}

bool CPlugVisual::IsFeatureDirty(void) const {
    return state_.featureDirty;
}

bool CPlugVisual::IsVisualDirty(void) const {
    return state_.visualDirty;
}

u32 CPlugVisual::SkinIndexCount(void) const {
    return state_.skinIndexCount;
}

void CPlugVisual::SetGeometryStatic(int enabled) {
    const bool requested = enabled != 0;
    if (state_.geometryStatic != requested) {
        state_.geometryStatic = requested;
        MarkVisualDirty();
    }
}



void CPlugVisual::SetIndexationStatic(int enabled) {
    const bool requested = enabled != 0;
    if (state_.indexationStatic != requested) {
        state_.indexationStatic = requested;
        MarkVisualDirty();
    }
}



void CPlugVisual::EnableVertexColor(int enabled) {
    const bool requested = enabled != 0;
    if (state_.vertexColor != requested) {
        state_.vertexColor = requested;
        MarkFeatureDirty();
    }
}



void CPlugVisual::EnableVertexNormal(int enabled) {
    const bool requested = enabled != 0;
    if (state_.vertexNormal != requested) {
        state_.vertexNormal = requested;
        MarkFeatureDirty();
    }
}



void CPlugVisual::SetOptimizeInVision(int enabled) {
    const bool requested = enabled != 0;
    if (state_.optimizeInVision != requested) {
        state_.optimizeInVision = requested;
        MarkVisualDirty();
    }
}



void CPlugVisual::SetBoundingBox(const GmBoxAligned &box) {
    boundingBox = box;
}


void CPlugVisual::SetBoundingMinMax(
        const GmVec3 &minPoint,
        const GmVec3 &maxPoint) {
    boundingBox.SetMinMax(minPoint, maxPoint);
}


void CPlugVisual::SetBoundingNull(void) {
    boundingBox.center = {0.0f, 0.0f, 0.0f};
    boundingBox.halfExtents = {-1.0f, -1.0f, -1.0f};
}

const GmBoxAligned &CPlugVisual::BoundingBox(void) const {
    return boundingBox;
}



uint8_t *CPlugVisual::GetSkinIndexWeights(void) const {
    if (skinData == nullptr || skinData->skinIndexWeights.empty()) {
        return nullptr;
    }
    return skinData->skinIndexWeights.data();
}


void CPlugVisual::SetSkinIndexCount(unsigned long count) {
    constexpr u32 SkinIndexCountRange = 8u;
    state_.skinIndexCount = static_cast<u32>(count) % SkinIndexCountRange;
    if (state_.skinIndexCount != 0u) {
        skinData = std::make_unique<SSkinData>();
        return;
    }

    skinData.reset();
}
