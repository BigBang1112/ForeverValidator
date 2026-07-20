#include "format/static_solid/static_solid_archive_feedback.h"
#include "format/archive/classic_buffer_crypted.h"
#include <new>
int CGameCtnReplayStaticSolidArchiveFeedback::QueueDeferred(
        u32 value,
        int nested) {
    if (queuedValues.size() >= QueuedValueLimit) {
        return 0;
    }
    try {
        queuedValues.push_back(value);
    } catch (const std::bad_alloc &) {
        return 0;
    }
    appliedCount++;
    if (nested) {
        nestedAppliedCount++;
    }
    return 1;
}

void CGameCtnReplayStaticSolidArchiveFeedback::RecordDirect(int nested) {
    appliedCount++;
    if (nested) {
        nestedAppliedCount++;
    }
}

int CGameCtnReplayStaticSolidArchiveFeedback::ApplyValue(
        CClassicBufferCrypted *reader,
        u32 value,
        int nested,
        int deferUntilCompressedPageRead) {
    if (reader == nullptr) {
        if (deferUntilCompressedPageRead) {
            return 0;
        }
        // Decoded archives retain feedback accounting without mutating a cipher.
        RecordDirect(nested);
        return 1;
    }
    if (deferUntilCompressedPageRead) {
        return QueueDeferred(value, nested);
    }
    reader->MixPageFeedbackWord(value);
    RecordDirect(nested);
    return 1;
}

int CGameCtnReplayStaticSolidArchiveFeedback::FlushTo(
        CClassicBufferCrypted *reader) {
    if (queuedValues.empty() || reader == nullptr) {
        return 1;
    }
    for (u32 value : queuedValues) {
        reader->MixPageFeedbackWord(value);
    }
    queuedValues.clear();
    return 1;
}

u32 CGameCtnReplayStaticSolidArchiveFeedback::RootAppliedFlag() const {
    return appliedCount != 0u ? 1u : 0u;
}

u32 CGameCtnReplayStaticSolidArchiveFeedback::AppliedCount() const {
    return appliedCount;
}

u32 CGameCtnReplayStaticSolidArchiveFeedback::NestedAppliedCount() const {
    return nestedAppliedCount;
}
