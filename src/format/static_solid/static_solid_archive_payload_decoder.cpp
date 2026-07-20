#include "format/static_solid/static_solid_archive_payload_decoder.h"
#include <stddef.h>

#include "format/archive/tmnf_gbx_body_reader.h"
#include "format/pack/installed/plug_file_pack.h"
#include "format/static_solid/static_solid_archive_payload_bytes.h"
#include "format/static_solid/static_solid_archive_payload_stream.h"

namespace {

int RequiredPayloadHasValidReferencePrefix(
        const SNat128 &key,
        const StaticSolidArchivePayload &payload,
        StaticSolidArchiveRawBytes rawPayload) {
    CGameCtnReplayStaticSolidDecodedPayload prefix;
    const int decoded = payload.IsEncrypted()
            ? CGameCtnReplayStaticSolidArchivePayloadReader::
                      DecodeReferenceTablePrefixWithStreamFeedback(
                              key, payload, rawPayload, &prefix)
            : CGameCtnReplayStaticSolidRawPayloadDecoder::Decode(
                      key, payload, rawPayload, &prefix);
    u32 classId = 0u;
    GbxBodyReferenceTable references;
    return decoded && prefix.IsReady() &&
           GbxBodyOffsetReader::TryParseWithReferences(
                   prefix.Bytes(),
                   prefix.ByteCount(),
                   &classId,
                   &references) &&
           classId == payload.DescriptorClassId();
}

}  // namespace

int StaticSolidArchiveLoadSession::DecodeAndAppendArchive(
        StaticSolidArchivePayload payloadAsset,
        CGameCtnReplayArchiveStaticModelCollection *archiveModels,
        CGameCtnReplayStaticSolidDescriptorDependencyQueue *dependencyQueue,
        int required) {
    if (dependencyQueue == nullptr) {
        return 0;
    }
    const SNat128 *packKey = payloadPackSource.Key();
    if (packKey == nullptr) {
        return 0;
    }
    const CGameCtnReplayStaticSolidDecoratorTreeCursor
            descriptorDecoratorTreeStart =
                    MutableArchiveGraph().DecoratorTreeEnd();
    if (!LoadRawPayloadBytesFromPack(&payloadAsset)) {
        return 0;
    }
    const StaticSolidArchiveRawBytes rawPayload =
            RawPayloadBytesForPayload(payloadAsset);
    if (!rawPayload.IsAvailable()) {
        return 0;
    }

    CGameCtnReplayStaticSolidDecodedPayload decodedPayload;
    int decoded = 0;
    /*
     * Encrypted GBX bodies interleave decryption with class and body feedback.
     * Decrypting an entire payload first can expose a syntactically valid but
     * incomplete node archive, so full-payload decoding is valid only for
     * unencrypted data.
     */
    if (!payloadAsset.IsEncrypted()) {
        decoded = CGameCtnReplayStaticSolidRawPayloadDecoder::Decode(
                *packKey,
                payloadAsset,
                rawPayload,
                &decodedPayload);
    }
    if (decoded) {
        if (!CGameCtnReplayStaticSolidArchivePayloadReader::EnqueueExternalRefDependencies(
                    payloadAsset,
                    decodedPayload.Bytes(),
                    decodedPayload.ByteCount(),
                    this,
                    dependencyQueue)) {
            return 0;
        }
        CGameCtnReplayStaticSolidArchiveDecodeStats stats;
        const StaticSolidArchiveRollback checkpoint =
                MarkDecodedArchiveTail(archiveModels);
        const StaticSolidArchiveId payload =
                StaticSolidArchiveId::
                        FromIndex(static_cast<u32>(payloads.size()));
        if (!CGameCtnReplayStaticSolidArchivePayloadReader::ParseDecodedIntoStore(
                    payloadAsset,
                    decodedPayload.Bytes(),
                    decodedPayload.ByteCount(),
                    this,
                    archiveModels,
                    payload,
                    &stats)) {
            if (!RestoreDecodedArchiveTail(checkpoint, archiveModels)) {
                return 0;
            }
            decodedPayload.Clear();
            decoded = 0;
        }
    }
    if (!decoded &&
        payloadAsset.IsEncrypted()) {
        CGameCtnReplayStaticSolidArchiveDecodeStats stats;
        const StaticSolidArchiveRollback checkpoint =
                MarkDecodedArchiveTail(archiveModels);
        const StaticSolidArchiveId payload =
                StaticSolidArchiveId::
                        FromIndex(static_cast<u32>(payloads.size()));
        decoded = CGameCtnReplayStaticSolidArchivePayloadReader::DecodeWithStreamFeedback(
                *packKey,
                payloadAsset,
                rawPayload,
                this,
                archiveModels,
                payload,
                &decodedPayload,
                &stats);
        if (decoded) {
            if (!CGameCtnReplayStaticSolidArchivePayloadReader::EnqueueExternalRefDependencies(
                        payloadAsset,
                        decodedPayload.Bytes(),
                        decodedPayload.ByteCount(),
                        this,
                        dependencyQueue)) {
                return 0;
            }
        } else if (!RestoreDecodedArchiveTail(checkpoint, archiveModels)) {
            return 0;
        }
    }
    if (decoded) {
        if (!AppendDecodedPayload(decodedPayload.Bytes(),
                                  decodedPayload.ByteCount(),
                                  &payloadAsset)) {
            return 0;
        }
    } else {
        if (required &&
            !RequiredPayloadHasValidReferencePrefix(
                    *packKey, payloadAsset, rawPayload)) {
            return 0;
        }
        payloadAsset.MarkDecodeUnavailable();
    }
    decodedPayload.Clear();
    if (!AddPayloadAsset(payloadAsset)) {
        return 0;
    }
    return ArchiveGraph().QueueDecoratorTreeDependencies(
            this,
            dependencyQueue,
            descriptorDecoratorTreeStart);
}

int StaticSolidArchiveLoadSession::
DecodePackFilePayloadWithStreamFeedback(
        const CPlugFilePack &pack,
        const CPlugFileFidContainer_SFileDesc &file,
        u32 descriptorIndex,
        u32 loaderArgument,
        const char *plainPath,
        const char *selectedDescriptorPath,
        CGameCtnReplayStaticSolidDecodedPayload *decodedOut,
        CGameCtnReplayStaticSolidArchiveDecodeStats *statsOut) {
    if (decodedOut == nullptr) {
        return 0;
    }
    StaticSolidArchivePayload payloadAsset;
    if (!payloadAsset.BuildEncryptedPackFilePayload({
                file,
                pack.dataStart,
                descriptorIndex,
                loaderArgument,
                plainPath,
                selectedDescriptorPath})) {
        return 0;
    }
    if ((size_t)payloadAsset.PackPayloadOffset() > pack.bytes.Size() ||
        payloadAsset.RawByteCount() >
                pack.bytes.Size() - (size_t)payloadAsset.PackPayloadOffset()) {
        return 0;
    }

    const StaticSolidArchiveId payload =
            StaticSolidArchiveId::FromIndex(
                    static_cast<u32>(payloads.size()));
    if (!AddPayloadAsset(payloadAsset)) {
        return 0;
    }
    return CGameCtnReplayStaticSolidArchivePayloadReader::DecodeWithStreamFeedback(
            pack.key,
            payloadAsset,
            StaticSolidArchiveRawBytes::FromBytes(
                    pack.bytes.Data() + payloadAsset.PackPayloadOffset(),
                    payloadAsset.RawByteCount()),
            this,
            nullptr,
            payload,
            decodedOut,
            statsOut);
}
