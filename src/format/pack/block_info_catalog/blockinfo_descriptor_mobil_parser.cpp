#include "engine/game/game_ctn_block_info.h"
#include "format/pack/block_info_catalog/blockinfo_descriptor_mobil_parser.h"
#include <stdint.h>
#include "format/pack/block_info_catalog/blockinfo_archive_stream.h"
#include "format/pack/block_info_catalog/blockinfo_descriptor_external_refs.h"
#include "format/pack/block_info_catalog/blockinfo_descriptor_unit_geometry.h"
#include "format/pack/installed/byte_buffer.h"
#include "format/archive/classic_archive_chunk_info.h"
#include "format/pack/installed/plug_file_pack.h"
#include "format/archive/scene_object_archive_chunk_ids.h"
#include "format/archive/scene_object_link_archive_chunk_ids.h"
#include "format/pack/installed/scene_descriptor_folder_paths.h"
#include "format/archive/tmnf_archive_ids.h"
#include "format/static_solid/static_scene_archive_loader.h"
#include "format/static_solid/static_solid_archive_payload_decode.h"
#include <utility>

#include "format/archive/archive_binary.h"
#include <new>
using TmnfFormat::ArchiveBinary::ReadU32LE;

static int BlockInfoDescriptorStringEquals(const char *lhs,
                                           const char *rhs) {
    if (lhs == nullptr || rhs == nullptr) {
        return lhs == rhs;
    }
    size_t i = 0u;
    for (;;) {
        if (lhs[i] != rhs[i]) {
            return 0;
        }
        if (lhs[i] == '\0') {
            return 1;
        }
        i++;
    }
}

static void BlockInfoDescriptorCopyIso4(float destination[12],
                                        const float source[12]) {
    for (u32 i = 0u; i < 12u; i++) {
        destination[i] = source[i];
    }
}

static int assign_cstr_to_string(std::string *dst, const char *src) {
    if (dst == nullptr || src == nullptr) {
        return 0;
    }
    try {
        *dst = src;
    } catch (const std::bad_alloc &) {
        return 0;
    }
    return 1;
}

enum BlockInfoDescriptorNodRefPurpose {
    BlockInfoDescriptorNodRefPurpose_Generic = 0,
    BlockInfoDescriptorNodRefPurpose_BlockMobil = 1,
    BlockInfoDescriptorNodRefPurpose_HmsSolid = 2,
    BlockInfoDescriptorNodRefPurpose_PylonMobil = 3
};

static int blockinfo_descriptor_append_model_ref(
        const char *descriptorPath,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        const BlockInfoDescriptorExternalRef *ref,
        const BlockInfoParsedHmsState *hmsState,
        BlockInfoParsedMobilCollection *models) {
    if (descriptorPath == nullptr || externalRefs == nullptr || models == nullptr ||
        ref == nullptr || !ref->HasName()) {
        return 1;
    }
    BlockInfoParsedMobil model{};
    model.selectorGroup = selectorGroup;
    model.variantIndex = variantIndex;
    model.mobilIndex = mobilIndex;
    model.loadable = ref->IsLoadable();
    if (hmsState != nullptr && hmsState->hasItemState != 0u) {
        model.hasHmsItemState = 1u;
        model.hmsItemPhysicsFlags = hmsState->physicsFlags;
        model.hmsItemRenderingFlags = hmsState->renderingFlags;
        model.hmsItemVisibilityFlags = hmsState->visibilityFlags;
        model.isRaceTriggerMobil = hmsState->isRaceTriggerMobil;
    }
    char plainPackPath[CGameCtnReplayStaticSelectedPathCapacity];
    char hashedDescriptorPath[CGameCtnReplayStaticSelectedPathCapacity];
    plainPackPath[0] = '\0';
    hashedDescriptorPath[0] = '\0';
    if (!externalRefs->OptionalPlainPathForReference(ref,
                                                     plainPackPath,
                                                     sizeof(plainPackPath))) {
        return 0;
    }
    if (plainPackPath[0] != '\0' &&
        SceneDescriptorFolderPaths::IsMediaSolidPath(plainPackPath) &&
        !SceneDescriptorFolderPaths::HashMediaSolidPath(
                plainPackPath,
                hashedDescriptorPath,
                sizeof(hashedDescriptorPath))) {
        return 0;
    }
    return assign_cstr_to_string(&model.descriptorPath, descriptorPath) &&
           assign_cstr_to_string(&model.modelName, ref->FileName()) &&
           assign_cstr_to_string(&model.plainPackPath, plainPackPath) &&
           assign_cstr_to_string(&model.hashedDescriptorPath, hashedDescriptorPath) &&
           models->Append(model);
}

static int blockinfo_descriptor_parse_scene_mobil_archive(
        BlockInfoSizeParseStream *stream,
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        BlockInfoParsedMobilCollection *models);

static int blockinfo_descriptor_extract_mobil_bytes(
        const CPlugFilePack *pack,
        const char *plainMobilPath,
        const char *selectedMobilPath,
        ByteBuffer *bytesOut) {
    if (pack == nullptr || plainMobilPath == nullptr ||
        selectedMobilPath == nullptr || bytesOut == nullptr) {
        return 0;
    }
    const CPlugFileFidContainer_SFileDesc *file =
            pack->FindFileDescByPath(selectedMobilPath);
    if (file != nullptr && file->IsEncryptedPayload()) {
        const u32 fileIndex = static_cast<u32>(file - pack->files.data());
        StaticSolidArchiveLoadSession decodeSession;
        CGameCtnReplayStaticSolidDecodedPayload decoded;
        CGameCtnReplayStaticSolidArchiveDecodeStats stats;
        return decodeSession.DecodePackFilePayloadWithStreamFeedback(
                       *pack,
                       *file,
                       fileIndex,
                       fileIndex + 1u,
                       plainMobilPath,
                       selectedMobilPath,
                       &decoded,
                       &stats) &&
               decoded.CopyToByteBuffer(bytesOut);
    }
    return pack->ExtractPath(selectedMobilPath, bytesOut);
}

static int blockinfo_descriptor_pack_payload_is_complete(
        const CPlugFilePack *pack,
        const CPlugFileFidContainer_SFileDesc *file) {
    if (pack == nullptr || file == nullptr ||
        pack->dataStart > UINT32_MAX - file->offsetRel) {
        return 0;
    }
    const size_t payloadOffset =
            static_cast<size_t>(pack->dataStart) + file->offsetRel;
    if (payloadOffset > pack->bytes.Size()) {
        return 0;
    }
    const u32 packedByteCount = file->PackedPayloadByteCount();
    size_t storedByteCount = packedByteCount;
    if (file->IsEncryptedPayload()) {
        if (packedByteCount > UINT32_MAX - 7u) {
            return 0;
        }
        const size_t encryptedByteCount =
                (static_cast<size_t>(packedByteCount) + 7u) & ~size_t{7u};
        if (encryptedByteCount > UINT32_MAX - 8u) {
            return 0;
        }
        storedByteCount = 8u + encryptedByteCount;
    }
    return storedByteCount <= pack->bytes.Size() - payloadOffset;
}

