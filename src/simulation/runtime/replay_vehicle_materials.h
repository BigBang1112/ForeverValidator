#ifndef TMNF_REPLAY_VEHICLE_MATERIALS_H
#define TMNF_REPLAY_VEHICLE_MATERIALS_H

#include "engine/game/replay_vehicle_simulation_definition.h"
#include "engine/rendering/plug_bitmap.h"
#include "engine/resources/plug_file_img.h"
#include "engine/scene/scene_vehicle_car.h"
#include "engine/scene/scene_vehicle_material.h"
class ReplayVehicleMaterials {
public:
    void Install(CSceneVehicleCar &car,
                 const VehicleMaterialSetDefinition &materials);

private:
    void Reset();
    CPlugBitmap *BuildFakeContactBitmap(
            const VehicleFakeContactTextureDefinition &texture);

    CSceneVehicleMaterialContainer container;
    CMwNodRef<CPlugFileImg> fakeContactFileImg_;
    CMwNodRef<CPlugBitmap> fakeContactBitmap_;
};

#endif // TMNF_REPLAY_VEHICLE_MATERIALS_H
