#pragma once

#include "engine/resources/asset_type.h"
#include "engine/core/engine_types.h"
#include "engine/core/fast_string.h"
#include "engine/core/mw_id.h"
#include "engine/rendering/plug.h"
struct CSystemFidFile;

struct CPlugFile : CPlug {
    struct SPlugFileType {
        CFastString extension;

        SPlugFileType(void);
        ~SPlugFileType(void);
    };

    CPlugFile(void);
    ~CPlugFile(void) override;
    static AssetType GetAssetTypeFromFileName(
            const CFastStringInt &filename);
    static AssetType GetAssetTypeFromFid(CSystemFidFile *fid);
};

struct CPlugFileSnd : CPlugFile {
};
