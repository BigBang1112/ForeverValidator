#include "format/static_solid/static_solid_archive_tree_state_flags.h"
u32 StaticSolidArchiveTreeStateFlags::
        FromCPlugTreeChunk0904f010(u32 rawFlags) {
    u32 flags = rawFlags & 0x1ffffu;
    flags = (flags & 0xffff9fffu) | 0x2000u |
            (((0xffffffffu - (((flags | 0x2000u) >> 14u) & 1u)) & 1u)
             << 14u) |
            0x8800u;
    return flags;
}

u32 StaticSolidArchiveTreeStateFlags::Low17Or2800(
        u32 rawFlags) {
    return (rawFlags & 0x1ffffu) | 0x2800u;
}

u32 StaticSolidArchiveTreeStateFlags::FullOr2800(
        u32 rawFlags) {
    return rawFlags | 0x2800u;
}

u32 StaticSolidArchiveTreeStateFlags::FullOr2000(
        u32 rawFlags) {
    return rawFlags | 0x2000u;
}
