#include "engine/rendering/plug_tree.h"
#include <algorithm>

void GmBoxAligned::AddValidPlugTreeBox(const GmBoxAligned &other) {
    if (halfExtents.x < 0.0f) {
        *this = other;
        return;
    }
    if (other.halfExtents.x < 0.0f) {
        return;
    }

    GmVec3 minPoint = {
        center.x - halfExtents.x,
        center.y - halfExtents.y,
        center.z - halfExtents.z,
    };
    GmVec3 maxPoint = {
        center.x + halfExtents.x,
        center.y + halfExtents.y,
        center.z + halfExtents.z,
    };
    float otherX = other.center.x - other.halfExtents.x;
    float otherY = other.center.y - other.halfExtents.y;
    float otherZ = other.center.z - other.halfExtents.z;
    if (minPoint.x > otherX) {
        minPoint.x = otherX;
    }
    if (minPoint.y > otherY) {
        minPoint.y = otherY;
    }
    if (minPoint.z > otherZ) {
        minPoint.z = otherZ;
    }

    otherX = other.center.x + other.halfExtents.x;
    otherY = other.center.y + other.halfExtents.y;
    otherZ = other.center.z + other.halfExtents.z;
    if (maxPoint.x < otherX) {
        maxPoint.x = otherX;
    }
    if (maxPoint.y < otherY) {
        maxPoint.y = otherY;
    }
    if (maxPoint.z < otherZ) {
        maxPoint.z = otherZ;
    }
    SetMinMax(minPoint, maxPoint);
}

int CPlugTree::GetDecorationBoundingBox(
        int flags,
        GmBoxAligned &out) const {
    int hasBox = 0;
    CPlugVisual *provider = this->visual;

    if (provider != nullptr) {
        if (flags != 0) {
            provider->ComputeBoundingBox(0xffffffffu, 0xffffffffu);
        }
        out = provider->BoundingBox();
        hasBox = 1;
    }

    if (Surface() != nullptr && Surface()->GeometryNode() != nullptr) {
        CPlugSurfaceGeom *geom = Surface()->GeometryNode();
        if (flags != 0 && geom->Surface() != nullptr) {
            geom->ComputeBoundingBox();
        }
        if (!geom->Bounds().IsValidForPlugTreeRefresh()) {
            return hasBox;
        }
        if (hasBox) {
            out.AddValidPlugTreeBox(geom->Bounds());
        } else {
            out = geom->Bounds();
            hasBox = 1;
        }
    }

    return hasBox;
}

int CPlugTree::UpdateBoundingBox(int flags) {
    const u32 childCount = this->GetChildCount();
    GmBoxAligned box;
    int hasBox = this->GetDecorationBoundingBox(flags, box);

    if (!hasBox) {
        if (childCount == 0) {
            SetInvalidBox();
            return EmptyLeafRefreshReturn();
        }

        u32 childIndex = 0;
        while (childIndex < childCount) {
            CPlugTree *child = this->GetChild(childIndex);
            if (child->UpdateBoundingBox(flags) &&
                child->box.IsValidForPlugTreeRefresh()) {
                box = child->box;
                hasBox = 1;
                childIndex++;
                break;
            }
            childIndex++;
        }

        if (!hasBox) {
            SetInvalidBox();
            return 0;
        }

        while (childIndex < childCount) {
            CPlugTree *child = this->GetChild(childIndex);
            if (child->UpdateBoundingBox(flags) &&
                child->box.IsValidForPlugTreeRefresh()) {
                box.AddValidPlugTreeBox(child->box);
            }
            childIndex++;
        }
    } else {
        for (u32 childIndex = 0; childIndex < childCount; childIndex++) {
            CPlugTree *child = this->GetChild(childIndex);
            if (child->UpdateBoundingBox(flags) &&
                child->box.IsValidForPlugTreeRefresh()) {
                box.AddValidPlugTreeBox(child->box);
            }
        }
    }

    if (state_.usesLocation) {
        this->box.SetMult(box, this->localIso);
    } else {
        this->box = box;
    }
    return 1;
}
