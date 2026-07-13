#include <initializer_list>
#include <math.h>
#include <stdio.h>
#include <vector>

#include "format/vehicle_tuning/archive_ids.h"
#include "format/vehicle_tuning/curve_definition.h"
#include "format/vehicle_tuning/curve_catalog.inc"
#include <new>
static bool CurveMatches(const VehicleTuningCurveRecord &record,
                         const VehicleTuningCurveBinding &ref) {
    return record.chunk.Matches(ref.chunkId) && record.nodRefIndex == ref.nodRefIndex;
}

static bool HasFiniteCurveValues(const VehicleTuningCurveRecord &record) {
    for (const ReplayTuningCurveKey &key : record.keys) {
        if (!isfinite(key.position) || !isfinite(key.value)) {
            return false;
        }
    }
    return true;
}

static ReplayTuningCurveDefinition
BuildCurveDefinition(u32 interpolationMode, const std::vector<ReplayTuningCurveKey> &keys) {
    ReplayTuningCurveDefinition definition;
    definition.interpolation = interpolationMode == 1u ? ReplayTuningCurveInterpolation::Constant
                                                       : ReplayTuningCurveInterpolation::Linear;
    definition.keys.reserve(keys.size());
    for (const ReplayTuningCurveKey &key : keys) {
        definition.keys.push_back({
                (key.position),
                (key.value),
        });
    }
    return definition;
}

static void AssignCurveDefinition(ReplayVehicleTuningCurves &curves,
                                  VehicleTuningCurveTarget target,
                                  ReplayTuningCurveDefinition definition) {
    switch (target) {
#define ASSIGN_CURVE(targetName, member)                                                           \
    case VehicleTuningCurveTarget::targetName:                                                     \
        curves.member = std::move(definition);                                                     \
        return
        ASSIGN_CURVE(M5AccelFromSpeedCurve, slipResponseAccelFromSpeed);
        ASSIGN_CURVE(LateralContactSlowDownFromSpeedCurve, lateralContactSlowDownFromSpeed);
        ASSIGN_CURVE(M5SteerSlowDownFromSpeedCurve, steerSlowDownFromSpeed);
        ASSIGN_CURVE(SteerDriveTorqueFromSpeedCurve, steeringDriveTorqueFromSpeed);
        ASSIGN_CURVE(MaxSideFrictionFromSpeedCurve, maxSideFrictionFromSpeed);
        ASSIGN_CURVE(RolloverLateralFromSpeedCurve, rolloverLateralFromSpeed);
        ASSIGN_CURVE(RolloverLateralCoefFromAngleCurve, rolloverLateralCoefficientFromAngle);
        ASSIGN_CURVE(M4SteerRadiusFromSpeedCurve, radiusSteeringRadiusFromSpeed);
        ASSIGN_CURVE(M4MaxFrictionForceFromSpeedCurve, radiusSteeringMaxFrictionFromSpeed);
        ASSIGN_CURVE(M5SlippingAccelFromSpeedCurve, slipResponseSlippingAccelFromSpeed);
        ASSIGN_CURVE(SplashVerticalImpulseCurve, splashVerticalImpulse);
        ASSIGN_CURVE(SplashHorizontalImpulseCurve, splashHorizontalImpulse);
        ASSIGN_CURVE(WaterFrictionFromSpeedCurve, waterFrictionFromSpeed);
        ASSIGN_CURVE(M6DamperAbsorbModulationCurve, suspensionDamperAbsorbModulation);
        ASSIGN_CURVE(M6RearGearAccelFromSpeedCurve, reverseGearAccelFromSpeed);
        ASSIGN_CURVE(M6RolloverLateralFromSpeedRatioCurve, burnoutRolloverLateralFromSpeedRatio);
        ASSIGN_CURVE(M6BurnoutRadiusFromSpeedCurve, burnoutRadiusFromSpeed);
        ASSIGN_CURVE(M6LateralSpeedFromBurnoutRadiusCurve, burnoutLateralSpeedFromRadius);
        ASSIGN_CURVE(M6DonutRolloverFromSpeedCurve, donutRolloverFromSpeed);
        ASSIGN_CURVE(M6BurnoutRolloverFromSpeedCurve, burnoutRolloverFromSpeed);
        ASSIGN_CURVE(AirControlZScaleCurve, airControlZScale);
        ASSIGN_CURVE(WheelVisualSteerAngleFromSpeedCurve, wheelVisualSteerAngleFromSpeed);
        ASSIGN_CURVE(SurfaceFeedbackCurve, surfaceFeedback);
#undef ASSIGN_CURVE
    }
}

