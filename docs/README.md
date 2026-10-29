# Documentation

Read the current guides in this order:

1. [Repository starting point](../README.md): folders, build and product entrypoints.
2. [Code guide](development/code-guide.md): where responsibilities and functions live.
3. [Build cycle](development/build-cycle.md): presets, output locations and resource limits.
4. [Engine and Desktop ownership](architecture/products.md) and [SDK upgrades](architecture/repository_split.md).
5. [Model artifact v3](architecture/model_artifact_v3.md), [goal-aware training](model-training/goal-aware-training.md) and [Min specialists](model-training/min_turn_order_experts.md).
6. [Training panel](../training-panel/README.md): current configuration selection, launching and monitoring.

For the current large Min experiment, see [Min 512 by 12 local training](model-training/min_512x12_local.md)
for heuristic-removal readiness, CPU inference measurements and continuation settings.

`architecture/` contains product contracts and decisions. `development/` contains
current contributor guidance. `model-training/` combines current operating guides
with experiment analyses; dated result sections retain their recorded settings
and measurements.

## Historical documentation

`archive/` contains records whose paths, commands, test counts and measurements
reflect the session in which they were written. They are evidence rather than
current setup instructions. Start from the current build guide when reproducing
a result against today's source tree.

| Collection | Content |
|---|---|
| [plans](archive/plans/) | Implementation plans and execution notes |
| [specs](archive/specs/) | Historical design proposals |
| [reports](archive/reports/) | Dated experiments, handoffs and the Min transition roadmap |
| [inventories](archive/inventories/) | Python migration inventories and ledgers |
| [selfplay](archive/selfplay/) | Native self-play migration and Windows handoffs |
| [performance](archive/performance/) | Profiling, GPU and throughput records |
| [storage](archive/storage/) | Former TrainAlphaDiamond storage guide and historical manifests |

Raw historical data lives in the separate root [`archive/`](../archive/README.md).
Current local runs and optional downloads belong under `artifacts/`, not in a
documentation collection. See the [storage guide](archive/storage/README.md) for
the role of the existing Hugging Face bucket.
