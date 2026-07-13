#pragma once

#include <cstdint>
#include <vector>

#include "engine/rendering/plug_visual.h"
struct CPlugVisual::SSkinData {
    std::vector<std::uint8_t> skinIndexWeights;
    std::vector<GmIso4> visualToBoneTransforms;
    std::vector<CMwId> boneIds;
    std::vector<GmBoxAligned> boneLocalBoxes;

    SSkinData(void);
    ~SSkinData(void);
    SSkinData(const SSkinData &) = delete;
    SSkinData &operator=(const SSkinData &) = delete;
    void BoneSetCount(unsigned long count);
};
