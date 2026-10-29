# Training panel

A small local Svelte panel for the native training CLI. Node 22.12+ is required.

```sh
cd training-panel
npm ci
npm run build
npm start
```

Open <http://127.0.0.1:5178>. The default binary is
`build/training-release/bin/alphadiamond-train` in the Engine checkout.
Set `ALPHADIAMOND_TRAIN_BIN=/absolute/path/to/alphadiamond-train` or `PORT` when
starting the server to select another binary or port. Keep the panel local.

The preset list reads all seven files in `configs/current/`: the three Min expert
local pilots, the Min `min-512x12-3h.json` scratch configuration, and the Soo
bootstrap, production and six-hour configurations. The larger Min uses width
512, 12 residual blocks and a three-hour iteration-boundary budget; an external
process cap is required for a strict wall-clock limit.
Earlier Min configurations and the CLI-only expert arena configuration remain
in `configs/experiments/`; they are excluded from the training picker.
The move preserves every configuration's contents. Dated experiment reports
retain their original paths and recorded identities.

Choose a preset, edit common settings or the complete JSON, supply a new run
directory, and start. The run directory's parent must be `min` or `soo`, as
required by the native CLI. An empty warm-start path uses `--scratch`;
otherwise the path is passed as `--warm-start`. Native startup errors appear
in the form. The CLI validates the configuration.

New panel-launched trainers inherit the server's environment, including an explicit
`TORCH_ALLOW_TF32_CUBLAS_OVERRIDE` value. When unset, the panel defaults that request
to `1`; start the server with `TORCH_ALLOW_TF32_CUBLAS_OVERRIDE=0` to preserve a zero
request for new launches. CLI and Agent launches use their own environment.
The selected job shows its `/proc` TF32 environment requests, including
`NVIDIA_TF32_OVERRIDE`. These requests do not prove actual TF32 kernel use.
Native checkpoint `environment_json` records the effective cuBLAS/cuDNN allow
flags as well as the environment requests; those flags describe permission,
not whether a particular kernel used TF32.

The three Min expert local-pilot presets use 24 search threads, 256 lanes,
batch cap 128 and a 100µs batch wait. On this RTX 5080 machine, full ABBA runs
with the canonical Min opening and cyclic checkpoint improved completed samples/s
from 123.5608 to 171.2915 (+38.63%) with TF32 and 256 lanes versus FP32 and
128 lanes; both settings aborted 3/768 games. Reverse and shared-control reuse
these hardware settings as an extrapolation; they were not separately validated.
This measures self-play throughput, not playing strength. Learner batch size and
training budgets are unchanged.

Active CLI and Agent jobs appear automatically through Linux process discovery.
The list also includes active `min_expert_arena --out PATH` evaluations, with
relative output paths resolved against the process working directory. Evaluation
views show recorded/scheduled, completed, aborted and pending match counts, plus
the native invocation, schedule, summary and match metadata. Progress snapshots
update after each complete turn-order group; individual games do not stream into
the panel. Evaluation console output comes from its actual stdout destination
when that is a regular file, even outside the output directory. Pipes are never
read. Evaluation launching remains with the existing CLI or Agent workflow.

To retain live output for a job launched from a terminal or Agent, redirect both
output streams into `console.log` inside its run directory:

```sh
RUN_DIR="$PWD/artifacts/example/runs/min/experiment-1"
mkdir -p "$RUN_DIR"
TORCH_ALLOW_TF32_CUBLAS_OVERRIDE=1 OMP_NUM_THREADS=2 MKL_NUM_THREADS=2 \
  build/training-release/bin/alphadiamond-train train \
  --run-dir "$RUN_DIR" --config configs/current/min-expert-cyclic-local-pilot.json \
  --warm-start /absolute/path/to/deployment-export >"$RUN_DIR/console.log" 2>&1
```

The browser and server can be closed without stopping a job. State, settings,
iteration metadata and loss samples come directly from native files. They update
at stage boundaries; console output appears as the CLI writes it. The panel polls
every 1.5 seconds, with one refresh in flight at a time. `/api/status` reads only
the active jobs' state/configuration or evaluation summary. The selected job's
`/api/runs/:pid?fromIteration=N` response supplies detailed metadata and the most
recent 64 KiB of console output. The browser retains older iteration metadata and
re-reads the latest seen iteration to pick up new stage files; selecting another
job starts a fresh detail read. Full console output remains available through
the **전체 콘솔 열기** link (`/api/runs/:pid/log`), streamed directly from the file.
No logs or metadata are deleted. Completed or stopped processes leave the active list.
Settings and configuration copying use `active-config.json` after an accepted
resume transition; runs without a transition use their initial `resolved-config.json`.

**Stop immediately sends SIGKILL** to the selected training or evaluation process, or its
process group when it leads that group. The in-flight stage stops immediately;
earlier durable checkpoints remain in the run directory. An external supervisor
should exit on a stopped child rather than automatically relaunch it.

For development, keep `npm start` running and run `npm run dev` in a second
terminal; open <http://127.0.0.1:5173>. The Vite server proxies `/api` to port
5178. Use `npm run build` and inspect the running panel directly for development
validation. The previous bridge and chart regression suites have been removed.

See the [code guide](../docs/development/code-guide.md) for native entrypoints and
the [documentation index](../docs/README.md) for current guides and historical records.
