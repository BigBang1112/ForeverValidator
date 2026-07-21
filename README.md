# ForeverValidator

ForeverValidator is a deterministic C++17 replay validator for TrackMania
United Forever. It decodes a replay and its embedded challenge, loads the
original installed game assets, and reruns the recorded inputs through
reconstructed game physics.

## Supported United content

The validator supports the seven vanilla map environments and vehicles:

| Map environment | Vehicle |
| --- | --- |
| Alpine / Snow | SnowCar |
| Speed / Desert | DesertCar |
| Rally | RallyCar |
| Island | IslandCar |
| Coast | CoastCar |
| Bay | BayCar |
| Stadium | StadiumCar |

All 49 vanilla map-environment and vehicle combinations are routed from the
identifiers embedded in the replay. TMUnlimiter environments, custom vehicles,
and custom physics are outside the supported scope.

Race, Platform, Puzzle, and Stunts replays are supported. Shortcut and
multilap challenges use Race completion and checkpoint semantics. Race,
Platform, and Puzzle validate completion, time, and respawns where recorded.
Stunts validates completion, respawns, and the independently simulated stunt
score.

TMInterface replays are detected from the input clock marker written by
TMInterface. ForeverValidator normalizes their recorded input timeline and
checks whether those inputs reproduce the stored ghost, while clearly marking
the replay as TMInterface-generated. These replays remain invalid for
competitive validation even when the inputs and ghost match.

## Game files

ForeverValidator requires the `Packs` directory from a complete TrackMania
United Forever installation. A TrackMania Nations Forever installation is not
sufficient: it does not contain the six non-Stadium environments or their
vehicles.

The directory must contain `packlist.dat` and the installed United packs,
including Alpine, Speed, Rally, Island, Coast, Bay, Stadium, Game, and Resource.

## Linux build

Dependencies are GNU Make, a C++17 compiler, zlib development files, and
OpenSSL libcrypto development files.

```sh
make -j"$(nproc)"
```

This creates:

```text
build/native/forevervalidator
build/native/libforevervalidator_core.a
build/native/libforevervalidator_json.a
build/native/libforevervalidator_native.a
```

## Windows build

### From Linux

The reproducible cross-build requires a MinGW-w64 C++ toolchain, `curl`,
`perl`, `tar`, and `sha256sum`:

```sh
JOBS="$(nproc)" make -j"$(nproc)" windows
```

The build downloads the pinned OpenSSL 3.5.7 and zlib 1.3.2 source archives,
verifies their SHA-256 checksums, builds static Windows libraries, and creates
`build/windows/forevervalidator.exe`. The executable does not depend on MinGW
runtime DLLs.

### On Windows

