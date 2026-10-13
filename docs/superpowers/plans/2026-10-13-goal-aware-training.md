# Goal-aware training implementation plan

> **For agentic workers:** Use the existing task assignments and verification ledger below; do not restart completed tasks.

**Goal:** Fix arena model ownership, expose every player's goal to Soo and Min, verify supported configurations, and restart Min with a durable run.

**Architecture:** An explicit root player identifies the model owner throughout a search. Versioned encoder v2 appends one canonical target-camp membership plane per player to legacy occupancy and finished planes. Checkpoints, replay, deployment artifacts, training and GUI inference select the same encoder contract.

**Tech stack:** Native C++26, CMake/CTest, LibTorch/CUDA. JAX is used for numerical diagnostic analysis.

**Spec:** The user approved the diagnosed arena and observation fixes and requested verification before restarting Min. The detailed contract is below. Separate turn-order experts are a proposed follow-up experiment, not part of this implementation.

## Global constraints

- Preserve all frozen legacy fixtures and shipped model artifacts.
- Standard match factories remain the authority for camps and goals.
- Legacy encoding remains readable and explicitly selected by old configs/artifacts.
- New encoding is `diamond-camp-relative-goals-v2`: occupancy `[0,P)`, finished `[P,2P)`, goal membership `[2P,3P)`, with the same canonical player order and board rotation.
- Soo uses 6 features and one value; Min uses 9 features and three placement values.
- A legacy warm start copies all learned parameters, zero-pads only input-projection columns, and starts fresh optimizer/replay. Exact resume rejects incompatible encoders.
- Omit the new config field when legacy is selected to preserve existing resolved-config identities.
- Do not deploy, overwrite historical training data, or claim universal learned board coverage.

## Review focus

- Search leaves change players: owner must remain the actual move's root player, including after placement.
- Old weights/replay must never be silently interpreted as new features.
- Goal planes must transform with actor rotation and player-channel permutation.
- Both GUI families and deployment export must read the declared encoder.
- The padded warm start must retain predictions before learning, while new weights can receive gradients.

## Tasks and verification

- [x] Arena owner field, routing validation, full-search regression tests for Soo/Min order/actor combinations.
- [x] Versioned encoder/search contract; all supported orders, actors, rotations and finish flags; reachable v1 aliases distinguished by v2.
- [x] Compatibility/model migration, checkpoint/replay rejection, artifact export/reload and gradient tests.
- [x] Config and production wiring, version-aware GUI inference, CLI warm-start migration with durable provenance.
- [x] Native core/training suites and available Qt checks; independent review of changed interfaces.
- [ ] CUDA smoke, new balanced Min run with immutable config/checkpoints, verify completed self-play and actual learning updates before handoff.

## Execution ledger

- Work is on `codex/goal-aware-training` in the existing checkout to reuse the verified native build and local training assets.
- Root owns integration/config/runtime and final training launch; separate agents own arena, encoder, and compatibility/migration changes.
- Baseline: clean tree; previous diagnostic work verified core/training suites and preserved artifacts.
- All 54 native training tests passed, including the four CUDA tests, after final integration. Independent production review found no consequential issue. Generic self-play, learning and training-step benchmarks passed with v2 inputs; vacancy distillation passed both encoder contracts after reproducing and removing its six-feature assumption. A v2 Min CUDA self-play smoke also passed.
- Qt inference wiring was reviewed, but this machine lacks Qt development packages, so the Qt application could not be compiled here.
