#pragma once

#include <optional>

#include "engine/core/engine_types.h"
#include "engine/game/material_definition.h"
std::optional<MaterialSurfaceDefinition> DecodeMaterialArchive(
        const unsigned char *bytes,
        u32 byteCount);