Windows users can download a ready-to-run 64-bit executable from the
[GitHub releases](https://github.com/Skycrafter-dev/ForeverValidator/releases);
building from source is optional.

To build from source, open PowerShell in the repository and run:

```powershell
.\build-windows.bat
```

The script downloads and verifies pinned portable build tools and dependency
sources, then creates `dist\forevervalidator.exe`. Downloads and compiled
dependencies are cached for later builds. The first build requires an internet
connection, but no administrator privileges, package manager, preinstalled
compiler, or manual `PATH` changes.

## Command line

Validate one replay and write its JSON report to standard output:

```sh
build/native/forevervalidator \
  --pak-dir "/path/to/TmUnitedForever/Packs" \
  "/path/to/run.Replay.Gbx"
```

Use `--out` for one output file, or validate files and directories in a batch
with `--out-dir`:

```sh
build/native/forevervalidator \
  --pak-dir "/path/to/TmUnitedForever/Packs" \
  --out-dir /tmp/validation-results \
  "/path/to/replays"
```

Select a simulation backend at runtime with `--backend reference`,
`--backend optimized-cpu`, or `--backend batched`. Single-replay validation
defaults to the authoritative reference backend. Multi-replay runs default to
the ordered batched backend. `--batch-size N` controls how many replay files
are loaded and submitted together, and defaults to 10.

A single replay returns exit status 0 for valid, 1 for invalid, and a distinct
nonzero error code when replay decoding, asset loading, or simulation cannot
be completed. JSON includes typed map environment, vehicle, play mode,
replay provenance, input/ghost match, expected and observed outcomes,
trajectory counts, maximum deviation, and the first divergence when present.
TMInterface replays use the `tminterface_replay` status and expose
`replay_file_metadata.replay_provenance` plus `input_ghost_match`; the CLI also
prints the TMInterface classification to stderr.

### JSON result

Each validated replay produces a `forevervalidator-result-v1` JSON object:

```json5
{
  status: "valid",                 // outcome, e.g. wrong_simulation, race_time_mismatch, tminterface_replay
  validate_result_code: 1,         // 1 valid, 0 invalid, 2 wrong simulation, null if unavailable/errored
  message: null,                   // human-readable detail for non-valid statuses
  measured_sample_count: 295,      // ghost samples actually compared
  expected_sample_count: 295,      // ghost samples expected from the replay
  max_deviation: 0,                // largest position deviation found
  max_deviation_time_ms: -1,       // time of the largest deviation, -1 if exact
  max_deviation_distance: 0,       // distance of the largest deviation
  compared_exact_ghost_state_count: 295, // samples that matched the ghost state exactly
  wrong_simulation: false,         // true if the simulated trajectory diverged from the ghost
  input_ghost_match: "Unavailable",// Unavailable | Match | Mismatch (mainly for TMInterface replays)
  first_divergence: null,         // first sample where simulation diverged, or null
  first_exact_deviation: null,     // first sample with any exact-state deviation, or null
  replay_file_metadata: {          // decoded replay info
    replay_provenance: "Unmarked", // Unmarked | TMInterface
    map_environment: "Stadium",
    vehicle_model: "StadiumCar",
    play_mode: "Race",
    expected_stunts_score: null,
    sample_count: 325,
    input_duration_ms: 29420,
    expected_race_time_ms: 29420,
    expected_respawns: 0,
    // ...additional decode/sample counters
  },
  simulation_outcome: {             // independently simulated result
    race_completed: true,
    race_time_ms: 29420,
    stunts_score: null,
    respawn_count: 0,
  },
  schema: "forevervalidator-result-v1",
  replay: "/path/to/run.Replay.Gbx", // replay path as passed on the command line
  valid: true,                       // true only when status is "valid"
}
```

Batch runs (`--out-dir`) wrap these per-replay reports in a
`forevervalidator-batch-v1` summary. Replays that fail to decode or validate
instead produce a `forevervalidator-error-v1` object with `category`, `code`,
and `stage` fields describing the failure.

## Library API

The public API separates portable validation from native file access:

```cpp
#include <forevervalidator/native.h>
#include <forevervalidator/validation.h>

#include <utility>

using namespace forevervalidator;

void validateOneReplay() {
    auto source = OpenInstalledPackDirectory(
        "/path/to/TmUnitedForever/Packs");
    if (!source) {
        return; // Inspect source.Error().
    }

    auto context = CreateValidationContext(std::move(source).Value());
    auto replay = ReadNativeReplayFile(
        "run.Replay.Gbx", {"run.Replay.Gbx"});
    if (!context || !replay) {
        return;
    }

    auto report = ValidateReplay(
        context.Value(),
        {replay.Value().data(), replay.Value().size()},
        {"run.Replay.Gbx"});
}
```

Applications can instead provide assets from memory with `AssetProvider` and
`CreateAssetSource`. A validation context lazily caches pack and vehicle data,
so it can be reused for replays from different United environments. Every
public operation returns a typed `Result<T>`.

`ValidationOptions::backend` selects the backend for one library call. The
optimized CPU backend currently forwards to the reference implementation while
remaining behind an independent implementation boundary. `ValidateReplayBatch`
executes an ordered list of independent replay requests and preserves an
individual report or error for every item.

## Experimental physics sandbox

`<forevervalidator/experimental/physics_sandbox.h>` provides the deliberately
unstable `PhysicsSandbox` integration surface for simulation and backend
development. A sandbox consumes its own `AssetSource`, loads the embedded map
and input timeline from a replay, advances by an exact tick count, and captures
or restores opaque deterministic states. Static map collision structures are
owned by the sandbox rather than copied into state snapshots.

The API exposes replay-level input events and search-relevant state such as car
dynamics, race-relative time, checkpoints, laps, respawns, and completion. It
does not expose arbitrary engine objects, per-tick callbacks, or TAS search
concepts. Snapshot restoration currently rebuilds the mutable simulation by
deterministically replaying the captured input prefix.

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
src/simulation            controls and deterministic simulation runtime
src/engine                reconstructed game physics and scene code
src/format                replay, GBX, pack, tuning, and solid decoding
src/platform/native       native filesystem and pack-directory integration
```

## Acknowledgements

Reverse engineering, implementation, and repetitive structure mapping were
accelerated with LLM assistance.