static int blockinfo_descriptor_parse_external_mobil_solid_refs(
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        const BlockInfoDescriptorExternalRef *mobilRef,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        const BlockInfoParsedHmsState *hmsState,
        BlockInfoParsedMobilCollection *models,
        u32 requiredDescriptorClassId = 0u) {
    if (descriptorPath == nullptr || externalRefs == nullptr ||
        mobilRef == nullptr || !mobilRef->HasName() ||
        externalRefs->InstalledPack() == nullptr || models == nullptr) {
        return requiredDescriptorClassId == 0u;
    }
    char plainMobilPath[512];
    char hashedMobilPath[512];
    plainMobilPath[0] = '\0';
    hashedMobilPath[0] = '\0';
    if (!externalRefs->PlainPathForReference(mobilRef,
                                             plainMobilPath,
                                             sizeof(plainMobilPath))) {
        return 0;
    }
    if (SceneDescriptorFolderPaths::IsMediaSolidPath(plainMobilPath)) {
        return requiredDescriptorClassId == 0u &&
               blockinfo_descriptor_append_model_ref(descriptorPath,
                                                      selectorGroup,
                                                      variantIndex,
                                                      mobilIndex,
                                                      externalRefs,
                                                      mobilRef,
                                                      hmsState,
                                                      models);
    }
    if (!SceneDescriptorFolderPaths::IsMobilDescriptorPath(plainMobilPath)) {
        return requiredDescriptorClassId == 0u;
    }
    if (!SceneDescriptorFolderPaths::HashMobilDescriptorPath(
                plainMobilPath, hashedMobilPath, sizeof(hashedMobilPath))) {
        return 0;
    }
    const CPlugFilePack *pack = externalRefs->InstalledPack();
    const CPlugFileFidContainer_SFileDesc *file =
            pack->FindFileDescByPath(hashedMobilPath);
    ByteBuffer mobilBytes{};
    if (!blockinfo_descriptor_extract_mobil_bytes(
                pack,
                plainMobilPath,
                hashedMobilPath,
                &mobilBytes)) {
        // The game does not instantiate a non-loadable external mobil. Still
        // reject a selected pack entry whose declared payload is truncated.
        return mobilRef->IsLoadable() == 0u &&
               (file == nullptr ||
                blockinfo_descriptor_pack_payload_is_complete(pack, file));
    }
    if (mobilBytes.Empty() || mobilBytes.Size() > UINT32_MAX) {
        return 0;
    }
    BlockInfoDescriptorExternalRefs mobilExternalRefs;
    if (!mobilExternalRefs.ParseGbx(
                mobilBytes.Data(), static_cast<u32>(mobilBytes.Size())) ||
        (requiredDescriptorClassId != 0u &&
         mobilExternalRefs.ClassId() != requiredDescriptorClassId)) {
        return 0;
    }
    if (!mobilExternalRefs.AttachInstalledPackRelativeTo(
                externalRefs->InstalledPack(), plainMobilPath)) {
        return 0;
    }
    BlockInfoSizeParseStream mobilStream{};
    mobilStream.bytes = mobilBytes.Data();
    mobilStream.byteCount = static_cast<u32>(mobilBytes.Size());
    mobilStream.offset = mobilExternalRefs.BodyOffsetForFormatParser();
    mobilStream.SetBlockInfoSourceRefs(&mobilExternalRefs);
    /*
     * Mobil descriptors may reference several Stadium\Media\Solid assets. Only
     * the solid nested under the mobil's CHmsItem contributes to collision;
     * other reachable solid references are not materialized here.
     */
    if (!blockinfo_descriptor_parse_scene_mobil_archive(&mobilStream,
                                                        descriptorPath,
                                                        &mobilExternalRefs,
                                                        selectorGroup,
                                                        variantIndex,
                                                        mobilIndex,
                                                        models)) {
        return 0;
    }
    return 1;
}

static int blockinfo_descriptor_parse_scene_link_ref_before_iso(
        BlockInfoSizeParseStream *stream,
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        int requireFacadeBoundary,
        int parseLinkedMobil,
        BlockInfoParsedMobilCollection *models) {
    ArchiveNodeReference linkedNode =
            ArchiveNodeReference::Invalid();
    if (!stream->ReadArchiveNodeRef(&linkedNode)) {
        return 0;
    }
    if (linkedNode.IsInvalid()) {
        return 1;
    }
    const u32 firstParsedRow = models != nullptr ? models->Count() : 0u;
    if (parseLinkedMobil != 0) {
        const BlockInfoDescriptorExternalRef *externalRef =
                externalRefs->FindReference(linkedNode);
        if (externalRef != nullptr &&
            !blockinfo_descriptor_parse_external_mobil_solid_refs(
                    descriptorPath,
                    externalRefs,
                    externalRef,
                    selectorGroup,
                    variantIndex,
                    mobilIndex,
                    nullptr,
                    models)) {
            return 0;
        }
        if (models != nullptr) {
            models->MarkAppendedSceneObjectLink(firstParsedRow);
        }
    }
    // A scene-object link stores its object reference before a transform and
    // enabled flag. Inline object data ends at the next complete link record.
    const u32 searchStart = stream->offset;
    if (stream->byteCount < 56u || searchStart > stream->byteCount - 56u) {
        return 0;
    }
    for (u32 cursor = searchStart;
         cursor <= stream->byteCount - 56u;
         cursor++) {
        if (requireFacadeBoundary &&
            cursor != searchStart &&
            ReadU32LE(stream->bytes + cursor - 4u) != CMwNodArchiveFacadeSentinel) {
            continue;
        }
        const u32 nextChunk =
                ReadU32LE(stream->bytes + cursor + 52u);
        if (BlockInfoSizeParseStream::WordIsSceneObjectLinkChunk(nextChunk)) {
            stream->offset = cursor;
            float linkIso[12];
            BlockInfoSizeParseStream isoStream = *stream;
            if (isoStream.ReadIso4(linkIso)) {
                if (models != nullptr) {
                    models->ApplySceneLinkIso4ToAppendedRaceTriggerModels(
                            firstParsedRow,
                            linkIso);
                }
            }
            return 1;
        }
    }
    return 0;
}

static int blockinfo_descriptor_parse_scene_link_inline_mobil_before_iso(
        BlockInfoSizeParseStream *stream,
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        BlockInfoParsedMobilCollection *models) {
    const u32 firstParsedRow = models != nullptr ? models->Count() : 0u;
    /*
     * Scene-object link version 0 contains a presence flag, model node
     * reference, inline mobil data, transform, and enabled flag. No child or
     * object collection appears between the inline mobil id and the transform.
     */
    ArchiveNodeReference modelNode =
            ArchiveNodeReference::Invalid();
    if (!stream->ReadArchiveNodeRef(&modelNode)) {
        return 0;
    }
    const BlockInfoDescriptorExternalRef *modelRef =
            externalRefs->FindReference(modelNode);
    if (modelRef != nullptr &&
        !blockinfo_descriptor_parse_external_mobil_solid_refs(descriptorPath,
                                                              externalRefs,
                                                              modelRef,
                                                              selectorGroup,
                                                              variantIndex,
                                                              mobilIndex,
                                                              nullptr,
                                                              models)) {
        return 0;
    }
    u32 inlineMobilVersion = 0u;
    if (!stream->ReadU32(&inlineMobilVersion)) {
        return 0;
    }
    if (inlineMobilVersion != 2u) {
        return 0;
    }
    for (;;) {
        u32 wrappedChunkId = 0u;
        if (!stream->ReadU32(&wrappedChunkId)) {
            return 0;
        }
        if (wrappedChunkId == 0xffffffffu) {
            break;
        }
        return 0;
    }
    char sceneMobilObjectId[64];
    if (!stream->ReadCMwIdText(sceneMobilObjectId,
                              sizeof(sceneMobilObjectId))) {
        return 0;
    }
    if (models != nullptr) {
        models->MarkAppendedSceneObjectLink(firstParsedRow);
    }
    if (sceneMobilObjectId[0] != '\0' &&
        (BlockInfoDescriptorStringEquals(sceneMobilObjectId, "TriggerCheckpoint") ||
         BlockInfoDescriptorStringEquals(sceneMobilObjectId, "TriggerFinishLine"))) {
        if (models != nullptr) {
            models->MarkAppendedRaceTrigger(firstParsedRow);
        }
    }
    const u32 searchStart = stream->offset;
    if (stream->byteCount < 56u || searchStart > stream->byteCount - 56u) {
        return 0;
    }
    for (u32 cursor = searchStart;
         cursor <= stream->byteCount - 56u;
         cursor++) {
        const u32 nextChunk =
                ReadU32LE(stream->bytes + cursor + 52u);
        if (BlockInfoSizeParseStream::WordIsSceneObjectLinkChunk(nextChunk)) {
            stream->offset = cursor;
            float linkIso[12];
            BlockInfoSizeParseStream isoStream = *stream;
            if (isoStream.ReadIso4(linkIso)) {
                if (models != nullptr) {
                    models->ApplySceneLinkIso4ToAppendedRaceTriggerModels(
                            firstParsedRow,
                            linkIso);
                }
            }
            return 1;
        }
    }
    return 0;
}

