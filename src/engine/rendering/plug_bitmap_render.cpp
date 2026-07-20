#include "engine/rendering/plug_bitmap_render.h"
#include <cmath>

#include "format/archive/archive_class_ids.h"

void SPlugVisibleFilter::SetDefaultValues(void) {
    testedCategories_ = 824u;
    selectedCategories_ = 0u;
}

void SPlugVisibleFilter::ClearSelection(void) {
    testedCategories_ = 0u;
    selectedCategories_ = 0u;
}

bool SPlugVisibleFilter::HasEquivalentSelection(
        const SPlugVisibleFilter &other) const {
    return testedCategories_ == other.testedCategories_ &&
           (testedCategories_ & selectedCategories_) ==
                   (other.testedCategories_ & other.selectedCategories_);
}

void GmFrustum::SetFovY(
        float fovY,
        float ratioXY,
        float nearZ,
        float farZ) {
    constexpr float DegreesToRadians = 3.1415927f / 180.0f;
    const float halfAngle = fovY * DegreesToRadians * 0.5f;
    verticalSlope_ = std::tan(halfAngle);
    horizontalSlope_ = verticalSlope_ * ratioXY;
    nearDistance_ = nearZ;
    farDistance_ = farZ;
}

float GmFrustum::HorizontalSlope(void) const {
    return horizontalSlope_;
}

float GmFrustum::VerticalSlope(void) const {
    return verticalSlope_;
}

float GmFrustum::NearDistance(void) const {
    return nearDistance_;
}

float GmFrustum::FarDistance(void) const {
    return farDistance_;
}

CPlugBitmapRender::CPlugBitmapRender(void) {
    primaryFilter_.SetDefaultValues();
    secondaryFilter_.SetDefaultValues();
}

CPlugBitmapRender::~CPlugBitmapRender(void) = default;

void CPlugBitmapRender::CleanRenderCache(void) {
}

unsigned long CPlugBitmapRender::GetMwClassId(void) const {
    return TMNF_CLASS_CPlugBitmapRender;
}

int CPlugBitmapRender::MwIsKindOf(unsigned long classId) const {
    return classId == TMNF_CLASS_CPlugBitmapRender ||
           classId == TMNF_CLASS_CPlug || classId == TMNF_CLASS_CMwNod;
}

CPlugBitmapRenderCamera::CPlugBitmapRenderCamera(void) = default;
CPlugBitmapRenderCamera::~CPlugBitmapRenderCamera(void) = default;

unsigned long CPlugBitmapRenderCamera::GetMwClassId(void) const {
    return TMNF_CLASS_CPlugBitmapRenderCamera;
}

int CPlugBitmapRenderCamera::MwIsKindOf(unsigned long classId) const {
    return classId == TMNF_CLASS_CPlugBitmapRenderCamera ||
           CPlugBitmapRender::MwIsKindOf(classId);
}

CPlugBitmapRenderScene3d::CPlugBitmapRenderScene3d(void) = default;
CPlugBitmapRenderScene3d::~CPlugBitmapRenderScene3d(void) = default;

unsigned long CPlugBitmapRenderScene3d::GetMwClassId(void) const {
    return TMNF_CLASS_CPlugBitmapRenderScene3d;
}

int CPlugBitmapRenderScene3d::MwIsKindOf(unsigned long classId) const {
    return classId == TMNF_CLASS_CPlugBitmapRenderScene3d ||
           CPlugBitmapRenderCamera::MwIsKindOf(classId);
}

CSystemFid *CPlugBitmapRenderScene3d::SourceFid(void) const {
    return source_ ? source_->fid : nullptr;
}

void CPlugBitmapRenderScene3d::SetSource(CMwNod *source) {
    source_.MwSetNod(source);
}

CPlugBitmapRenderWater::CPlugBitmapRenderWater(void) {
    cameraTransform_.SetIdentity();
    frustum_.SetFovY(60.0f, 1.0f, 0.5f, 1000.0f);
}

CPlugBitmapRenderWater::~CPlugBitmapRenderWater(void) = default;

unsigned long CPlugBitmapRenderWater::GetMwClassId(void) const {
    return TMNF_CLASS_CPlugBitmapRenderWater;
}

int CPlugBitmapRenderWater::MwIsKindOf(unsigned long classId) const {
    return classId == TMNF_CLASS_CPlugBitmapRenderWater ||
           CPlugBitmapRender::MwIsKindOf(classId);
}

CPlugBitmapRenderCubeMap::CPlugBitmapRenderCubeMap(void) {
    CleanRenderCache();
}

CPlugBitmapRenderCubeMap::~CPlugBitmapRenderCubeMap(void) = default;

unsigned long CPlugBitmapRenderCubeMap::GetMwClassId(void) const {
    return TMNF_CLASS_CPlugBitmapRenderCubeMap;
}

int CPlugBitmapRenderCubeMap::MwIsKindOf(unsigned long classId) const {
    return classId == TMNF_CLASS_CPlugBitmapRenderCubeMap ||
           CPlugBitmapRender::MwIsKindOf(classId);
}

void CPlugBitmapRenderCubeMap::CleanRenderCache(void) {
    cachedCaptureBounds_.reset();
}

bool CPlugBitmapRenderCubeMap::HasCachedCaptureBounds(void) const {
    return cachedCaptureBounds_.has_value();
}
