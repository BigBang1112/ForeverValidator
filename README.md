# ForeverValidator

ForeverValidator is a deterministic C++17 validator for TrackMania Nations
Forever Stadium replays.
It decodes the replay and embedded challenge, loads the original Stadium
assets, and reruns the recorded inputs through reconstructed game physics.

## Scope and result

For replays with version-9 fixed-step car state, the validator compares
simulated positions with the recorded ghost trajectory. It also validates race
completion, finish time, and respawns.

TMNF validation replays may intentionally contain no ghost-state samples (the
replays specifically exported for validation). They still run the complete map
and vehicle simulation. Because no trajectory is stored, their finish and
respawn result is resolved from the canonical recorded validation events after
the physics run; no position comparison is claimed.

`valid` means every check available for that replay form passed. `invalid` means
simulation completed but the applicable result did not match. `error` means the
replay, assets, map, or simulation could not be processed.

Supported input is currently limited to TMNF Stadium car replays with an
embedded challenge and input stream, using either version-9 fixed-step car
state or the canonical empty state used by validation replays.

## Quick start

Dependencies: a C++17 compiler, GNU Make, zlib, and OpenSSL libcrypto. The
bundled command-line frontend targets POSIX systems; the core validation
library remains independent of native filesystem access.

```sh
make -j"$(nproc)"

build/forevervalidator \
  --pak-dir "/path/to/TmNationsForever/Packs" \
  "/path/to/run.Replay.Gbx"
```

The pack directory must contain `packlist.dat` and `Stadium.pak`; current
support is limited to TMNF. Use `--out` to write one JSON result, or validate
a directory recursively with `--out-dir`:

```sh
build/forevervalidator \
  --pak-dir "/path/to/TmNationsForever/Packs" \
  --out-dir /tmp/tmnf-results \
  "/path/to/replays"
```

## Library API

The backend is usable without the CLI or native filesystem:

```cpp
#include <forevervalidator/validation.h>

#include <utility>

using namespace forevervalidator;

Result<ValidationReport> validateFromMemory(
    AssetBytes replayBytes,
    AssetBytes packlistBytes,
    AssetBytes stadiumBytes) {
    AssetProvider provider = [
        packlist = std::move(packlistBytes),
        stadium = std::move(stadiumBytes)
    ](const AssetRequest &request) -> Result<AssetBytes> {
        if (request.logicalIdentifier == "packlist.dat") {
            return Result<AssetBytes>::Success(packlist);
        }
        if (request.logicalIdentifier == "Stadium.pak") {
            return Result<AssetBytes>::Success(stadium);
        }

        ValidationError error;
        error.category = ValidationErrorCategory::Asset;
        error.code = ValidationErrorCode::AssetLoadingFailed;
        error.stage = ValidationStage::AssetLoading;
        error.reason = ValidationFailureReason::RequiredAssetMissing;
        error.relatedAsset = request.logicalIdentifier;
        return Result<AssetBytes>::Failure(std::move(error));
    };

    auto source = CreateAssetSource(std::move(provider));
    if (!source) {
        return Result<ValidationReport>::Failure(
            std::move(source).Error());
    }

    auto context = CreateValidationContext(std::move(source).Value());
    if (!context) {
        return Result<ValidationReport>::Failure(
            std::move(context).Error());
    }

    return ValidateReplay(
        context.Value(),
        {replayBytes.data(), replayBytes.size()},
        {"run.Replay.Gbx"});
}
```

The caller loads the replay and both pack files into `AssetBytes`. Every public
operation returns a typed `Result<T>`; check it before accessing `Value()` and
propagate or inspect `Error()` on failure.

```text
libforevervalidator_core.a    portable validation backend
libforevervalidator_json.a    JSON serialization
libforevervalidator_native.a  native file and pack-directory adapters
forevervalidator              command-line frontend
```

## Repository layout

```text
include/forevervalidator  public API
src/app/cli               command-line frontend
src/validation            validation and JSON output
src/simulation            controls and persistent simulation runtime
src/engine                reconstructed game physics and scene code
src/format                replay, GBX, pack, tuning, and solid decoding
src/platform/native       native filesystem and process integration
```