static int blockinfo_descriptor_parse_scene_object_link_archive(
        BlockInfoSizeParseStream *stream,
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        BlockInfoParsedMobilCollection *models) {
    for (;;) {
        u32 chunkId = 0u;
        if (!stream->ReadU32(&chunkId)) {
            return 0;
        }
        if (chunkId == CMwNodArchiveFacadeSentinel) {
            return 1;
        }
        switch (chunkId) {
        case ArchiveChunkIdValue(CSceneObjectLinkArchiveChunkId::Root):
        case ArchiveChunkIdValue(CSceneObjectLinkArchiveChunkId::LegacyRoot):
            if (!blockinfo_descriptor_parse_scene_link_ref_before_iso(
                        stream,
                        descriptorPath,
                        externalRefs,
                        selectorGroup,
                        variantIndex,
                        mobilIndex,
                        1,
                        1,
                        models) ||
                !stream->Skip(48u + 4u)) {
                return 0;
            }
            break;
        case ArchiveChunkIdValue(
                CSceneObjectLinkArchiveChunkId::MobilOrObjectRefBeforeIso): {
            u32 usesMobilPtr = 0u;
            if (!stream->ReadU32(&usesMobilPtr)) {
                return 0;
            }
            if (usesMobilPtr != 0u) {
                if (!blockinfo_descriptor_parse_scene_link_inline_mobil_before_iso(
                            stream,
                            descriptorPath,
                            externalRefs,
                            selectorGroup,
                            variantIndex,
                            mobilIndex,
                            models) ||
                    !stream->Skip(48u + 4u)) {
                    return 0;
                }
                break;
            }
            if (!blockinfo_descriptor_parse_scene_link_ref_before_iso(
                        stream,
                        descriptorPath,
                        externalRefs,
                        selectorGroup,
                        variantIndex,
                        mobilIndex,
                        1,
                        1,
                        models) ||
                !stream->Skip(48u + 4u)) {
                return 0;
            }
            break;
        }
        case ArchiveChunkIdValue(
                CSceneObjectLinkArchiveChunkId::LegacyPositionAndActiveFlag):
            if (!stream->Skip(6u * 4u + 4u)) {
                return 0;
            }
            break;
        case ArchiveChunkIdValue(CSceneObjectLinkArchiveChunkId::LegacyActiveFlag):
            if (!stream->Skip(4u)) {
                return 0;
            }
            break;
        case ArchiveChunkIdValue(
                CSceneObjectLinkArchiveChunkId::MobilTreeIdAndInstallFlags):
            if (!stream->SkipCMwId() ||
                !stream->Skip(8u)) {
                return 0;
            }
            break;
        case ArchiveChunkIdValue(CSceneObjectLinkArchiveChunkId::ArchivedTreeId):
            if (!stream->SkipCMwId()) {
                return 0;
            }
            break;
        default:
            return 0;
        }
    }
}

static int blockinfo_descriptor_parse_counted_scene_object_links(
        BlockInfoSizeParseStream *stream,
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        BlockInfoParsedMobilCollection *models) {
    u32 count = 0u;
    if (!stream->ReadU32(&count) ||
        count > 0x100000u) {
        return 0;
    }
    for (u32 i = 0u; i < count; i++) {
        ArchiveNodeReference sceneObjectNode =
                ArchiveNodeReference::Invalid();
        if (!stream->ReadArchiveNodeRef(&sceneObjectNode)) {
            return 0;
        }
        if (sceneObjectNode.IsInvalid()) {
            continue;
        }
        if (stream->offset <= stream->byteCount - 4u &&
            ReadU32LE(stream->bytes + stream->offset) ==
                    ArchiveChunkIdValue(CSceneObjectLinkArchiveChunkId::Root)) {
            u32 classId = 0u;
            if (!stream->ReadU32(&classId) ||
                classId != ArchiveChunkIdValue(CSceneObjectLinkArchiveChunkId::Root) ||
                !blockinfo_descriptor_parse_scene_object_link_archive(
                        stream,
                        descriptorPath,
                        externalRefs,
                        selectorGroup,
                        variantIndex,
                        mobilIndex,
                        models)) {
                return 0;
            }
        }
    }
    return 1;
}

static int blockinfo_descriptor_skip_or_parse_nod_ref(
        BlockInfoSizeParseStream *stream,
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        const BlockInfoParsedHmsState *hmsState,
        BlockInfoDescriptorNodRefPurpose purpose,
        BlockInfoParsedMobilCollection *models);

static int blockinfo_descriptor_skip_or_parse_nod_ref_value(
        BlockInfoSizeParseStream *stream,
        ArchiveNodeReference sourceNode,
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        const BlockInfoParsedHmsState *hmsState,
        BlockInfoDescriptorNodRefPurpose purpose,
        BlockInfoParsedMobilCollection *models,
        int *motionChangesLocationsOut = nullptr,
        int *collisionUsesInitialTransformOut = nullptr);

static int blockinfo_descriptor_is_solid_archive_boundary_word(u32 rawChunkId) {
    const u32 chunkId = TMNF_CPlugSolidCanonicalChunkIdFromArchiveWord(rawChunkId);
    return chunkId == CMwNodArchiveFacadeSentinel ||
           chunkId == 0xffffffffu ||
           chunkId == 0x09007161u ||
           (chunkId >= TMNF_CLASS_CPlugSolid && chunkId <= 0x09005012u);
}

static int blockinfo_descriptor_solid_boundary_payload_fits(
        const BlockInfoSizeParseStream *stream,
        u32 boundaryOffset,
        u32 rawChunkId) {
    if (stream == nullptr || boundaryOffset > stream->byteCount ||
        stream->byteCount - boundaryOffset < 4u) {
        return 0;
    }
    const u32 chunkId = TMNF_CPlugSolidCanonicalChunkIdFromArchiveWord(rawChunkId);
    u32 minPayloadBytes = 0u;
    switch (chunkId) {
    case CMwNodArchiveFacadeSentinel:
    case 0xffffffffu:
        minPayloadBytes = 0u;
        break;
    case 0x09007161u:
    case TMNF_CLASS_CPlugSolid:
    case 0x09005010u:
        minPayloadBytes = 4u;
        break;
    case 0x0900500eu:
        minPayloadBytes = 16u * 4u;
        break;
    case 0x0900500fu:
        minPayloadBytes = 2u * 4u;
        break;
    case 0x09005011u:
        minPayloadBytes = 2u * 4u;
        break;
    case 0x09005012u:
        minPayloadBytes = 1u;
        break;
    default:
        minPayloadBytes = 0u;
        break;
    }
    return stream->byteCount - boundaryOffset >= 4u + minPayloadBytes;
}

static int blockinfo_descriptor_try_parse_mask_dispatched_solid_model_body(
        BlockInfoSizeParseStream *stream,
        u32 boundaryOffset,
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        const BlockInfoParsedHmsState *hmsState,
        BlockInfoParsedMobilCollection *models,
        int *matchedOut) {
    if (matchedOut != nullptr) {
        *matchedOut = 0;
    }
    if (stream == nullptr || externalRefs == nullptr || models == nullptr ||
        boundaryOffset < stream->offset + 12u ||
        boundaryOffset > stream->byteCount) {
        return 1;
    }
    /*
     * Readable chunk selector 0x0900500d may expose its payload without an
     * inline four-byte chunk header. Recognize only the model form: use-model
     * flag, has-model flag, and optional model node reference. Collision-tree
     * forms remain with the normal parser because their references are
     * variable-length.
     */
    const u32 payloadOffset = boundaryOffset - 12u;
    const u32 useModel = ReadU32LE(stream->bytes + payloadOffset);
    const u32 hasModel = ReadU32LE(stream->bytes + payloadOffset + 4u);
    const ArchiveNodeReference modelNode =
            ArchiveNodeReference::FromIndex(
                    ReadU32LE(stream->bytes + payloadOffset + 8u));
    if (useModel != 1u || hasModel != 1u) {
        return 1;
    }
    const BlockInfoDescriptorExternalRef *modelRef =
            externalRefs->FindReference(modelNode);
    if (modelRef == nullptr || !modelRef->HasName()) {
        return 1;
    }
    char plainPath[CGameCtnReplayStaticSelectedPathCapacity];
    if (!externalRefs->PlainPathForReference(modelRef, plainPath,
                                                      sizeof(plainPath))) {
        return 0;
    }
    if (!SceneDescriptorFolderPaths::IsMediaSolidPath(plainPath)) {
        return 1;
    }
    if (!blockinfo_descriptor_append_model_ref(descriptorPath,
                                               selectorGroup,
                                               variantIndex,
                                               mobilIndex,
                                               externalRefs,
                                               modelRef,
                                               hmsState,
                                               models)) {
        return 0;
    }
    if (matchedOut != nullptr) {
        *matchedOut = 1;
    }
    return 1;
}

