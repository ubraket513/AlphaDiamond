# The replay store is a durable dataset with a stateless sampler

## What changed

`ReplayStore::sample()` used to be a durable transaction. Every minibatch
advanced a persisted RNG, wrote a `selection_transaction` record naming every
selected row, and rewrote `manifest.json` — once per training step. A TRAIN
stage of 1024 steps therefore performed 1024 manifest writes, each one
serialising and hashing every chunk descriptor in the store, and the manifest
digest changed under a stage that logically only reads.

Sampling is now a pure function:

```text
samples = replay.sample(batch, replay_sampling_seed(replay_seed, iteration, step))
```

It reads memory and copies rows. It opens no file, mutates no sampler state,
and can be called any number of times in any order.

## Why this is safe for resume

The old design persisted sampler position so that a resumed TRAIN could
continue the draw sequence. It does not need to: TRAIN has no partial-credit
resume. If the candidate checkpoint is absent, the stage re-runs from step 0.

The seed is derived from the run's `replay.seed`, the iteration index and the
*local* training step, so a stage re-run from step 0 against unchanged replay
contents reproduces the identical minibatch sequence, and the resumed run's
final model matches an uninterrupted one. Determinism comes from the seed
derivation, not from durable state — which is why the durable state was
removable without weakening the contract.

## The manifest is contents identity only

`manifest.json` is schema 4 and answers exactly one question: which samples
does this store hold, under which compatibility, at which capacity.

```text
aborted  capacity  chunks  compatibility  game_ids  schema_version
```

Gone: `rng`, `selection_transaction`, `ingest_transaction`. Training cannot
change the manifest, so its digest — which checkpoint provenance records and
`validate_checkpoint_context` enforces — is stable across a TRAIN stage by
construction rather than by convention.

The current store accepts schema 4 (JSON chunks) and schema 5 (JSON or binary
chunks). Schemas 1, 2, and 3 are rejected. The frozen files under
`tests/golden/replay-v1/` remain a historical record and cannot be read by this
store; they are the replay exception to the oracle rule in `CLAUDE.md`.

## Consequence for tests

Replay tests build schema-4/5 stores natively and check ingest idempotence,
contents, deterministic sampling, schema rejection, and transaction failure
recovery. They do not load or regenerate the retired replay-v1 corpus.
