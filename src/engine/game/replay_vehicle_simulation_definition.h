#ifndef TMNF_REPLAY_VEHICLE_SIMULATION_DEFINITION_H
#define TMNF_REPLAY_VEHICLE_SIMULATION_DEFINITION_H

#include <cstdint>
#include <optional>
#include <vector>

#include "engine/physics/dynamics/replay_dyna_parameters.h"
#include "engine/game/replay_vehicle_tuning_definition.h"
#include "engine/game/replay_vehicle_wheel_definition.h"
inline constexpr std::uint32_t VehicleFakeContactTextureWidth = 128u;
inline constexpr std::uint32_t VehicleFakeContactTextureHeight = 128u;
inline constexpr std::uint32_t VehicleFakeContactTextureBytesPerPixel = 3u;
inline constexpr std::uint32_t VehicleFakeContactTexturePixelBytes =
        VehicleFakeContactTextureWidth *
        VehicleFakeContactTextureHeight *
        VehicleFakeContactTextureBytesPerPixel;

struct VehicleMaterialBlendValues {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};

struct VehicleMaterialDefinition {
    std::uint32_t naturalId = 0u;
    VehicleMaterialBlendValues blendableValues{};
    float fakeContactPeriodX = 0.0f;
    float fakeContactPeriodZ = 0.0f;
    float fakeContactSpeedScale = 0.0f;
    float fakeContactDepthMax = 0.0f;
    float feedbackSpeedDivisor = 0.0f;
    float feedbackScale = 0.0f;
    bool usesFakeContactTexture = false;

    bool HasFiniteParameters() const;
};

struct VehicleFakeContactTextureDefinition {
    std::uint32_t width = VehicleFakeContactTextureWidth;
    std::uint32_t height = VehicleFakeContactTextureHeight;
    std::vector<std::uint8_t> rgbPixels;

    bool HasExpectedShape() const {
        return width == VehicleFakeContactTextureWidth &&
               height == VehicleFakeContactTextureHeight &&
               rgbPixels.size() == VehicleFakeContactTexturePixelBytes;
    }
};

struct VehicleMaterialSetDefinition {
    std::vector<VehicleMaterialDefinition> materials;
    std::vector<std::uint32_t> materialIndexByNaturalId;
    std::optional<VehicleFakeContactTextureDefinition> fakeContactTexture;

    bool IsValid() const;
};

struct VehicleInitialParameters {
    float linearSpeedCap = 277.77777f;
    float reverseGearSpeedThreshold = 10.0f;
    GmBoxAligned waterBoxLocal{
            {0.0f, 0.0f, 0.0f},
            {-1.0f, -1.0f, -1.0f}};
};

struct VehicleSimulationDefinition {
    VehicleInitialParameters initialParameters{};
    ReplayVehicleTuningDefinition tuning{};
    VehicleWheelSetDefinition wheels{};
    VehicleCollisionModelDefinition collisionModel{};
    VehicleMaterialSetDefinition materials{};
    ReplayDynaParameters dynaParameters{};
};

#endif