static int blockinfo_descriptor_consume_encoded_nod_ref_tail_to_solid_boundary(
        BlockInfoSizeParseStream *stream,
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        const BlockInfoParsedHmsState *hmsState,
        BlockInfoParsedMobilCollection *models) {
    /*
     * Chunk 0x09005010 contains a variable-length reference-buffer node. Its
     * packed form may continue after the first natural instead of embedding a
     * reference-buffer node body, so advance to the next complete solid chunk
     * boundary.
     */
    if (stream == nullptr ||
        stream->offset > stream->byteCount ||
        stream->byteCount - stream->offset < 4u ||
        (blockinfo_descriptor_is_solid_archive_boundary_word(
                 ReadU32LE(stream->bytes + stream->offset)) &&
         blockinfo_descriptor_solid_boundary_payload_fits(
                 stream,
                 stream->offset,
                 ReadU32LE(stream->bytes + stream->offset)))) {
        return 1;
    }
    const u32 start = stream->offset;
    for (u32 delta = 1u;
         delta <= 64u &&
         start <= stream->byteCount &&
         stream->byteCount - start >= delta + 4u;
         delta++) {
        const u32 candidate =
                ReadU32LE(stream->bytes + start + delta);
        if (blockinfo_descriptor_is_solid_archive_boundary_word(candidate) &&
            blockinfo_descriptor_solid_boundary_payload_fits(
                    stream,
                    start + delta,
                    candidate)) {
            int matchedMaskBody = 0;
            if (!blockinfo_descriptor_try_parse_mask_dispatched_solid_model_body(
                        stream,
                        start + delta,
                        descriptorPath,
                        externalRefs,
                        selectorGroup,
                        variantIndex,
                        mobilIndex,
                        hmsState,
                        models,
                        &matchedMaskBody)) {
                return 0;
            }
            stream->offset = start + delta;
            return 1;
        }
    }
    return 1;
}

struct BlockInfoSolidParseContext {
    BlockInfoSizeParseStream *stream = nullptr;
    const char *descriptorPath = nullptr;
    const BlockInfoDescriptorExternalRefs *externalRefs = nullptr;
    u32 selectorGroup = 0u;
    u32 variantIndex = 0u;
    u32 mobilIndex = 0u;
    const BlockInfoParsedHmsState *hmsState = nullptr;
    BlockInfoParsedMobilCollection *models = nullptr;
};

static int blockinfo_descriptor_parse_solid_ref(
        const BlockInfoSolidParseContext &context,
        BlockInfoDescriptorNodRefPurpose purpose) {
    return blockinfo_descriptor_skip_or_parse_nod_ref(
            context.stream,
            context.descriptorPath,
            context.externalRefs,
            context.selectorGroup,
            context.variantIndex,
            context.mobilIndex,
            context.hmsState,
            purpose,
            context.models);
}

static int blockinfo_descriptor_consume_solid_ref_tail(
        const BlockInfoSolidParseContext &context) {
    return blockinfo_descriptor_consume_encoded_nod_ref_tail_to_solid_boundary(
            context.stream,
            context.descriptorPath,
            context.externalRefs,
            context.selectorGroup,
            context.variantIndex,
            context.mobilIndex,
            context.hmsState,
            context.models);
}

static int blockinfo_descriptor_append_solid_model(
        const BlockInfoSolidParseContext &context,
        const BlockInfoDescriptorExternalRef *modelRef) {
    return blockinfo_descriptor_append_model_ref(
            context.descriptorPath,
            context.selectorGroup,
            context.variantIndex,
            context.mobilIndex,
            context.externalRefs,
            modelRef,
            context.hmsState,
            context.models);
}

enum class BlockInfoSolidChunkResult {
    Continue,
    Complete,
    Error,
};

static BlockInfoSolidChunkResult blockinfo_descriptor_parse_solid_model_chunk(
        const BlockInfoSolidParseContext &context,
        bool supportsExternalFid) {
    u32 useModel = 0u;
    u32 hasModel = 0u;
    if (!context.stream->ReadU32(&useModel) ||
        !context.stream->ReadU32(&hasModel)) {
        return BlockInfoSolidChunkResult::Error;
    }
    if (hasModel != 0u) {
        if (!supportsExternalFid) {
            if (!blockinfo_descriptor_parse_solid_ref(
                        context, BlockInfoDescriptorNodRefPurpose_HmsSolid)) {
                return BlockInfoSolidChunkResult::Error;
            }
        } else {
            u32 modelIsFid = 0u;
            if (!context.stream->ReadU32(&modelIsFid)) {
                return BlockInfoSolidChunkResult::Error;
            }
            if (modelIsFid != 0u) {
                ArchiveNodeReference modelNode = ArchiveNodeReference::Invalid();
                if (!context.stream->ReadArchiveNodeRef(&modelNode) ||
                    !blockinfo_descriptor_append_solid_model(
                            context,
                            context.externalRefs->FindReference(modelNode))) {
                    return BlockInfoSolidChunkResult::Error;
                }
            } else if (!blockinfo_descriptor_parse_solid_ref(
                               context,
                               BlockInfoDescriptorNodRefPurpose_Generic)) {
                return BlockInfoSolidChunkResult::Error;
            }
        }
    }
    if ((useModel == 0u || hasModel == 0u) &&
        !blockinfo_descriptor_parse_solid_ref(
                context, BlockInfoDescriptorNodRefPurpose_Generic)) {
        return BlockInfoSolidChunkResult::Error;
    }
    return BlockInfoSolidChunkResult::Continue;
}

static BlockInfoSolidChunkResult blockinfo_descriptor_parse_solid_chunk(
        const BlockInfoSolidParseContext &context,
        u32 chunkId) {
    switch (chunkId) {
    case 0xffffffffu:
        return BlockInfoSolidChunkResult::Continue;
    case TMNF_CLASS_CPlugSolid:
        return context.stream->Skip(4u)
                ? BlockInfoSolidChunkResult::Continue
                : BlockInfoSolidChunkResult::Error;
    case 0x0900500eu:
        return context.stream->Skip(16u * 4u) &&
                       blockinfo_descriptor_consume_solid_ref_tail(context)
                ? BlockInfoSolidChunkResult::Continue
                : BlockInfoSolidChunkResult::Error;
    case 0x0900500fu:
        return context.stream->Skip(2u * 4u)
                ? BlockInfoSolidChunkResult::Continue
                : BlockInfoSolidChunkResult::Error;
    case 0x09005010u:
        return blockinfo_descriptor_parse_solid_ref(
                       context, BlockInfoDescriptorNodRefPurpose_Generic) &&
                       blockinfo_descriptor_consume_solid_ref_tail(context)
                ? BlockInfoSolidChunkResult::Continue
                : BlockInfoSolidChunkResult::Error;
    case 0x0900500du:
        return blockinfo_descriptor_parse_solid_model_chunk(context, false);
    case 0x09005011u:
        return blockinfo_descriptor_parse_solid_model_chunk(context, true);
    case 0x09005012u:
        return context.stream->Skip(1u)
                ? BlockInfoSolidChunkResult::Continue
                : BlockInfoSolidChunkResult::Error;
    case 0x09007161u:
        return context.stream->Skip(4u)
                ? BlockInfoSolidChunkResult::Continue
                : BlockInfoSolidChunkResult::Error;
    default:
        u32 skipMagic = 0u;
        if (!context.stream->ReadU32(&skipMagic)) {
            return BlockInfoSolidChunkResult::Complete;
        }
        if (skipMagic != 0x534b4950u) {
            return BlockInfoSolidChunkResult::Complete;
        }
        u32 skipByteCount = 0u;
        return context.stream->ReadU32(&skipByteCount) &&
                       context.stream->Skip(skipByteCount)
                ? BlockInfoSolidChunkResult::Continue
                : BlockInfoSolidChunkResult::Error;
    }
}

