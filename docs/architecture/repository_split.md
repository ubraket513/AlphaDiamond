# Repository split and SDK upgrades

AlphaDiamond remains the Engine/Training source and model publishing repository.
AlphaDiamondDesktop is an independent Qt/QML consumer of installed Runtime SDKs.
No engine checkout is required for ordinary Desktop builds. A missing binary
platform may explicitly build an SDK from the exact lock revision in
`ubraket513/AlphaDiamond`; this is a separate producer build, never an implicit
`add_subdirectory` fallback.

The approved pre-extraction source revision is
`9ba28a0619c540443f1aeca471b16f95adfef5c0`; the original baseline is
`adc2037`. Desktop records its extraction source and filtered revision in
`docs/history/extraction.json`. The implementation session preserves full
source-to-filtered commit and path maps and verification evidence in its task
artifacts. The original working checkout
and preexisting local changes were excluded from extraction.

## Independent releases

A UI-only change builds/tests/packages Desktop against the unchanged SDK lock.
A trainer-only change builds/tests Engine and does not change Desktop's lock.
New encoder or architecture experiments cannot reach the app until a compatible
SDK and model contract are deliberately accepted. Format-v3 metadata, tensor,
encoder, rules, digest and model identity meanings remain unchanged.

For an SDK upgrade:

1. Build and test the intended Engine revision and configuration. Rebuild all
   Runtime targets (including `engine_sdk_provenance`) before installation so
   revision/content evidence matches the installed libraries.
2. Produce and validate an SDK with a new version, archive SHA-256 and manifest.
   Run relocated consumer tests with producer source/build unavailable.
3. In a separate Desktop change, update the exact SDK version, source revision,
   archive digest and platform/compiler/Torch/configuration/ABI lock together.
4. Run Desktop controller, replay, no-model topology, Soo/Min inference, model
   rejection, rating and profile compatibility tests, then package smokes.

Debug consumers use Debug SDKs; final optimized Release artifacts are selected
explicitly. Linux development retains GCC, mold, Ninja, sccache and low-cost
Debug compilation with LTO off. Build and test worker counts account for CPU,
memory and nested threads in both repositories.

## Models and rollback

Engine retains `models/index.json`, GitHub `ubraket513/alphadiamond` and Hugging
Face `ubraket513/AlphaDiamond`; their download URLs do not follow the new Desktop
remote. Runtime bundles remain distinct from training checkpoints. Desktop links
the authoritative rules, encoder and artifact documentation in Engine.

An SDK regression is rolled back by restoring the previous Desktop SDK lock and
matching artifact. A model regression is rolled back by selecting the previous
validated model index/default. Existing installation identity, saved games,
model cache and rating outbox retain their established application paths.
Windows package deployment smoke remains a platform CI gate when development
is performed on Linux.
