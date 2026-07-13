#ifndef TMNF_BLOCKINFO_DESCRIPTOR_MOBIL_PARSER_H
#define TMNF_BLOCKINFO_DESCRIPTOR_MOBIL_PARSER_H

#include "engine/core/engine_types.h"
#include <optional>

#include "format/pack/block_info_catalog/blockinfo_descriptor_parse_state.h"
struct CPlugFilePack;

struct BlockInfoDescriptorParseRequest {
    const unsigned char *bytes = nullptr;
    u32 byteCount = 0u;
    const char *descriptorPath = nullptr;
    const CPlugFilePack *installedPack = nullptr;
    bool continueAfterMainBlockInfoChunk = false;
};

struct BlockInfoDescriptorParseOutput {
    BlockInfoDescriptorParseResult descriptor;
    BlockInfoParsedMobilCollection models;
};

struct BlockInfoDescriptorMobilParser {
    static std::optional<BlockInfoDescriptorParseOutput> ParseDescriptorAndModels(
            const BlockInfoDescriptorParseRequest &request);
    static void TryParseHelperPaths(
            const BlockInfoDescriptorParseRequest &request,
            BlockInfoDescriptorParseResult &descriptor);
};

#endif