static int blockinfo_descriptor_parse_solid_archive(
        const BlockInfoSolidParseContext &context) {
    for (;;) {
        u32 rawChunkId = 0u;
        if (!context.stream->ReadU32(&rawChunkId)) {
            return 0;
        }
        const u32 chunkId =
                TMNF_CPlugSolidCanonicalChunkIdFromArchiveWord(rawChunkId);
        if (chunkId == CMwNodArchiveFacadeSentinel) {
            return 1;
        }
        switch (blockinfo_descriptor_parse_solid_chunk(context, chunkId)) {
        case BlockInfoSolidChunkResult::Continue:
            break;
        case BlockInfoSolidChunkResult::Complete:
            return 1;
        case BlockInfoSolidChunkResult::Error:
            return 0;
        }
    }
}

static int blockinfo_descriptor_parse_hms_item_archive(
        BlockInfoSizeParseStream *stream,
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        const char *sceneMobilObjectId,
        BlockInfoParsedMobilCollection *models) {
    BlockInfoParsedHmsState hmsState{};
    hmsState.isRaceTriggerMobil =
            ((sceneMobilObjectId) != nullptr &&
           (BlockInfoDescriptorStringEquals((sceneMobilObjectId), "TriggerCheckpoint") ||
            BlockInfoDescriptorStringEquals((sceneMobilObjectId), "TriggerFinishLine")));
    for (;;) {
        u32 chunkId = 0u;
        if (!stream->ReadU32(&chunkId)) {
            return 0;
        }
        if (chunkId == CMwNodArchiveFacadeSentinel) {
            return 1;
        }
        switch (chunkId) {
        case 0x06003001u:
            if (!blockinfo_descriptor_skip_or_parse_nod_ref(stream,
                                                            descriptorPath,
                                                            externalRefs,
                                                            selectorGroup,
                                                            variantIndex,
                                                            mobilIndex,
                                                            &hmsState,
                                                            BlockInfoDescriptorNodRefPurpose_HmsSolid,
                                                            models)) return 0;
            break;
        case 0x06003011u:
            if (!stream->ReadU32(&hmsState.physicsFlags) ||
                !stream->ReadU32(&hmsState.renderingFlags) ||
                !stream->ReadU16(&hmsState.visibilityFlags)) return 0;
            hmsState.hasItemState = 1u;
            if (models != nullptr) {
                models->ApplyHmsState(descriptorPath,
                                      selectorGroup,
                                      variantIndex,
                                      mobilIndex,
                                      &hmsState);
            }
            break;
        default: {
            u32 skipMagic = 0u;
            if (!stream->ReadU32(&skipMagic)) {
                return 1;
            }
            if (skipMagic == 0x534b4950u) {
                u32 skipByteCount = 0u;
                if (!stream->ReadU32(&skipByteCount) ||
                    !stream->Skip(skipByteCount)) {
                    return 0;
                }
                break;
            }
            return 1;
        }
        }
    }
}

static int blockinfo_descriptor_parse_scene_mobil_archive(
        BlockInfoSizeParseStream *stream,
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        BlockInfoParsedMobilCollection *models) {
    char sceneMobilObjectId[64];
    sceneMobilObjectId[0] = '\0';
    int motionChangesLocations = 0;
    int collisionUsesInitialTransform = 0;
    const u32 firstMobil = models != nullptr ? models->Count() : 0u;
    const auto finish = [&]() {
        if (motionChangesLocations != 0u && models != nullptr) {
            models->MarkAppendedMotionChangesLocations(firstMobil);
        }
        if (collisionUsesInitialTransform != 0u && models != nullptr) {
            models->MarkAppendedCollisionUsesInitialTransform(firstMobil);
        }
        return 1;
    };
    for (;;) {
        if (stream->offset >= stream->byteCount) {
            return finish();
        }
        if (stream->byteCount - stream->offset == 4u) {
            const u32 tail = ReadU32LE(stream->bytes + stream->offset);
            if (tail != CMwNodArchiveFacadeSentinel &&
                !IsCSceneObjectDescriptorMobilTailChunk(tail)) {
                return finish();
            }
        }
        u32 rawChunkId = 0u;
        if (!stream->ReadU32(&rawChunkId)) {
            return 0;
        }
        const u32 chunkId =
                NormalizePackedCSceneObjectOrMobilArchiveChunkId(rawChunkId);
        if (chunkId == CMwNodArchiveFacadeSentinel) {
            return finish();
        }
        switch (chunkId) {
        case ArchiveChunkIdValue(CSceneObjectArchiveChunkId::Id):
            /*
             * This field is the scene-object id. Only TriggerCheckpoint and
             * TriggerFinishLine children participate in race-trigger selection.
             */
            if (!stream->ReadCMwIdText(sceneMobilObjectId,
                                                       sizeof(sceneMobilObjectId))) {
                return 0;
            }
            break;
        case ArchiveChunkIdValue(CSceneObjectArchiveChunkId::MotionRef): {
            ArchiveNodeReference motionNode = ArchiveNodeReference::Invalid();
            if (!stream->ReadArchiveNodeRef(&motionNode) ||
                !blockinfo_descriptor_skip_or_parse_nod_ref_value(
                        stream,
                        motionNode,
                        descriptorPath,
                        externalRefs,
                        selectorGroup,
                        variantIndex,
                        mobilIndex,
                        nullptr,
                BlockInfoDescriptorNodRefPurpose_Generic,
                models,
                &motionChangesLocations,
                &collisionUsesInitialTransform)) {
                return 0;
            }
            break;
        }
        case ArchiveChunkIdValue(CSceneMobilArchiveChunkId::MessageHandlerRef):
            if (!blockinfo_descriptor_skip_or_parse_nod_ref(stream,
                                                            descriptorPath,
                                                            externalRefs,
                                                            selectorGroup,
                                                            variantIndex,
                                                            mobilIndex,
                                                            nullptr,
                                                            BlockInfoDescriptorNodRefPurpose_Generic,
                                                            models)) return 0;
            break;
        case ArchiveChunkIdValue(CSceneMobilArchiveChunkId::ChildObjects):
            if (!blockinfo_descriptor_parse_counted_scene_object_links(
                        stream,
                        descriptorPath,
                        externalRefs,
                        selectorGroup,
                        variantIndex,
                        mobilIndex,
                        models)) return 0;
            break;
        case ArchiveChunkIdValue(CSceneMobilArchiveChunkId::HmsItemData):
            if (!blockinfo_descriptor_parse_hms_item_archive(stream,
                                                             descriptorPath,
                                                             externalRefs,
                                                             selectorGroup,
                                                             variantIndex,
                                                             mobilIndex,
                                                             sceneMobilObjectId,
                                                             models)) return 0;
            break;
        default: {
            // Unknown node records may carry a SKIP marker and byte count.
            // Otherwise the current node body ends at the consumed word.
            u32 skipMagic = 0u;
            if (!stream->ReadU32(&skipMagic)) {
                return finish();
            }
            if (skipMagic == 0x534b4950u) {
                u32 skipByteCount = 0u;
                if (!stream->ReadU32(&skipByteCount) ||
                    !stream->Skip(skipByteCount)) {
                    return 0;
                }
                break;
            }
            return finish();
        }
        }
    }
}

