#pragma once

#include <optional>

#include "engine/resources/catalog_asset_repository.h"
class CGameCtnReplayMapInput;
class CPlugFilePack;

std::optional<CatalogDecorationSizeDefinition>
DecodeReplayDecorationSizeArchive(
        const unsigned char *archiveBytes,
        u32 archiveByteCount);

std::optional<CatalogDecorationSizeDefinition>
ResolveReplayDecorationSize(
        const CPlugFilePack &pack,
        const CGameCtnReplayMapInput &mapInput);
