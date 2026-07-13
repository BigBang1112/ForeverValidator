#pragma once

#include "engine/rendering/plug_material.h"
#include "engine/core/engine_types.h"
#include <new>
#include <vector>

#include "format/static_solid/static_solid_geometry_decoder.h"
#include "format/static_solid/static_solid_archive_id.h"
class StaticSolidMaterialRemaps {
public:
    void Clear() {
        entries.clear();
    }

    int Add(StaticSolidArchiveId payload,
            CPlugMaterial *sourceMaterial,
            CPlugMaterial *replacementMaterial,
            CPlugMaterial *decorationSkinMaterial) {
        if (sourceMaterial == nullptr) {
            return 1;
        }
        try {
            entries.push_back({payload,
                               sourceMaterial,
                               replacementMaterial,
                               decorationSkinMaterial});
        } catch (const std::bad_alloc &) {
            return 0;
        }
        return 1;
    }

    CPlugMaterial *Resolve(
            StaticSolidArchiveId payload,
            StaticSolidMaterialRemap kind,
            const CPlugMaterial *sourceMaterial) const {
        if (sourceMaterial == nullptr) {
            return nullptr;
        }
        for (const Entry &entry : entries) {
            if (!entry.payload.Matches(payload) ||
                entry.sourceMaterial != sourceMaterial) {
                continue;
            }
            CPlugMaterial *target =
                    kind == StaticSolidMaterialRemap::
                                    Replacement
                            ? entry.replacementMaterial
                            : entry.decorationSkinMaterial;
            if (target != nullptr) {
                return target;
            }
        }
        return nullptr;
    }

    u32 Count() const {
        return entries.size() <= UINT32_MAX ? (u32)entries.size()
                                            : UINT32_MAX;
    }

private:
    struct Entry {
        StaticSolidArchiveId payload =
                StaticSolidArchiveId::Invalid();
        CPlugMaterial *sourceMaterial = nullptr;
        CPlugMaterial *replacementMaterial = nullptr;
        CPlugMaterial *decorationSkinMaterial = nullptr;
    };

    std::vector<Entry> entries;
};