static int blockinfo_descriptor_skip_or_parse_nod_ref_value(
        BlockInfoSizeParseStream *stream,
        ArchiveNodeReference sourceNode,
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        const BlockInfoParsedHmsState *hmsState,
        BlockInfoDescriptorNodRefPurpose purpose,
        BlockInfoParsedMobilCollection *models,
        int *motionChangesLocationsOut,
        int *collisionUsesInitialTransformOut) {
    if (motionChangesLocationsOut != nullptr) {
        *motionChangesLocationsOut = 0;
    }
    if (collisionUsesInitialTransformOut != nullptr) {
        *collisionUsesInitialTransformOut = 0;
    }
    if (sourceNode.IsInvalid()) {
        return 1;
    }
    const BlockInfoDescriptorExternalRef *externalRef =
            externalRefs->FindReference(sourceNode);
    if (externalRef != nullptr) {
        if (purpose == BlockInfoDescriptorNodRefPurpose_HmsSolid) {
            return blockinfo_descriptor_append_model_ref(descriptorPath,
                                                        selectorGroup,
                                                        variantIndex,
                                                        mobilIndex,
                                                        externalRefs,
                                                        externalRef,
                                                        hmsState,
                                                        models);
        }
        if (purpose == BlockInfoDescriptorNodRefPurpose_BlockMobil) {
            return blockinfo_descriptor_parse_external_mobil_solid_refs(
                    descriptorPath,
                    externalRefs,
                    externalRef,
                    selectorGroup,
                    variantIndex,
                    mobilIndex,
                    hmsState,
                    models);
        }
        if (purpose == BlockInfoDescriptorNodRefPurpose_PylonMobil) {
            return blockinfo_descriptor_parse_external_mobil_solid_refs(
                    descriptorPath,
                    externalRefs,
                    externalRef,
                    selectorGroup,
                    variantIndex,
                    mobilIndex,
                    hmsState,
                    models,
                    TMNF_CLASS_CSceneMobil);
        }
        if (purpose != BlockInfoDescriptorNodRefPurpose_Generic ||
            stream->offset > stream->byteCount - 4u ||
            ReadU32LE(stream->bytes + stream->offset) !=
                    TMNF_CLASS_CMotionPlayer) {
            return 1;
        }
    }
    if (stream->offset > stream->byteCount ||
        stream->byteCount - stream->offset < sizeof(u32)) {
        return purpose != BlockInfoDescriptorNodRefPurpose_PylonMobil;
    }
    const u32 nextWord = ReadU32LE(stream->bytes + stream->offset);
    if (purpose == BlockInfoDescriptorNodRefPurpose_PylonMobil &&
        nextWord != TMNF_CLASS_CSceneMobil) {
        return 0;
    }
    if (nextWord != TMNF_CLASS_CSceneMobil &&
        nextWord != TMNF_CLASS_CPlugSolid &&
        nextWord != TMNF_CLASS_CGameCtnBlockUnitInfo &&
        nextWord != TMNF_CLASS_CMotions &&
        nextWord != TMNF_CLASS_CMotionPlayer &&
        nextWord != TMNF_CLASS_CMotionEmitterLeaves) {
        return 1;
    }
    u32 classId = 0u;
    if (!stream->ReadU32(&classId)) {
        return 0;
    }
    switch (classId) {
    case TMNF_CLASS_CSceneMobil:
        return blockinfo_descriptor_parse_scene_mobil_archive(stream,
                                                              descriptorPath,
                                                              externalRefs,
                                                              selectorGroup,
                                                              variantIndex,
                                                              mobilIndex,
                                                              models);
    case TMNF_CLASS_CPlugSolid:
        return blockinfo_descriptor_parse_solid_archive({
                stream,
                descriptorPath,
                externalRefs,
                selectorGroup,
                variantIndex,
                mobilIndex,
                hmsState,
                models,
        });
    case TMNF_CLASS_CGameCtnBlockUnitInfo: {
        u32 ignoredOffset[3] = {0u, 0u, 0u};
        return stream->ParseUnitNode(ignoredOffset,
                                                     nullptr);
    }
    case TMNF_CLASS_CMotions:
        return stream->ParseInlineCMotionsArchive(
                sourceNode,
                motionChangesLocationsOut,
                collisionUsesInitialTransformOut);
    case TMNF_CLASS_CMotionPlayer:
    case TMNF_CLASS_CMotionEmitterLeaves:
        return stream->ParseInlineMotionArchive(
                sourceNode,
                classId,
                motionChangesLocationsOut,
                collisionUsesInitialTransformOut);
    default:
        return 0;
    }
}

static int blockinfo_descriptor_skip_or_parse_nod_ref(
        BlockInfoSizeParseStream *stream,
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        u32 selectorGroup,
        u32 variantIndex,
        u32 mobilIndex,
        const BlockInfoParsedHmsState *hmsState,
        BlockInfoDescriptorNodRefPurpose purpose,
        BlockInfoParsedMobilCollection *models) {
    ArchiveNodeReference sourceNode =
            ArchiveNodeReference::Invalid();
    return stream != nullptr &&
           stream->ReadArchiveNodeRef(&sourceNode) &&
           blockinfo_descriptor_skip_or_parse_nod_ref_value(
                   stream,
                   sourceNode,
                   descriptorPath,
                   externalRefs,
                   selectorGroup,
                   variantIndex,
                   mobilIndex,
                   hmsState,
                   purpose,
                   models);
}

static int blockinfo_descriptor_parse_mobil_ref_array(
        BlockInfoSizeParseStream *stream,
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        u32 selectorGroup,
        BlockInfoParsedMobilCounts *counts,
        BlockInfoParsedMobilCollection *models,
        u32 *totalOut) {
    u32 variantCount = 0u;
    u32 total = 0u;
    if (!stream->ReadU32(&variantCount) ||
        variantCount > 0x10000u) {
        return 0;
    }
    for (u32 variant = 0u; variant < variantCount; variant++) {
        u32 mobilCount = 0u;
        if (!stream->ReadU32(&mobilCount) ||
            mobilCount > 0x10000u) {
            return 0;
        }
        if (variant < 64u &&
            !counts->SetVariantCount(selectorGroup, variant, mobilCount)) {
            return 0;
        }
        for (u32 mobil = 0u; mobil < mobilCount; mobil++) {
            ArchiveNodeReference sourceNode =
                    ArchiveNodeReference::Invalid();
            if (!stream->ReadArchiveNodeRef(&sourceNode)) {
                return 0;
            }
            if (!sourceNode.IsInvalid()) {
                if (!sourceNode.IsValid() ||
                    sourceNode.Index() >
                            externalRefs->NodeCountForFormatBounds()) {
                    return 0;
                }
                const BlockInfoDescriptorExternalRef *externalRef =
                        externalRefs->FindReference(sourceNode);
                const bool hasInlineSceneMobil =
                        externalRef == nullptr &&
                        stream->offset <= stream->byteCount &&
                        stream->byteCount - stream->offset >= 4u &&
                        ReadU32LE(stream->bytes + stream->offset) ==
                                TMNF_CLASS_CSceneMobil;
                if (externalRef == nullptr && variant < 64u &&
                    !models->RecordArchiveLocalRef(
                            sourceNode.Index(),
                            {selectorGroup, variant, mobil},
                            hasInlineSceneMobil)) {
                    return 0;
                }
            }
            if (!blockinfo_descriptor_skip_or_parse_nod_ref_value(
                        stream,
                        sourceNode,
                        descriptorPath,
                        externalRefs,
                        selectorGroup,
                        variant,
                        mobil,
                        nullptr,
                        BlockInfoDescriptorNodRefPurpose_BlockMobil,
                        models)) {
                return 0;
            }
            total++;
        }
    }
    if (totalOut != nullptr) {
        *totalOut = total;
    }
    return 1;
}

