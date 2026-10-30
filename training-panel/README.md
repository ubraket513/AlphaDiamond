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
at stage boundaries; console output appears as the CLI writes it. New trainer
builds flush stage start/completion and periodic progress lines to stderr while
keeping stdout reserved for the final command JSON. The panel polls
every second, with one refresh in flight at a time. `/api/status` reads only
the active jobs' state/configuration, progress or evaluation summary. The selected job's
`/api/runs/:pid?fromIteration=N` response supplies detailed metadata and the most
recent 64 KiB of console output. The browser retains older iteration metadata and
re-reads the latest seen iteration to pick up new stage files; selecting another
job starts a fresh detail read. Full console output remains available through
the **전체 콘솔 열기** link (`/api/runs/:pid/log`), streamed directly from the file.
No logs or metadata are deleted. Completed or stopped processes leave the active list.
Settings and configuration copying use `active-config.json` after an accepted
resume transition; runs without a transition use their initial `resolved-config.json`.

New trainer builds also write `progress.json` through the existing atomic JSON
writer about once per second and at completion. It reports processed self-play
games (completed plus aborted) or completed learner updates, plus the latest
training metrics. The panel uses its existing polling to show the current-stage
count, percentage, latest loss, throughput and cumulative training step. Progress
must match the active PID, stage and iteration; stale files are ignored. Full
chart history still comes from completed stage reports. Existing running binaries
need a later restart with the new build to emit this file.

**Stop immediately sends SIGKILL** to the selected training or evaluation process, or its
process group when it leads that group. The in-flight stage stops immediately;
earlier durable checkpoints remain in the run directory. An external supervisor
should exit on a stopped child rather than automatically relaunch it.

For development, keep `npm start` running and run `npm run dev` in a second
terminal; open <http://127.0.0.1:5173>. The Vite server proxies `/api` to port
5178. Use `npm run build` and inspect the running panel directly for development
validation.

The panel leads with the current stage, iteration and cumulative training steps.
Run paths, TF32 requests and the native command are under **실행 정보**.
Charts use Chart.js with hover values, clickable legends and subtle gradient fills
that fade to transparent beneath each line. Large numeric
series use arithmetic means over 150 equal training-step intervals in the visible
range; zooming recomputes the averages and missing values remain gaps. Tooltips
show interval averages rather than individual measurements. The underlying
metrics are retained. Scrolling the mouse wheel over either plot zooms both plots around the cursor
to the same training-step interval. GPU memory
uses adaptive SI units (KB, MB, GB, etc.) on axes and tooltips. The light layout
uses the PhX-UI shell as a visual reference: a 240px gray job rail, white
workspace and sections separated by thin rules instead of cards. The rail's
home link opens the main screen; there is no separate top title bar.
The new-training action sits at the bottom of the desktop rail; mobile keeps
jobs in a horizontal list. This is local Svelte markup and CSS, with no React or
PhX runtime dependency. `npm test` checks chart data and memory formatting.
Pretendard (400, 500, 600 and 700) is served locally from `public/fonts`, including
its SIL Open Font License. Body text, key numbers and chart labels use the same
family; console output and technical identifiers retain a monospace font.
Secondary instructions, chart record counts, footer status and local-connection
status are omitted. Copy settings, stop, close and open-console actions use icons
with accessible names and tooltips. There is no full-range reset button.

Processor, Memory and GPU entries in the sidebar open live resource monitors.
The existing one-second status poll reads system-wide CPU counters from
`/proc/stat`, available memory and swap from `/proc/meminfo`, and the first
NVIDIA GPU from `nvidia-smi`. GPU query failures show unavailable values rather
than zero usage. GPU MiB values are converted to bytes before display formatting.
The browser keeps 120 samples (about two minutes), initializes the plots with
a zero baseline, and adds new measurements without replaying chart animations.
The numeric readouts always use actual measurements. History is
kept while the page is open and resets on reload; no monitoring service or new
dependency is required.
Processor can switch to individual logical CPU graphs with current frequencies.
Memory properties show populated slots, configured speed and module type from
the system's DMI/udev properties. GPU details include encoder/decoder activity,
graphics and memory clocks, power draw/limit and temperature. Unsupported or
unreadable hardware properties display an em dash; memory properties are read
once per server process, while usage and sensors refresh every second.
Key training numbers briefly flash green on increases or red on decreases,
hold their color until every reel stops, then fade to the normal color over 300 ms. They use digit reels with CSS transitions; reduced-motion
preferences disable the animation. No animation dependency is needed.

Throughput defaults to samples/min. Clicking its unit cycles through samples/sec,
samples/min and samples/hour, immediately converting the number, comparison badge,
chart axis and tooltips while preserving native samples/s measurements. The throughput number refreshes every 10 seconds
while resource monitoring and native progress still poll every second. Cumulative
training steps display immediately without animation. Other digit reels slow to
a stop over roughly 2–4 seconds, with the later digits settling last and the
increase/decrease color held through the full roll before a 300ms fade.
Recent loss displays the mean of the latest complete
global 1,000-step block (1–1,000, 1,001–2,000, etc.), holding that value until the
next block completes. Its badge shows the point difference from the immediately
preceding complete block (green up/red down); missing predecessors have no badge.
The trainer includes these as `progress.loss_average` and its optional
`previous_total_loss` field;
completed stage records also supply historical block means. An already-running
older trainer needs a safe restart after its TRAIN checkpoint to emit live means;
its uncheckpointed updates must not be discarded just to change this display.

All dropdowns use one small Svelte `Select` backed by the native Popover API,
with PhX-style translucent menus and blur. Keyboard selection, Escape, Tab,
outside dismissal and viewport positioning are handled in the component.
The 8px rounded progress track and solid blue fill follow Astryx; reduced motion
disables the fill transition. Settings inputs use PhX focus/error states, icon
tooltips work on hover and keyboard focus and dismiss on Escape, and selected
tabs have the blue Astryx underline. No React or additional UI dependency is required.

See the [code guide](../docs/development/code-guide.md) for native entrypoints and
the [documentation index](../docs/README.md) for current guides and historical records.
