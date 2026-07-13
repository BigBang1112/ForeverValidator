#include <forevervalidator/native.h>

#include "platform/native/native_system_file_operations.h"

#include <fstream>
#include <limits>
#include <new>

namespace forevervalidator {
namespace {

ValidationError NativeReadError(
        ValidationErrorCategory category,
        ValidationErrorCode code,
        ValidationStage stage,
        ValidationFailureReason reason,
        const ReplayIdentity &identity,
        const std::string &path,
        const char *diagnostic) {
    ValidationError error;
    error.category = category;
    error.code = code;
    error.stage = stage;
    error.reason = reason;
    error.replay = identity;
    error.relatedAsset = path;
    error.diagnostic = diagnostic;
    return error;
}

}  // namespace

Result<AssetBytes> ReadNativeReplayFile(
        const std::string &path,
        const ReplayIdentity &identity) noexcept {
    tmnf::platform::InstallNativeSystemFileOperations();
    try {
        if (path.empty()) {
            return Result<AssetBytes>::Failure(NativeReadError(
                    ValidationErrorCategory::InvalidInput,
                    ValidationErrorCode::InvalidArgument,
                    ValidationStage::ReplayDecoding,
                    ValidationFailureReason::EmptyReplayPath,
                    identity,
                    path,
                    "replay path is empty"));
        }

        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        if (!stream) {
            return Result<AssetBytes>::Failure(NativeReadError(
                    ValidationErrorCategory::Replay,
                    ValidationErrorCode::ReplayDecodingFailed,
                    ValidationStage::ReplayDecoding,
                    ValidationFailureReason::ReplayFileOpenFailed,
                    identity,
                    path,
                    "could not open replay file"));
        }
        const std::streamoff length = stream.tellg();
        if (length <= 0 ||
            static_cast<std::uintmax_t>(length) >
                    std::numeric_limits<std::size_t>::max()) {
            return Result<AssetBytes>::Failure(NativeReadError(
                    ValidationErrorCategory::Replay,
                    ValidationErrorCode::ReplayDecodingFailed,
                    ValidationStage::ReplayDecoding,
                    ValidationFailureReason::ReplayFileLengthInvalid,
                    identity,
                    path,
                    "invalid replay file length"));
        }
        stream.seekg(0, std::ios::beg);
        AssetBytes bytes(static_cast<std::size_t>(length));
        if (!stream.read(reinterpret_cast<char *>(bytes.data()), length)) {
            return Result<AssetBytes>::Failure(NativeReadError(
                    ValidationErrorCategory::Replay,
                    ValidationErrorCode::ReplayDecodingFailed,
                    ValidationStage::ReplayDecoding,
                    ValidationFailureReason::ReplayFileReadFailed,
                    identity,
                    path,
                    "could not read complete replay file"));
        }
        return Result<AssetBytes>::Success(std::move(bytes));
    } catch (const std::bad_alloc &) {
        return Result<AssetBytes>::Failure(NativeReadError(
                ValidationErrorCategory::Allocation,
                ValidationErrorCode::AllocationFailed,
                ValidationStage::ReplayDecoding,
                ValidationFailureReason::AllocationFailed,
                identity,
                path,
                "allocation failed while reading replay"));
    } catch (...) {
        return Result<AssetBytes>::Failure(NativeReadError(
                ValidationErrorCategory::Internal,
                ValidationErrorCode::UnexpectedFailure,
                ValidationStage::ReplayDecoding,
                ValidationFailureReason::UnexpectedFailure,
                identity,
                path,
                "unexpected failure while reading replay"));
    }
}

}  // namespace forevervalidator
