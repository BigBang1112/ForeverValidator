#ifndef TMNF_STATIC_SOLID_ARCHIVE_NODE_KIND_H
#define TMNF_STATIC_SOLID_ARCHIVE_NODE_KIND_H

#include "engine/core/engine_types.h"
#include "format/archive/archive_class_ids.h"
namespace StaticSolidArchiveNodeKind {

constexpr u32 NoNode = 0xffffffffu;

inline int IsSurface(u32 classId) {
    return classId == TMNF_CLASS_CPlugSurface;
}

inline int IsSurfaceGeom(u32 classId) {
    return classId == TMNF_CLASS_CPlugSurfaceGeom;
}

inline int IsMaterial(u32 classId) {
    return classId == TMNF_CLASS_CPlugMaterial;
}

inline int IsTree(u32 classId) {
    return classId == TMNF_CLASS_CPlugTree ||
           classId == TMNF_CLASS_CPlugTreeVisualMip ||
           classId == TMNF_CLASS_CPlugTreeLight;
}

inline int IsVisualIndexedTriangles(u32 classId) {
    return classId == TMNF_CLASS_CPlugVisualIndexedTriangles ||
           classId == 0x09020000u;
}

}  // namespace StaticSolidArchiveNodeKind

#endif