static int blockinfo_descriptor_parse_helper_source_ref(
        BlockInfoSizeParseStream *stream,
        const char *descriptorPath,
        const BlockInfoDescriptorExternalRefs *externalRefs,
        u32 selectorGroup,
        BlockInfoParsedMobilCollection *models,
        std::string *descriptorOut) {
    const u32 beforeCount = models != nullptr ? models->Count() : 0u;
    if (descriptorOut != nullptr) {
        descriptorOut->clear();
    }
    if (!blockinfo_descriptor_skip_or_parse_nod_ref(stream,
                                                    descriptorPath,
                                                    externalRefs,
                                                    selectorGroup,
                                                    0u,
                                                    0u,
                                                    nullptr,
                                                    BlockInfoDescriptorNodRefPurpose_BlockMobil,
                                                    models)) {
        return 0;
    }
    if (models == nullptr || descriptorOut == nullptr) {
        return 1;
    }
    for (u32 i = beforeCount; i < models->Count(); i++) {
        const BlockInfoParsedMobil *model = models->ModelAt(i);
        if (model == nullptr) {
            continue;
        }
        if (model->hashedDescriptorPath.empty()) {
            continue;
        }
        return assign_cstr_to_string(descriptorOut,
                                     model->hashedDescriptorPath.c_str());
    }
    return 1;
}

namespace {

enum class BlockInfoDescriptorChunkResult {
    Continue,
    Complete,
    Error,
};

struct BlockInfoDescriptorParseContext {
    BlockInfoSizeParseStream &stream;
    const char *descriptorPath;
    const BlockInfoDescriptorExternalRefs &externalRefs;
    BlockInfoDescriptorParseResult &descriptor;
    std::vector<BlockInfoDescriptorUnitDefinition> &groundUnitDefinitions;
    std::vector<BlockInfoDescriptorUnitDefinition> &airUnitDefinitions;
    BlockInfoParsedMobilCollection &models;
    bool continueAfterMainBlockInfoChunk;
    bool *wrongRootPylonChunkOut;
    bool parsedMobilArrays = false;
};

bool IsMainBlockInfoChunk(u32 chunkId) {
    return chunkId == TMNF_CLASS_CGameCtnBlockInfo ||
           chunkId == 0x0304e001u ||
           chunkId == 0x0304e004u ||
           chunkId == 0x0304e005u ||
           chunkId == 0x0304e008u;
}

BlockInfoDescriptorChunkResult ParseMainBlockInfoChunk(
        BlockInfoDescriptorParseContext &context,
        u32 chunkId) {
    u32 ignored = 0u;
    if (!context.stream.SkipCMwId() ||
        !context.stream.Skip(20u) ||
        !context.stream.ReadU32(&ignored) ||
        !context.stream.SkipNodRef() ||
        !context.stream.ReadUnitLayout(
                nullptr, &context.groundUnitDefinitions) ||
        !context.stream.ReadUnitLayout(
                nullptr, &context.airUnitDefinitions)) {
        return BlockInfoDescriptorChunkResult::Error;
    }
    if (!context.parsedMobilArrays) {
        if (!blockinfo_descriptor_parse_mobil_ref_array(
                    &context.stream,
                    context.descriptorPath,
                    &context.externalRefs,
                    1u,
                    &context.descriptor.counts,
                    &context.models,
                    &context.descriptor.mobilArg1Count) ||
            !blockinfo_descriptor_parse_mobil_ref_array(
                    &context.stream,
                    context.descriptorPath,
                    &context.externalRefs,
                    0u,
                    &context.descriptor.counts,
                    &context.models,
                    &context.descriptor.mobilArg0Count)) {
            return BlockInfoDescriptorChunkResult::Error;
        }
        context.parsedMobilArrays = true;
    } else if (!context.stream.SkipMobilRefArray() ||
               !context.stream.SkipMobilRefArray()) {
        return BlockInfoDescriptorChunkResult::Error;
    }
    if ((chunkId == 0x0304e004u ||
         chunkId == 0x0304e005u ||
         chunkId == 0x0304e008u) &&
        !context.stream.Skip(7u)) {
        return BlockInfoDescriptorChunkResult::Error;
    }
    if ((chunkId == 0x0304e005u || chunkId == 0x0304e008u) &&
        !context.stream.Skip(2u)) {
        return BlockInfoDescriptorChunkResult::Error;
    }
    if (chunkId == 0x0304e008u && !context.stream.SkipString()) {
        return BlockInfoDescriptorChunkResult::Error;
    }
    return context.continueAfterMainBlockInfoChunk
            ? BlockInfoDescriptorChunkResult::Continue
            : BlockInfoDescriptorChunkResult::Complete;
}

bool ParseHelperSources(
        BlockInfoDescriptorParseContext &context,
        bool includeCommon) {
    return context.stream.Skip(4u) &&
           blockinfo_descriptor_parse_helper_source_ref(
                   &context.stream,
                   context.descriptorPath,
                   &context.externalRefs,
                   2u,
                   &context.models,
                   &context.descriptor.helperGroundDescriptorPath) &&
           blockinfo_descriptor_parse_helper_source_ref(
                   &context.stream,
                   context.descriptorPath,
                   &context.externalRefs,
                   3u,
                   &context.models,
                   &context.descriptor.helperAirDescriptorPath) &&
           (!includeCommon ||
            blockinfo_descriptor_parse_helper_source_ref(
                    &context.stream,
                    context.descriptorPath,
                    &context.externalRefs,
                    4u,
                    &context.models,
                    &context.descriptor.helperCommonDescriptorPath));
}

bool ParsePylonSources(BlockInfoDescriptorParseContext &context) {
    if (context.descriptor.parsedPylonSourceChunk) {
        return false;
    }
    std::array<std::optional<BlockInfoParsedMobilSlot>, 3> parsed{};
    std::array<ArchiveNodeReference, 3> sourceNodes{};
    std::size_t sourceCount = 0u;
    for (std::size_t index = 0u; index < parsed.size(); ++index) {
        std::optional<BlockInfoParsedMobilSlot> &source = parsed[index];
        ArchiveNodeReference sourceNode = ArchiveNodeReference::Invalid();
        if (!context.stream.ReadArchiveNodeRef(&sourceNode)) {
            return false;
        }
        if (sourceNode.IsInvalid()) {
            continue;
        }
        sourceCount++;
        if (!sourceNode.IsValid() ||
            sourceNode.Index() >=
                    context.externalRefs.NodeCountForFormatBounds()) {
            return false;
        }
        sourceNodes[index] = sourceNode;
        for (std::size_t prior = 0u; prior < index; ++prior) {
            if (!sourceNode.Matches(sourceNodes[prior])) {
                continue;
            }
            if (!parsed[prior].has_value()) {
                return false;
            }
            source = parsed[prior];
            break;
        }
        if (source.has_value()) {
            continue;
        }
        const BlockInfoDescriptorExternalRef *externalRef =
                context.externalRefs.FindReference(sourceNode);
        if (externalRef == nullptr) {
            source = context.models.ResolveArchiveLocalSource(sourceNode);
        }
        if (source.has_value()) {
            continue;
        }
        const BlockInfoParsedMobilSlot parsedSource{
            static_cast<u32>(5u + index), 0u, 0u};
        if (externalRef == nullptr &&
            !context.models.RecordArchiveLocalRef(
                    sourceNode.Index(), parsedSource, true)) {
            return false;
        }
        const u32 firstModel = context.models.Count();
        if (!blockinfo_descriptor_skip_or_parse_nod_ref_value(
                    &context.stream,
                    sourceNode,
                    context.descriptorPath,
                    &context.externalRefs,
                    parsedSource.selectorGroup,
                    parsedSource.variantIndex,
                    parsedSource.mobilIndex,
                    nullptr,
                    BlockInfoDescriptorNodRefPurpose_PylonMobil,
                    &context.models) ||
            context.models.Count() == firstModel) {
            return false;
        }
        source = parsedSource;
    }
    if (sourceCount != 0u && sourceCount != parsed.size()) {
        return false;
    }
    context.descriptor.pylonSourceMobils = parsed;
    context.descriptor.parsedPylonSourceChunk = true;
    return true;
}

BlockInfoDescriptorChunkResult ParseSupplementalBlockInfoChunk(
        BlockInfoDescriptorParseContext &context,
        u32 chunkId) {
    switch (chunkId) {
    case 0x0304e002u:
        return context.stream.SkipCMwId()
                ? BlockInfoDescriptorChunkResult::Continue
                : BlockInfoDescriptorChunkResult::Complete;
    case 0x0304e003u:
        return context.stream.Skip(8u) && context.stream.SkipCMwId()
                ? BlockInfoDescriptorChunkResult::Continue
                : BlockInfoDescriptorChunkResult::Complete;
    case 0x0304e006u:
        return context.stream.Skip(4u) && context.stream.SkipCMwId()
                ? BlockInfoDescriptorChunkResult::Continue
                : BlockInfoDescriptorChunkResult::Complete;
    case 0x0304e007u:
        if (!context.stream.ReadIso4(context.descriptor.blockInfoIso4A)) {
            return BlockInfoDescriptorChunkResult::Complete;
        }
        BlockInfoDescriptorCopyIso4(
                context.descriptor.blockInfoIso4B,
                context.descriptor.blockInfoIso4A);
        context.descriptor.hasBlockInfoIso4A = 1u;
        context.descriptor.hasBlockInfoIso4B = 1u;
        return BlockInfoDescriptorChunkResult::Continue;
    case 0x0304e009u:
    case 0x0304e00du:
        return context.stream.Skip(4u)
                ? BlockInfoDescriptorChunkResult::Continue
                : BlockInfoDescriptorChunkResult::Complete;
    case 0x0304e00fu: {
        u32 respawnUsesCurrentTransform = 0u;
        if (!context.stream.ReadU32(&respawnUsesCurrentTransform)) {
            return BlockInfoDescriptorChunkResult::Complete;
        }
        context.descriptor.blockInfoRespawnUsesCurrentTransform =
                respawnUsesCurrentTransform != 0u;
        return BlockInfoDescriptorChunkResult::Continue;
    }
    case 0x0304e00au:
        return ParseHelperSources(context, false)
                ? BlockInfoDescriptorChunkResult::Continue
                : BlockInfoDescriptorChunkResult::Complete;
    case 0x0304e00bu:
    case 0x0304e00eu:
        return ParseHelperSources(context, true)
                ? BlockInfoDescriptorChunkResult::Continue
                : BlockInfoDescriptorChunkResult::Complete;
    case 0x0304e00cu:
        if (!context.stream.ReadIso4(context.descriptor.blockInfoIso4A) ||
            !context.stream.ReadIso4(context.descriptor.blockInfoIso4B)) {
            return BlockInfoDescriptorChunkResult::Complete;
        }
        context.descriptor.hasBlockInfoIso4A = 1u;
        context.descriptor.hasBlockInfoIso4B = 1u;
        return BlockInfoDescriptorChunkResult::Continue;
    case TMNF_CLASS_CGameCtnBlockInfoRoad:
        return context.descriptor.descriptorClassId ==
                           TMNF_CLASS_CGameCtnBlockInfoRoad &&
                       context.stream.SkipNodRef()
                ? BlockInfoDescriptorChunkResult::Continue
                : BlockInfoDescriptorChunkResult::Error;
    case TMNF_CLASS_CGameCtnBlockInfoPylon:
        if (context.descriptor.descriptorClassId !=
                TMNF_CLASS_CGameCtnBlockInfoPylon) {
            if (context.wrongRootPylonChunkOut != nullptr) {
                *context.wrongRootPylonChunkOut = true;
            }
            return BlockInfoDescriptorChunkResult::Error;
        }
        if (!ParsePylonSources(context)) {
            return BlockInfoDescriptorChunkResult::Error;
        }
        return BlockInfoDescriptorChunkResult::Continue;
    default: {
        u32 magic = 0u;
        u32 size = 0u;
        return context.stream.ReadU32(&magic) && magic == 0x534b4950u &&
                       context.stream.ReadU32(&size) && context.stream.Skip(size)
                ? BlockInfoDescriptorChunkResult::Continue
                : BlockInfoDescriptorChunkResult::Error;
    }
    }
}

}  // namespace

