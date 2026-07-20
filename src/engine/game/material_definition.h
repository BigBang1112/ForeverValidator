#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "engine/core/engine_types.h"
#include "engine/game/material_render_definition.h"
enum EPlugSurfaceMaterialId : std::uint8_t;
struct CPlugMaterial;
struct MaterialSurfaceDefinitionFormatAccess;

class MaterialAssetHandle {
public:
    constexpr MaterialAssetHandle() = default;

    static constexpr MaterialAssetHandle FromRepositoryIndex(u32 index) {
        return MaterialAssetHandle(index + 1u);
    }

    constexpr bool IsValid() const { return value_ != 0u; }
    constexpr u32 RepositoryIndex() const { return value_ - 1u; }

    friend constexpr bool operator==(MaterialAssetHandle lhs,
                                     MaterialAssetHandle rhs) {
        return lhs.value_ == rhs.value_;
    }

    friend constexpr bool operator!=(MaterialAssetHandle lhs,
                                     MaterialAssetHandle rhs) {
        return !(lhs == rhs);
    }

private:
    explicit constexpr MaterialAssetHandle(u32 value) : value_(value) {}

    u32 value_ = 0u;
};

class MaterialSurfaceDefinition {
public:
    bool IsDefined() const;
    EPlugSurfaceMaterialId MaterialId() const;
    void ApplyTo(CPlugMaterial &material) const;

private:
    std::optional<EPlugSurfaceMaterialId> materialId_;

    friend struct MaterialSurfaceDefinitionFormatAccess;
};

struct MaterialAssetDefinition {
    MaterialAssetHandle asset;
    MaterialSurfaceDefinition surface;
    MaterialRenderDefinition render;
};

struct MaterialRemapCollection {
    std::optional<MaterialAssetDefinition> terrainModifierDirt;
};

struct ResolvedMaterialDefinition {
    MaterialAssetDefinition material;
    MaterialRemapCollection remaps;
};

class MaterialAssetRepository {
public:
    virtual ~MaterialAssetRepository() = default;

    virtual std::optional<ResolvedMaterialDefinition> ResolveMaterial(
            std::string_view identifier) = 0;
    virtual std::optional<ResolvedMaterialDefinition> ResolveMaterialPath(
            std::string_view plainPath) = 0;
};