static ReplayTuningCurveDefinition
AuthoredLinearCurve(std::initializer_list<ReplayTuningCurveKey> keys) {
    ReplayTuningCurveDefinition definition;
    definition.interpolation = ReplayTuningCurveInterpolation::Linear;
    definition.keys.assign(keys.begin(), keys.end());
    return definition;
}

int ReadVehicleTuningCurves(const VehicleTuningArchiveDocument &archive,
                            std::vector<VehicleTuningCurveRecord> &records) {
    records.clear();
    try {
        records.reserve(MaxVehicleTuningCurveCount);
    } catch (const std::bad_alloc &) {
        records.clear();
        return 6;
    }
    size_t offset = archive.ActiveStart();
    int sawFacade = 0;
    while (offset < archive.ActiveEnd() && !sawFacade) {
        if (!ReadVehicleTuningCurveChunk(archive, &offset, &records, &sawFacade)) {
            records.clear();
            return 8;
        }
    }
    if (!sawFacade) {
        records.clear();
        return 9;
    }
    return 0;
}

int ApplyVehicleTuningCurves(const std::vector<VehicleTuningCurveRecord> &records,
                             ReplayVehicleTuningDefinition &definition) {
    if (records.size() > MaxVehicleTuningCurveCount) {
        return 1;
    }
    for (size_t recordIndex = 0; recordIndex < records.size(); recordIndex++) {
        const VehicleTuningCurveRecord &record = records[recordIndex];
        if (!HasFiniteCurveValues(record)) {
            fprintf(stderr,
                    "invalid tuning archive curve record %zu chunk=0x%08x nod=%u "
                    "count=%zu\n",
                    recordIndex, record.chunk.ValueForDiagnostic(), record.nodRefIndex,
                    record.keys.size());
            return 2;
        }
    }

    ReplayVehicleTuningCurves &curves = definition.curves;
    curves = ReplayVehicleTuningCurves{};

    try {
        for (const VehicleTuningCurveBinding &ref : VehicleTuningCurveBindings) {
            const VehicleTuningCurveRecord *found = nullptr;
            for (const VehicleTuningCurveRecord &record : records) {
                if (CurveMatches(record, ref)) {
                    found = &record;
                    break;
                }
            }
            if (found == nullptr) {
                fprintf(stderr, "missing tuning archive curve chunk=0x%08x nod=%u\n",
                        ArchiveChunkIdValue(ref.chunkId), ref.nodRefIndex);
                return 3;
            }
            AssignCurveDefinition(curves, ref.target,
                                  BuildCurveDefinition(found->interpolationMode, found->keys));
        }

        curves.vehicleFeedbackRamp1 = AuthoredLinearCurve({
                {0.0f, 0.0f},
                {30.0f, 0.1f},
                {100.0f, 0.3f},
        });
        curves.vehicleFeedbackRamp0 = AuthoredLinearCurve({
                {0.0f, 0.0f},
                {30.0f, 0.01f},
                {100.0f, 0.05f},
        });
        curves.vehicleDefault30To100 = AuthoredLinearCurve({
                {0.0f, 0.0f},
                {30.0f, 0.0f},
                {100.0f, 1.0f},
        });
    } catch (const std::bad_alloc &) {
        curves = ReplayVehicleTuningCurves{};
        return 4;
    }
    return 0;
}
