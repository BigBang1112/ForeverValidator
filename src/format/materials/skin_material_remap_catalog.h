#pragma once

#include <memory>
#include <optional>
#include <string>

class CPlugFilePack;

struct TerrainModifierDirtRemapPaths {
    std::string preferred;
    std::string fallback;
};

class SkinMaterialRemapCatalog {
public:
    SkinMaterialRemapCatalog();
    ~SkinMaterialRemapCatalog();

    SkinMaterialRemapCatalog(const SkinMaterialRemapCatalog &) = delete;
    SkinMaterialRemapCatalog &operator=(
            const SkinMaterialRemapCatalog &) = delete;

    bool Load(const CPlugFilePack &pack);
    std::optional<TerrainModifierDirtRemapPaths> FindTerrainModifierDirt(
            const std::string &sourceMaterialPath) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
