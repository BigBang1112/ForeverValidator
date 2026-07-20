#include "format/archive/mw_deprecated.h"

u32 CMwDeprecated::WrapClassId(u32 classId) {
    switch (classId) {
    case 0x2403f000u: return 0x03093000u;
    case 0x2401b000u: return 0x03092000u;
    case 0x24003000u: return 0x03043000u;
    case 0x2403c000u: return 0x0301b000u;
    case 0x2400c000u: return 0x0305b000u;
    default: return classId;
    }
}
