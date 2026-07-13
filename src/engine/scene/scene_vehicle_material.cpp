#include "engine/scene/scene_vehicle_material.h"
CSceneVehicleMaterial::CSceneVehicleMaterial(void) = default;

CSceneVehicleMaterial::~CSceneVehicleMaterial(void) = default;

void CSceneVehicleMaterialContainer::Clear(void) {
    materials.clear();
}

CSceneVehicleMaterial &CSceneVehicleMaterialContainer::AddMaterial(void) {
    materials.push_back(std::make_unique<CSceneVehicleMaterial>());
    return *materials.back();
}

u32 CSceneVehicleMaterialContainer::MaterialCount(void) const {
    return static_cast<u32>(materials.size());
}

CSceneVehicleMaterial *CSceneVehicleMaterialContainer::MaterialAt(
        u32 index) const {
    return materials[index].get();
}

void CSceneVehicleMaterial::Configure(
        const SBlendableVals &blendableValues,
        float contactPeriodX,
        float contactPeriodZ,
        float contactSpeedScale,
        float contactDepthMax,
        float feedbackSpeedDenominator,
        float feedbackScaleValue,
        u32 naturalId) {
    blendableVals = blendableValues;
    fakeContactBitmap = nullptr;
    fakeContactPeriodX = contactPeriodX;
    fakeContactPeriodZ = contactPeriodZ;
    fakeContactSpeedScale = contactSpeedScale;
    fakeContactDepthMax = contactDepthMax;
    feedbackSpeedDivisor = feedbackSpeedDenominator;
    feedbackScale = feedbackScaleValue;
    materialNaturalId = naturalId;
}

void CSceneVehicleMaterial::AttachFakeContactBitmap(CPlugBitmap *bitmap) {
    fakeContactBitmap = bitmap;
}
