#pragma once

#include <optional>

#include "engine/core/gm_types.h"
#include "engine/resources/plug_file.h"
struct CSystemFid;

class SPlugVisibleFilter {
public:
    SPlugVisibleFilter(void) = default;
    SPlugVisibleFilter(std::uint32_t testedCategories,
                       std::uint32_t selectedCategories)
            : testedCategories_(testedCategories),
              selectedCategories_(selectedCategories) {}

    void SetDefaultValues(void);
    void ClearSelection(void);
    bool HasEquivalentSelection(const SPlugVisibleFilter &other) const;

private:
    std::uint32_t testedCategories_ = 0u;
    std::uint32_t selectedCategories_ = 0u;
};

class GmFrustum {
public:
    GmFrustum(void) = default;
    void SetFovY(float fovY, float ratioXY, float nearZ, float farZ);
    float HorizontalSlope(void) const;
    float VerticalSlope(void) const;
    float NearDistance(void) const;
    float FarDistance(void) const;

private:
    float horizontalSlope_ = 0.0f;
    float verticalSlope_ = 0.0f;
    float nearDistance_ = 0.0f;
    float farDistance_ = 0.0f;
};

class CPlugBitmapRender : public CPlug {
public:
    CPlugBitmapRender(void);
    ~CPlugBitmapRender(void) override;
    virtual void CleanRenderCache(void);

private:
    unsigned long sampleCount_ = 1ul;
    bool enabled_ = false;
    float renderDistance_ = 50.0f;
    SPlugVisibleFilter primaryFilter_;
    float primaryFilterDepth_ = 0.0f;
    SPlugVisibleFilter secondaryFilter_;
};

class CPlugBitmapRenderScene3d : public CPlugBitmapRender {
public:
    CPlugBitmapRenderScene3d(void);
    ~CPlugBitmapRenderScene3d(void) override;
    CSystemFid *SourceFid(void) const;
    void SetSource(CMwNod *source);

private:
    CMwNodRef<CMwNod> source_;
};

class CPlugBitmapRenderWater : public CPlugBitmapRender {
public:
    CPlugBitmapRenderWater(void);
    ~CPlugBitmapRenderWater(void) override;

private:
    GmIso4 cameraTransform_{};
    float minimumHeight_ = 0.0f;
    float waveStep_ = 0.05f;
    float heightOffset_ = 0.0f;
    float normalScaleX_ = 1.0f;
    float normalScaleY_ = 1.0f;
    float foamScale_ = 0.1f;
    float depthScale_ = 2.0f;
    GmFrustum frustum_;
};

class CPlugBitmapRenderCubeMap : public CPlugBitmapRender {
public:
    CPlugBitmapRenderCubeMap(void);
    ~CPlugBitmapRenderCubeMap(void) override;
    void CleanRenderCache(void) override;
    bool HasCachedCaptureBounds(void) const;

private:
    unsigned long faceCount_ = 6ul;
    float nearPlane_ = 0.5f;
    float farPlane_ = 1000.0f;
    GmVec3 center_{0.0f, 0.0f, 0.0f};
    float upAxis_ = 1.0f;
    std::optional<GmVec3> cachedCaptureBounds_;
    float minimumExposure_ = 0.0f;
    float maximumExposure_ = 1.0f;
};
