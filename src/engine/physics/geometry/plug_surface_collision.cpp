#include "engine/physics/geometry/plug_surface.h"

const GmSurf &LocatedGmSurf::Surface(void) const {
    return *surf;
}

void GmCollision::Neg(void) {
    impulseNormal.x = -impulseNormal.x;
    impulseNormal.y = -impulseNormal.y;
    impulseNormal.z = -impulseNormal.z;

    const GmLocalMaterialIndex oldLocalMaterialA = localMaterialA;
    const EPlugSurfaceMaterialId oldMaterialA = materialA;
    separation.x = -separation.x;
    localMaterialA = localMaterialB;
    localMaterialB = oldLocalMaterialA;
    materialA = materialB;
    materialB = oldMaterialA;
    separation.y = -separation.y;
    separation.z = -separation.z;

    extraNegated.x = -extraNegated.x;
    extraNegated.y = -extraNegated.y;
    extraNegated.z = -extraNegated.z;
}

int CPlugSurface::UsesSphereContactBuffer(void) const {
    const GmSurf *surface = Geometry();
    return surface != nullptr && surface->UsesSphereContactBuffer();
}

EPlugSurfaceMaterialId CPlugSurface::SurfaceMaterialIdFromLocalIndex(
        GmLocalMaterialIndex localIndex) const {
    return MaterialAt(localIndex.Index())->SurfaceMaterialId();
}

int GmSurf::ComputeCollision(
        const LocatedGmSurf &aRef,
        const LocatedGmSurf &bRef,
        CGmCollisionBuffer &collisionBufferRef) {
    if (aRef.surf == nullptr || bRef.surf == nullptr) {
        return 0;
    }
    return aRef.surf->Collide(aRef, bRef, collisionBufferRef);
}

int CPlugSurface::ComputeCollision(
        const SPlugSurfaceLocatedPair &pairRef,
        CGmCollisionBuffer &collisionBufferRef) {
    u32 firstNew = collisionBufferRef.GetCurrentCount();
    LocatedGmSurf surfaceB = {
        pairRef.SecondSurface().Geometry(),
        &pairRef.SecondLocation(),
        1,
    };
    LocatedGmSurf surfaceA = {
        pairRef.FirstSurface().Geometry(),
        &pairRef.FirstLocation(),
        1,
    };

    if (!GmSurf::ComputeCollision(surfaceA, surfaceB, collisionBufferRef)) {
        return 0;
    }

    u32 count = collisionBufferRef.GetCurrentCount();
    for (u32 collisionIndex = firstNew; collisionIndex < count;
         collisionIndex++) {
        GmCollision &collision =
                collisionBufferRef.GetCollision(collisionIndex);
        collision.materialA =
                pairRef.FirstSurface().SurfaceMaterialIdFromLocalIndex(
                        collision.localMaterialA);
        collision.materialB =
                pairRef.SecondSurface().SurfaceMaterialIdFromLocalIndex(
                        collision.localMaterialB);
    }

    return 1;
}
