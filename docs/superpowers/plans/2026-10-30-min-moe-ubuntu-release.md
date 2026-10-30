# Min-MoE-v1.0.0 and Ubuntu Desktop release

The user explicitly requests deployment, Desktop integration and an Ubuntu executable package after weakness remediation. They opt out of repeated approval stalls. The weakness pilot is complete and did not establish a strength gain; do not release those pilot weights. General learners and heartbeat stay stopped.

## Design choices

Use the existing deterministic full-match Cyclic/Reverse classifier and MinExpertRouter. Export verified C4096(128x6) and preserved big106(512x12), whose routed pair passed its independent strength gate. No distillation, averaging, learned gate, fallback expert or new training. Preserve each checkpoint and provenance.

Extend the existing format-v3 architecture union with a strict Min MoE variant containing exactly two validated format-v3 leaf artifacts. Keep game family min and public version1.0.0; artifact/release display name Min-MoE-v1.0.0. Root composite identities bind the router declaration and both child identities; preserve original source checkpoint identities in each child. Reject missing/corrupt/swapped/nested experts. Reuse model index/default/catalog interfaces. Current GitHub package contains Min2.0.1 and no Min1.0.0; verify destination absence again before publishing.

Add a small CPU Min BatchEvaluator adapter in Engine Runtime so Desktop uses the existing router without copying training code. Move the current single-position Torch inference into it. Single Min and Soo remain supported. No service or scheduler abstraction.

Build a matching Release CPU Runtime SDK and update the Desktop lock with its actual archive/manifest identities without bypassing validation. Desktop consumes installed SDK only; never copy checkpoints/training into Desktop. Add native Ubuntu packaging beside the existing Windows path. Bundle selected Qt/QML dependencies, CPU LibTorch, source/topology notices and runtime-only models. Prefer a relocatable launcher/folder archive that runs on this Ubuntu26.04x86_64 machine without Python or an SDK. Do not claim older Ubuntu compatibility without testing.

Use GCC15, Ninja, sccache and mold.32logicalCPUs/45GiBavailable; start with24compileworkers and OMP/MKL1, lower only if measured memory pressure warrants. Mold uses parallel linking. Parallelize independent build/validation work; sequential GPU parity where needed. Compress artifacts with available multicore compressor.

## Implementation

- [x] Engine: inspect references; add failing native MoE probe; strict variant validation/writer/runtime copy; CPU evaluator and SDK export; build and validate leaves, corruption rejection, router parity over6orders and changing actors.
- [x] Export frozen verified experts with native checkpoint exporter and exact parity. Stage a new runtime-only Min MoE bundle plus preserved Soo model and index defaults. Preserve all existing model paths.
- [x] Desktop: exact checkout Serena/Graphify, minimal catalog label/download/default integration, route inference with installed SDK. Respect AGENTS: no new test or CI layers; validate behavior directly.
- [x] Release: build/install/hash CPU Release SDK and lock; optimized Desktop build; Linux Qt/QML/library closure and launcher; relocated offline launch and actual Min inference. Inspect dependency/provenance/no-Python/no-training payload.
- [x] Review with fresh reviewer as required by execution workflow, fix evidence-backed findings, refresh both graphs including affected doc relationships.
- [x] Publish requested version using existing authorized sources after concrete local artifact verification. Do not overwrite an existing version or silently change hosting identities. Prepare Ubuntu launchable app/archive with SHA256 and clear paths.

## Checkout ruling

Engine has inherited related uncommitted training changes; retain its current codex checkout rather than losing prerequisites in a fresh worktree. Desktop starts clean except a graph query stamp and already has a codex branch. Reuse it for the authorized integration; isolate new artifact/build directories. Do not commit inherited unrelated changes.

## Verification

Native leaves each passed12exact export-parity cases. Final CPU release bundle passed18router/actor cases and rejection of missing/corrupt/swapped/tampered/symlinked experts. Actual Desktop search passed both routes and all3actors using packaged files. Relocated archive clean-environment GUI startup and138ELF dependency closures passed. Runtime package uses installed matching SDK with original Debug lock preserved and actual Release hashes. Native builds used24Desktop/8incrementalEngine workers, sccache and mold; compression zstd -T0. General training and heartbeat remain stopped. Final artifact/proof: artifacts/min-moe-release-20261030/release-result.json.

Public deployment completed: GitHub Min-MoE-v1.0.0 release (Ubuntu/archive/manifest/SHA256SUMS); HuggingFace existing bucket models/min/1.0.0 plus preserved index entries and new Min default. All219remote runtime files downloaded and SHA256 compared exactly; catalog endpoint returns entire new payload in a single response. GitHub asset SHA256 values match local archives.
