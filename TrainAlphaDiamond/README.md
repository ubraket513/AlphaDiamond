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
  checkpoints/                 ignored  soo/<run-id>/step-<n>/
  datasets/                    ignored  selfplay/ replay/ fixtures/
  releases/                    ignored  soo/ min/
  logs/                        ignored
  staging/                     ignored
```

## Sync

```bash
make data-push-dry-run    # show what a push would change
make data-push
make data-pull-dry-run
make data-pull
```

No target passes `--delete`. Deleting bucket content is a separate, manual
operation performed after reviewing a dry-run plan.

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

Manifest fields: see [manifests/checkpoint.schema.json](manifests/checkpoint.schema.json).

## Promotion

```bash
build/native-training/native/alphadiamond-release init \
  <native-checkpoint-root> --family soo
build/native-training/native/alphadiamond-release promote \
  <native-checkpoint-root> --to candidate
build/native-training/native/alphadiamond-release promote \
  <native-checkpoint-root> --to promoted \
  --artifact <validated-deployment-artifact>
```

States move one step at a time, because `candidate` is where conversion is
gated. Promotion converts: it exports the deployment artifact and writes its
digests back into the manifest, so a `promoted` checkpoint whose artifact was
never built cannot exist. The checkpoint's own digest is verified first --
these paths are immutable, and one that was overwritten invalidates every
measurement recorded against it.

## Historical fixture manifest

[manifests/ci-fixtures.json](manifests/ci-fixtures.json) records the original
Gate-D checkpoint path, digest and bucket identity. The regression suites and
GitHub CI workflows that consumed fixtures have been removed. Retain this
manifest and the recorded checkpoint as historical experiment evidence; it is
not a current CI fetch contract.
