#include "engine/rendering/plug_visual.h"
#include <algorithm>
#include <memory>

#include "engine/rendering/plug_vertex_stream.h"
#include "engine/rendering/plug_visual_skin.h"
namespace {

std::unique_ptr<CPlugVisual> DuplicateVisualStorage(
        const CPlugVisual &source) {
    if (const auto *triangles =
                dynamic_cast<const CPlugVisualIndexedTriangles *>(&source)) {
        return std::make_unique<CPlugVisualIndexedTriangles>(*triangles);
    }
    if (const auto *indexed =
                dynamic_cast<const CPlugVisualIndexed *>(&source)) {
        return std::make_unique<CPlugVisualIndexed>(*indexed);
    }
    if (const auto *visual3D =
                dynamic_cast<const CPlugVisual3D *>(&source)) {
        return std::make_unique<CPlugVisual3D>(*visual3D);
    }
    return std::make_unique<CPlugVisual>(source);
}

}  // namespace

CPlugVisual *CPlugVisual::Duplicate_ShareGeometryAndIndexation(
        unsigned long requestedVertexStreamCount) {
    std::unique_ptr<CPlugVisual> duplicate = DuplicateVisualStorage(*this);

    duplicate->boundingBox = boundingBox;
    duplicate->id = id;
    duplicate->state_ = State::ForDuplicate(state_);
    if (skinData != nullptr) {
        duplicate->skinData = std::make_unique<SSkinData>();
        duplicate->skinData->skinIndexWeights = skinData->skinIndexWeights;
        duplicate->skinData->visualToBoneTransforms =
                skinData->visualToBoneTransforms;
        duplicate->skinData->boneIds = skinData->boneIds;
        duplicate->skinData->boneLocalBoxes = skinData->boneLocalBoxes;
    }
    duplicate->texCoordSets.reserve(texCoordSets.size());
    for (const GxTexCoordSet &texCoordSet : texCoordSets) {
        duplicate->texCoordSets.push_back(texCoordSet);
    }

    const unsigned long shareCount = std::min(
            requestedVertexStreamCount,
            static_cast<unsigned long>(vertexStreams.size()));
    duplicate->vertexStreams.reserve(shareCount);
    for (unsigned long index = 0u; index < shareCount; ++index) {
        duplicate->VertexStreamAdd(vertexStreams[index].Get());
    }
    return duplicate.release();
}

std::unique_ptr<CPlugVisual> CPlugVisual::CloneVisual(void) const {
    return std::unique_ptr<CPlugVisual>(
            const_cast<CPlugVisual *>(this)
                    ->Duplicate_ShareGeometryAndIndexation(
                            VertexStreamCount()));
}

void CPlugVisual::CopyGeometry(
        const CPlugVisual *source,
        const unsigned short *indices) {
    if (source == nullptr) {
        return;
    }

    const unsigned long vertexCount = source->GetTotalVertexCount();
    texCoordSets.clear();
    texCoordSets.reserve(source->texCoordSets.size());
    for (const GxTexCoordSet &sourceSet : source->texCoordSets) {
        GxTexCoordSet destinationSet = GxTexCoordSet::Create(
                sourceSet.Dimension(), vertexCount);
        const unsigned long copyCount = std::min(
                vertexCount,
                sourceSet.Count());
        for (unsigned long sourceIndex = 0u;
             sourceIndex < copyCount;
             ++sourceIndex) {
            const unsigned long destinationIndex = indices == nullptr
                    ? sourceIndex
                    : indices[sourceIndex];
            if (destinationIndex == 0xffffu ||
                destinationIndex >= vertexCount) {
                continue;
            }
            destinationSet.SetCoordinate(
                    destinationIndex,
                    sourceSet.Coordinate4At(sourceIndex));
        }
        texCoordSets.push_back(std::move(destinationSet));
    }

    state_ = source->state_;
    boundingBox = source->boundingBox;
    skinData.reset();
    if (source->skinData != nullptr) {
        skinData = std::make_unique<SSkinData>();
        skinData->skinIndexWeights = source->skinData->skinIndexWeights;
        skinData->visualToBoneTransforms =
                source->skinData->visualToBoneTransforms;
        skinData->boneIds = source->skinData->boneIds;
        skinData->boneLocalBoxes = source->skinData->boneLocalBoxes;
        if (indices != nullptr && vertexCount != 0u &&
            skinData->skinIndexWeights.size() % vertexCount == 0u) {
            const unsigned long weightsPerVertex =
                    skinData->skinIndexWeights.size() / vertexCount;
            std::vector<uint8_t> remapped(
                    skinData->skinIndexWeights.size(),
                    0u);
            for (unsigned long sourceIndex = 0u;
                 sourceIndex < vertexCount;
                 ++sourceIndex) {
                const unsigned long destinationIndex = indices[sourceIndex];
                if (destinationIndex == 0xffffu ||
                    destinationIndex >= vertexCount) {
                    continue;
                }
                const auto sourceBegin =
                        skinData->skinIndexWeights.begin() +
                        sourceIndex * weightsPerVertex;
                std::copy(
                        sourceBegin,
                        sourceBegin + weightsPerVertex,
                        remapped.begin() +
                                destinationIndex * weightsPerVertex);
            }
            skinData->skinIndexWeights = std::move(remapped);
        }
    }
    MarkFeatureDirty();
    MarkVisualDirty();
}
