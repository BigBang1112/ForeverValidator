#include "engine/resources/plug_file.h"
#include <array>

#include "engine/resources/system_fid_file.h"
#include "engine/resources/system_file_name.h"
CPlug::CPlug(void) = default;
CPlug::~CPlug(void) = default;
CPlugFile::CPlugFile(void) = default;
CPlugFile::~CPlugFile(void) = default;

CPlugFile::SPlugFileType::SPlugFileType(void) = default;

CPlugFile::SPlugFileType::~SPlugFileType(void) = default;

AssetType CPlugFile::GetAssetTypeFromFileName(
        const CFastStringInt &filename) {
    struct RegisteredFileFormat {
        const char *extension;
        AssetType type;
    };
    static constexpr std::array<RegisteredFileFormat, 8> formats{{
        {".zip", AssetType::PlugFileZip},
        {".bik", AssetType::PlugFileBink},
        {".dds", AssetType::PlugFileImg},
        {".tga", AssetType::PlugFileImg},
        {".png", AssetType::PlugFileImg},
        {".jpg", AssetType::PlugFileImg},
        {".jpeg", AssetType::PlugFileImg},
        {".bmp", AssetType::PlugFileImg},
    }};
    for (const RegisteredFileFormat &format : formats) {
        if (CSystemFileName::IsExtension(
                    filename, format.extension) != 0) {
            return format.type;
        }
    }
    return AssetType::Unknown;
}

AssetType CPlugFile::GetAssetTypeFromFid(CSystemFidFile *fid) {
    if (fid == nullptr) {
        return AssetType::Unknown;
    }

    CSystemFidFile *loadableFid = dynamic_cast<CSystemFidFile *>(
            fid->ParametrizedGetLoadableFid());
    return loadableFid != nullptr
            ? GetAssetTypeFromFileName(loadableFid->FileName())
            : AssetType::Unknown;
}
