# Training Panel and Min Autopilot Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Deliver a small working local training panel while training and evaluating stronger Min candidates.

**Architecture:** Svelte consumes one thin Node HTTP bridge. The bridge discovers native processes and reads their existing output files. Min uses the existing native training and arena tools independently of the browser.

**Tech Stack:** Svelte 5, Vite, Node built-in HTTP/test/fs/process APIs, native SVG, existing C++/LibTorch training.

**Spec:** `docs/superpowers/specs/2026-10-29-training-panel.md`

## Global Constraints

- Work on local branches only; preserve original changes and artifacts.
- Minimal dependencies and code; no failed history, termination reasons, agent notifications, registry, fallback or retry framework.
- Immediate process termination on Stop; Agent-launched CLI jobs visible without registration.
- User autonomy expires 2026-10-29 16:20:02 KST; heartbeat continues only within this window.
- Use Serena, Graphify, Context7; refresh graph after changes.
- GCC, mold, sccache, Ninja; memory-safe parallel builds and tests; development O0/g1/pipe, no LTO; optimized Release for production training.
- No shipped-model replacement or external publishing.

## Review Focus

- Native jobs launched outside the panel remain discoverable after browser/server restart.
- Missing in-progress stage output is normal; completed metrics use native field names and cadence.
- Stop targets only the selected training and is immediate, including when it shares a supervisor process group.
- All configuration values remain editable without duplicating the native schema.
- Strength claims use arena evidence against the exact shipped baseline, with incomplete games and uncertainty visible.

### Task 1: Native CLI bridge

**Files:** Create `training-panel/server.mjs`, `training-panel/server.test.mjs`.
**Interfaces:** Implement the exact HTTP contract in the spec. The root agent owns package/Vite/frontend files. Training agent owns experiment files; coordinate log and metric shapes without modifying native source.

- [ ] Read relevant CLI/source relationships using Graphify and Serena; inspect references before shared-interface changes.
- [ ] Write failing focused Node tests for independently launched process discovery, stage metrics, CLI launch arguments/output and immediate stop.
- [ ] Implement the minimal bridge with built-in Node APIs and static serving of `dist/`.
- [ ] Run Node tests with independent tests parallelized; report evidence and commit only owned files.

### Task 2: Svelte panel

**Files:** Create `training-panel/package.json`, lockfile, `vite.config.js`, `index.html`, `src/main.js`, `src/App.svelte`, `src/app.css`, focused chart helper/test if necessary, and README.
**Interfaces:** Consume Task 1 API without changing its contract. Use existing configuration JSON and actual native metrics.

- [ ] Query current Svelte/Vite APIs with Context7; apply the agreed minimal layout and settings/log/chart requirements.
- [ ] Implement active-run selection, settings/start form, loss/throughput charts, logs and native metadata inspection.
- [ ] Build, run focused data transformation tests where meaningful, and verify actual UI in a browser using a separate fixture process.
- [ ] Document one-command use and Agent CLI integration; commit owned files.

### Task 3: Min training and strength evaluation

**Files:** Dedicated ignored `artifacts/min-strength-20261029/` configs, logs, checkpoints and evaluation output; report in the plan ledger.

- [ ] Build optimized CUDA training binaries with the required toolchain and resource budget.
- [ ] Freeze exact old specialist pair and shipped migrated baseline identities. Launch new specialist continuations into dedicated runs, with no automatic continuation after user Stop.
- [ ] Estimate throughput and reserve time for fixed before/after paired arenas and independent held-out evaluation if justified.
- [ ] Validate durable checkpoints; report actual training budgets and arena results/uncertainty without claiming strength from loss.

### Task 4: Integration and review

**Files:** Update root README and Graphify output; reports in `.superpowers/sdd/2026-10-29-training-panel/`.

- [ ] Run backend tests, frontend build, browser contract checks and local migration verification.
- [ ] Obtain independent spec/code review of the whole panel; fix concrete findings and reverify affected checks.
- [ ] Refresh Graphify and record commits, local run URL, active training and deadline/heartbeat state.
- [ ] At deadline, retain evidence, stop session-owned remaining compute, report actual results and pause heartbeat.
