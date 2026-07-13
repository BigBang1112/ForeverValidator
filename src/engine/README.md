# Replay Physics Engine Sources

This directory owns the deterministic Replay -> Physics engine code used by the
validator.

The code here is normal source-owned C++. Engine behavior is expressed through
typed objects and semantic APIs; archive parsing belongs under `src/format`.
Do not add analysis-data bypasses, replay exceptions, or alternate runtime
modes.

ForeverValidator is built with the repository `Makefile`; the command-line
executable is `build/forevervalidator`.