std::optional<BlockInfoDescriptorParseOutput>
BlockInfoDescriptorMobilParser::ParseDescriptorAndModels(
        const BlockInfoDescriptorParseRequest &request) {
    if (request.wrongRootPylonChunkOut != nullptr) {
        *request.wrongRootPylonChunkOut = false;
    }
    if (request.bytes == nullptr || request.descriptorPath == nullptr) {
        return std::nullopt;
    }
    BlockInfoDescriptorExternalRefs externalRefs;
    if (!externalRefs.ParseGbx(request.bytes, request.byteCount)) {
        return std::nullopt;
    }
    externalRefs.AttachInstalledPack(request.installedPack);

    BlockInfoDescriptorParseOutput output{};
    output.descriptor.counts.Init();
    output.descriptor.descriptorClassId = externalRefs.ClassId();
    if (!assign_cstr_to_string(
                &output.descriptor.descriptorPath, request.descriptorPath)) {
        return std::nullopt;
    }
    BlockInfoDescriptorUnitGeometry sizes{};
    if (sizes.ParseGbxDescriptorBytes(request.bytes, request.byteCount)) {
        output.descriptor.unitGeometry = std::move(sizes);
    }

    BlockInfoSizeParseStream stream{};
    stream.bytes = request.bytes;
    stream.byteCount = request.byteCount;
    stream.offset = externalRefs.BodyOffsetForFormatParser();
    stream.SetBlockInfoSourceRefs(&externalRefs);
    if (!stream.SkipCommonPrefix()) {
        return std::nullopt;
    }
    BlockInfoDescriptorParseContext context{
        stream,
        request.descriptorPath,
        externalRefs,
        output.descriptor,
        output.descriptor.groundUnitDefinitions,
        output.descriptor.airUnitDefinitions,
        output.models,
        request.continueAfterMainBlockInfoChunk,
        request.wrongRootPylonChunkOut,
    };
    for (;;) {
        u32 chunkId = 0u;
        if (!stream.ReadU32(&chunkId)) {
            return std::nullopt;
        }
        if (chunkId == CMwNodArchiveFacadeSentinel) {
            return output;
        }
        const BlockInfoDescriptorChunkResult result =
                IsMainBlockInfoChunk(chunkId)
                ? ParseMainBlockInfoChunk(context, chunkId)
                : ParseSupplementalBlockInfoChunk(context, chunkId);
        if (result == BlockInfoDescriptorChunkResult::Complete) {
            return output;
        }
        if (result == BlockInfoDescriptorChunkResult::Error) {
            return std::nullopt;
        }
    }
}

void BlockInfoDescriptorMobilParser::TryParseHelperPaths(
        const BlockInfoDescriptorParseRequest &request,
        BlockInfoDescriptorParseResult &descriptor) {
    BlockInfoDescriptorParseRequest helperRequest = request;
    helperRequest.continueAfterMainBlockInfoChunk = true;
    auto helper = ParseDescriptorAndModels(helperRequest);
    if (!helper.has_value()) {
        return;
    }
    descriptor.blockInfoRespawnUsesCurrentTransform =
            helper->descriptor.blockInfoRespawnUsesCurrentTransform;
    descriptor.hasBlockInfoIso4A = helper->descriptor.hasBlockInfoIso4A;
    descriptor.hasBlockInfoIso4B = helper->descriptor.hasBlockInfoIso4B;
    BlockInfoDescriptorCopyIso4(
            descriptor.blockInfoIso4A,
            helper->descriptor.blockInfoIso4A);
    BlockInfoDescriptorCopyIso4(
            descriptor.blockInfoIso4B,
            helper->descriptor.blockInfoIso4B);
    descriptor.helperGroundDescriptorPath =
            helper->descriptor.helperGroundDescriptorPath;
    descriptor.helperAirDescriptorPath =
            helper->descriptor.helperAirDescriptorPath;
    descriptor.helperCommonDescriptorPath =
            helper->descriptor.helperCommonDescriptorPath;
}
