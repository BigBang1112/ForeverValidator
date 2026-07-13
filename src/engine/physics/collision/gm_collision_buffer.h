#pragma once

#include "engine/physics/geometry/gm_surface.h"
struct CGmCollisionBuffer {
    virtual GmCollision &GetCollision(unsigned long index) = 0;
    virtual GmCollision &AddCollision(void) = 0;
    virtual unsigned long GetCurrentCount(void) = 0;

protected:
    ~CGmCollisionBuffer(void) = default;
};
