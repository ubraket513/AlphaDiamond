# TrainAlphaDiamond

Local synchronisation root for the Hugging Face storage bucket
`hf://buckets/ubraket513/AlphaDiamond`.

**Nothing under this directory is tracked by Git except this README and
`manifests/`.** The bucket is mutable, non-versioned object storage: it holds
checkpoints, replay/self-play data, logs and intermediate training data. Git
holds source, documentation and the manifest schema.

## Layout

```text
TrainAlphaDiamond/
  README.md                    tracked
  manifests/                   tracked  (schema + release index)
  checkpoints/                 ignored  <family>/<run-id>/iteration-<n>/
  datasets/                    ignored  selfplay/ replay/ fixtures/
  releases/                    ignored  soo/ min/
  logs/                        ignored
  staging/                     ignored
```

## Sync

Run from the repository root with the Hugging Face CLI installed and authenticated.
The Makefile provides build targets only. To upload a run's candidate checkpoints,
use the existing script (replace the example run path and family):

```bash
tools/push_checkpoints.sh --run-dir /absolute/path/to/runs/soo/my-run \
  --bucket ubraket513/AlphaDiamond --family soo --dry-run
tools/push_checkpoints.sh --run-dir /absolute/path/to/runs/soo/my-run \
  --bucket ubraket513/AlphaDiamond --family soo
```

The script uploads checkpoint roots containing `CURRENT` to
`checkpoints/<family>/<run-id>/iteration-<n>/`. It uses `--ignore-existing`,
does not delete remote files, and does not upload replay or logs. Use
`--iteration <n>` to select one iteration. Existing remote files are skipped;
skipping a file does not verify its contents.

To download a run's checkpoints into this local storage root, preview and then
run the same sync without `--dry-run`:

```bash
hf sync hf://buckets/ubraket513/AlphaDiamond/checkpoints/soo/my-run \
  TrainAlphaDiamond/checkpoints/soo/my-run --ignore-existing --dry-run
hf sync hf://buckets/ubraket513/AlphaDiamond/checkpoints/soo/my-run \
  TrainAlphaDiamond/checkpoints/soo/my-run --ignore-existing
```

These commands preserve existing destination files. Validate downloaded native
checkpoint roots with `alphadiamond-checkpoint validate` before using them.

## Immutable paths

`hf sync` overwrites in place and the bucket keeps no history, so a mutable
name is not an identity. Current native runs use transactional checkpoint-v3
roots. `CURRENT` selects a checksummed immutable generation; consumers validate
the manifest and every referenced payload before loading it:

```text
runs/soo/<run-id>/iterations/<iteration>/candidate-checkpoint/CURRENT
runs/soo/<run-id>/iterations/<iteration>/candidate-checkpoint/generations/<generation>/manifest.json
```

Historical Python `.pt` checkpoints remain archival inputs until they have a
validated native deployment export; they are not exact-resume checkpoints.
Release tooling consumes a native checkpoint root or deployment artifact and
verifies its recorded SHA-256 identities before changing release state.

The current native generation manifest is read and validated by
[`native/src/checkpoint.cpp`](../native/src/checkpoint.cpp); use
`alphadiamond-checkpoint validate <checkpoint-root>` to check it.
[`manifests/checkpoint.schema.json`](manifests/checkpoint.schema.json) describes
the historical Python `.pt` sidecar only. It is archival documentation, not the
schema or validator for a native checkpoint-v3 generation.

## Promotion

```bash
build/native-training/native/alphadiamond-release init \
  <native-checkpoint-root> --family soo
build/native-training/native/alphadiamond-release promote \
  <native-checkpoint-root> --to candidate
build/native-training/native/alphadiamond-checkpoint export \
  <native-checkpoint-root> --out <new-deployment-artifact> --version <version>
build/native-training/native/alphadiamond-release validate \
  <new-deployment-artifact>
build/native-training/native/alphadiamond-release promote \
  <native-checkpoint-root> --to promoted \
  --artifact <new-deployment-artifact>
```

States advance from archival to candidate to promoted. Export the native
checkpoint-v3 root separately with `alphadiamond-checkpoint export`, then validate
the resulting deployment artifact. Promotion validates the supplied artifact
for the checkpoint's family and records its model/runtime digests in
`promotion.json`; it does not export a model. Use the artifact exported from
that checkpoint. The recorded checkpoint digest is verified before changing
promotion state; overwriting a checkpoint invalidates measurements tied to it.

## Historical fixture manifest

[manifests/ci-fixtures.json](manifests/ci-fixtures.json) records the original
Gate-D checkpoint path, digest and bucket identity. The regression suites and
GitHub CI workflows that consumed fixtures have been removed. Retain this
manifest and the recorded checkpoint as historical experiment evidence; it is
not a current CI fetch contract.
