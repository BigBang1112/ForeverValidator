#include "engine/game/material_render_definition.h"

#include <new>
#include <utility>

#include "format/archive/archive_class_ids.h"

bool MaterialRenderDefinition::HasBitmapRenderWater(void) const {
    for (const MaterialRenderBitmapDefinition &bitmap : bitmaps_) {
        if (bitmap.renderClassId == TMNF_CLASS_CPlugBitmapRenderWater) {
            return true;
        }
    }
    return false;
}

const std::string &MaterialRenderDefinition::MaterialPlainPath(void) const {
    return materialPlainPath_;
}

const std::string &MaterialRenderDefinition::MaterialSelectedPath(void) const {
    return materialSelectedPath_;
}

const std::string &
MaterialRenderDefinition::MaterialModelPlainPath(void) const {
    return materialModelPlainPath_;
}

const std::string &
MaterialRenderDefinition::MaterialModelSelectedPath(void) const {
    return materialModelSelectedPath_;
}

const std::string &MaterialRenderDefinition::ShaderPlainPath(void) const {
    return shaderPlainPath_;
}

const std::string &MaterialRenderDefinition::ShaderSelectedPath(void) const {
    return shaderSelectedPath_;
}

const std::vector<MaterialRenderBitmapDefinition> &
MaterialRenderDefinition::Bitmaps(void) const {
    return bitmaps_;
}

u32 MaterialRenderDefinition::ShaderFlags(void) const {
    return shaderFlags_;
}

u32 MaterialRenderDefinition::ShaderRenderDeviceWord(void) const {
    return shaderRenderDeviceWord_;
}

void MaterialRenderDefinition::SetMaterialPaths(
        std::string plainPath,
        std::string selectedPath) {
    materialPlainPath_ = std::move(plainPath);
    materialSelectedPath_ = std::move(selectedPath);
}

void MaterialRenderDefinition::SetMaterialModelPaths(
        std::string plainPath,
        std::string selectedPath) {
    materialModelPlainPath_ = std::move(plainPath);
    materialModelSelectedPath_ = std::move(selectedPath);
}

void MaterialRenderDefinition::SetShaderPaths(
        std::string plainPath,
        std::string selectedPath) {
    shaderPlainPath_ = std::move(plainPath);
    shaderSelectedPath_ = std::move(selectedPath);
}

void MaterialRenderDefinition::SetShaderFlags(u32 flags) {
    shaderFlags_ = flags;
}

void MaterialRenderDefinition::SetShaderRenderDeviceWord(u32 deviceWord) {
    shaderRenderDeviceWord_ = deviceWord;
}

bool MaterialRenderDefinition::AppendBitmap(
        MaterialRenderBitmapDefinition bitmap) {
    try {
        bitmaps_.push_back(std::move(bitmap));
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

void MaterialRenderDefinition::ReplaceBitmaps(
        std::vector<MaterialRenderBitmapDefinition> bitmaps) {
    bitmaps_ = std::move(bitmaps);
}
