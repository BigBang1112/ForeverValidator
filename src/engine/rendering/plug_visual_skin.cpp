#include "engine/rendering/plug_visual_skin.h"
CPlugVisual::SSkinData::SSkinData(void) = default;

CPlugVisual::SSkinData::~SSkinData(void) = default;

void CPlugVisual::SSkinData::BoneSetCount(unsigned long count) {
    visualToBoneTransforms.resize(count);
    boneIds.resize(count);
    boneLocalBoxes.resize(count);
}
