#include "simulation/runtime/replay_vehicle_materials.h"
#include <utility>
#include <vector>

void ReplayVehicleMaterials::Reset() {
    container.Clear();
    fakeContactBitmap_.MwSetNod(nullptr);
    fakeContactFileImg_.MwSetNod(nullptr);
}

CPlugBitmap *ReplayVehicleMaterials::BuildFakeContactBitmap(
        const VehicleFakeContactTextureDefinition &texture) {
    if (!texture.HasExpectedShape()) {
        return nullptr;
    }

    fakeContactFileImg_ = MakeMwNod<CPlugFileImg>();
    CPlugFileImg *image = fakeContactFileImg_.Get();
    image->AssignPixels(
            texture.width,
            texture.height,
            1ul,
            CPlugFileImg::EFormat_RGB8,
            texture.rgbPixels);

    fakeContactBitmap_ = MakeMwNod<CPlugBitmap>();
    CPlugBitmap *bitmap = fakeContactBitmap_.Get();
    bitmap->SetImage(image);
    return bitmap;
}

void ReplayVehicleMaterials::Install(
        CSceneVehicleCar &car,
        const VehicleMaterialSetDefinition &materials) {
    Reset();
    car.ClearMaterialRemap();

    CPlugBitmap *bitmap = nullptr;
    if (materials.fakeContactTexture.has_value()) {
        bitmap = BuildFakeContactBitmap(*materials.fakeContactTexture);
    }

    for (const VehicleMaterialDefinition &definition : materials.materials) {
        CSceneVehicleMaterial::SBlendableVals blendableVals{};
        blendableVals.x = definition.blendableValues.x;
        blendableVals.y = definition.blendableValues.y;
        blendableVals.z = definition.blendableValues.z;
        blendableVals.w = definition.blendableValues.w;
        CSceneVehicleMaterial &material = container.AddMaterial();
        material.Configure(blendableVals,
                                     definition.fakeContactPeriodX,
                                     definition.fakeContactPeriodZ,
                                     definition.fakeContactSpeedScale,
                                     definition.fakeContactDepthMax,
                                     definition.feedbackSpeedDivisor,
                                     definition.feedbackScale,
                                     definition.naturalId);
        if (definition.usesFakeContactTexture && bitmap != nullptr) {
            material.AttachFakeContactBitmap(bitmap);
        }
    }

    car.BindMaterialContainer(container);
    car.SetMaterialRemap(materials.materialIndexByNaturalId);
}
