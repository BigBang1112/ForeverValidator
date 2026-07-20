#pragma once

#include <optional>

#include "engine/game/material_render_definition.h"

class CPlugFilePack;

std::optional<MaterialRenderDefinition> DecodeMaterialRenderArchive(
        const CPlugFilePack &pack,
        const char *materialPlainPath);
