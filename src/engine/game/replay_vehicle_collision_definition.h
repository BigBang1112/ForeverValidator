#ifndef TMNF_REPLAY_VEHICLE_COLLISION_DEFINITION_H
#define TMNF_REPLAY_VEHICLE_COLLISION_DEFINITION_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <optional>
#include <utility>
#include <vector>

#include "engine/core/gm_types.h"
#include "engine/game/surface_material.h"
enum class VehicleCollisionRole : std::uint8_t {
    BodyLongitudinal,
    BodyMain,
    BodyRear,
    BodyFront,
    FrontLeftWheel,
    FrontRightWheel,
    RearLeftWheel,
    RearRightWheel,
};

inline constexpr std::array<VehicleCollisionRole, 8u>
        VehicleCollisionRolesInDetectorOrder{
                VehicleCollisionRole::BodyLongitudinal,
                VehicleCollisionRole::BodyMain,
                VehicleCollisionRole::BodyRear,
                VehicleCollisionRole::BodyFront,
                VehicleCollisionRole::FrontLeftWheel,
                VehicleCollisionRole::FrontRightWheel,
                VehicleCollisionRole::RearLeftWheel,
                VehicleCollisionRole::RearRightWheel,
        };

inline constexpr std::size_t VehicleCollisionRoleIndex(
        VehicleCollisionRole role) {
    switch (role) {
        case VehicleCollisionRole::BodyLongitudinal: return 0u;
        case VehicleCollisionRole::BodyMain: return 1u;
        case VehicleCollisionRole::BodyRear: return 2u;
        case VehicleCollisionRole::BodyFront: return 3u;
        case VehicleCollisionRole::FrontLeftWheel: return 4u;
        case VehicleCollisionRole::FrontRightWheel: return 5u;
        case VehicleCollisionRole::RearLeftWheel: return 6u;
        case VehicleCollisionRole::RearRightWheel: return 7u;
    }
    return VehicleCollisionRolesInDetectorOrder.size();
}

inline constexpr bool IsVehicleWheelCollisionRole(VehicleCollisionRole role) {
    return role == VehicleCollisionRole::FrontLeftWheel ||
           role == VehicleCollisionRole::FrontRightWheel ||
           role == VehicleCollisionRole::RearLeftWheel ||
           role == VehicleCollisionRole::RearRightWheel;
}

inline constexpr VehicleCollisionRole VehicleWheelCollisionRole(
        std::size_t wheelIndex) {
    switch (wheelIndex) {
        case 0u: return VehicleCollisionRole::FrontLeftWheel;
        case 1u: return VehicleCollisionRole::FrontRightWheel;
        case 2u: return VehicleCollisionRole::RearRightWheel;
        case 3u: return VehicleCollisionRole::RearLeftWheel;
        default: return VehicleCollisionRole::BodyMain;
    }
}

struct VehicleCollisionShapeDefinition {
    std::uint32_t surfaceType = 1u;
    GmIso4 localPose{};
    GmBoxAligned localBounds{};
    GmLocalMaterialIndex localMaterial{};
    EPlugSurfaceMaterialId surfaceMaterial =
            EPlugSurfaceMaterialId_Concrete;
};

struct VehicleCollisionShapeEntry {
    std::optional<VehicleCollisionRole> wheelRole;
    std::optional<std::size_t> parentShapeIndex;
    VehicleCollisionShapeDefinition shape;
};

class VehicleCollisionModelDefinition {
public:
    bool AddBodyShape(
            VehicleCollisionShapeDefinition shape,
            std::optional<std::size_t> parentShapeIndex = std::nullopt) {
        try {
            shapes.push_back(VehicleCollisionShapeEntry{
                    std::nullopt, parentShapeIndex, std::move(shape)});
        } catch (const std::bad_alloc &) {
            return false;
        }
        return true;
    }

    bool SetWheelShape(VehicleCollisionRole role,
                       VehicleCollisionShapeDefinition shape,
                       std::optional<std::size_t> parentShapeIndex =
                               std::nullopt) {
        if (!IsVehicleWheelCollisionRole(role) || Shape(role) != nullptr) {
            return false;
        }
        try {
            shapes.push_back(VehicleCollisionShapeEntry{
                    role, parentShapeIndex, std::move(shape)});
        } catch (const std::bad_alloc &) {
            return false;
        }
        return true;
    }

    const VehicleCollisionShapeDefinition *Shape(VehicleCollisionRole role)
            const {
        for (const VehicleCollisionShapeEntry &entry : shapes) {
            if (entry.wheelRole == role) {
                return &entry.shape;
            }
        }
        return nullptr;
    }

    const std::vector<VehicleCollisionShapeEntry> &ShapesInArchiveOrder()
            const {
        return shapes;
    }

    bool IsComplete() const {
        bool hasBody = false;
        for (std::size_t index = 0u; index < shapes.size(); index++) {
            const VehicleCollisionShapeEntry &entry = shapes[index];
            if (entry.parentShapeIndex.has_value() &&
                *entry.parentShapeIndex >= index) {
                return false;
            }
            hasBody = hasBody || !entry.wheelRole.has_value();
        }
        if (!hasBody) {
            return false;
        }
        for (VehicleCollisionRole role :
             VehicleCollisionRolesInDetectorOrder) {
            if (IsVehicleWheelCollisionRole(role) &&
                Shape(role) == nullptr) {
                return false;
            }
        }
        return true;
    }

private:
    std::vector<VehicleCollisionShapeEntry> shapes;
};

#endif
