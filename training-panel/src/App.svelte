<script>
  import { onMount, tick } from 'svelte';
  import Chart from './Chart.svelte';
  import RollingNumber from './RollingNumber.svelte';
  import ResourceChart from './ResourceChart.svelte';
  import ResourceMonitor from './ResourceMonitor.svelte';
  import Select from './Select.svelte';
  import Tooltip from './Tooltip.svelte';
  import { appendResourceSample, completedLossAverage } from './chart.js';

  let jobs = $state([]);
  let selectedPid = $state(null);
  let presets = $state([]);
  let presetPath = $state('');
  let configText = $state('');
  let runDir = $state(newRunDir('Min'));
  let warmStart = $state('');
  let launcherOpen = $state(false);
  let starting = $state(false);
  let tab = $state('console');
  let error = $state('');
  let launchError = $state('');
  let followLog = $state(true);
  let logElement = $state();
  let speedMetric = $state('samples_per_second');
  let chartStart = $state(null);
  let chartEnd = $state(null);
  let detail = $state(null);
  let resourceView = $state(null);
  let resources = $state(null);
  let resourceHistory = $state([]);
  const resourceSections = {
    processor: { title: 'Processor', color: '#4998e8', metrics: [['usage', 'Usage']] },
    memory: { title: 'Memory', color: '#cb64b7', metrics: [['usage', 'Memory', 'used', 'total'], ['swapUsage', 'Swap', 'swapUsed', 'swapTotal']] },
    gpu: { title: 'GPU', color: '#f16c78', metrics: [['usage', 'Total Usage'], ['encoderUsage', 'Video Encoder Usage'], ['decoderUsage', 'Video Decoder Usage'], ['vramUsage', 'VRAM Usage', 'vramUsed', 'vramTotal']] },
  };
  let refreshing = false;
  let selectionVersion = 0;
  let job = $derived.by(() => {
    const selected = jobs.find((item) => item.pid === selectedPid);
    if (!selected) return undefined;
    const current = detail?.pid === selected.pid && detail.startTicks === selected.startTicks ? detail : null;
    const iterations = current?.iterations ?? [];
    const metrics = iterations.flatMap(({ iteration, stages }) =>
      (stages['train.json']?.training_metrics ?? []).map(metric => ({ ...metric, iteration })))
      .sort((a, b) => a.iteration - b.iteration || a.training_step - b.training_step);
    return { ...selected, ...current, iterations, metrics,
      evaluation: { ...selected.evaluation, ...current?.evaluation } };
  });
  let evaluating = $derived(job?.kind === 'evaluation');
  let summary = $derived(job?.evaluation?.summary);
  let latest = $derived(job?.progress?.metrics ?? job?.metrics.at(-1));
  let throughput = $state(null);
  let previousThroughput = $state(null);
  const throughputUnits = { 'samples/sec': 1, 'samples/min': 60, 'samples/hour': 3600 };
  let throughputUnit = $state('samples/min');
  let throughputFactor = $derived(throughputUnits[throughputUnit]);
  let throughputValue = $derived(Number.isFinite(throughput) ? throughput * throughputFactor : null);
  let throughputChange = $derived(Number.isFinite(previousThroughput)
    ? Math.round(throughput * throughputFactor) - Math.round(previousThroughput * throughputFactor) : null);
  let throughputRun, throughputAt = -Infinity, lastThroughput = null;
  $effect(() => {
    const run = job ? `${job.pid}:${job.startTicks}` : null;
    const value = latest?.samples_per_second;
    const now = performance.now();
    if (run !== throughputRun) {
      throughputRun = run;
      throughputAt = -Infinity;
      throughput = null;
      previousThroughput = null;
      lastThroughput = null;
    }
    if (Number.isFinite(value) && now - throughputAt >= 10000) {
      previousThroughput = lastThroughput;
      throughput = value;
      lastThroughput = value;
      throughputAt = now;
    }
  });
  let lossAverage = $derived.by(() => {
    const recorded = completedLossAverage(job?.metrics ?? []);
    const live = job?.progress?.loss_average;
    return live?.training_step > (recorded?.training_step ?? 0) ? live : recorded;
  });
  let lossChange = $derived(Number.isFinite(lossAverage?.previous_total_loss)
    ? lossAverage.total_loss - lossAverage.previous_total_loss : null);
  let firstStep = $derived(job?.metrics[0]?.training_step ?? 0);
  let lastStep = $derived(job?.metrics.at(-1)?.training_step ?? 0);
  let fromStep = $derived(chartStart ?? firstStep);
  let toStep = $derived(chartEnd ?? lastStep);
  let draft = $derived.by(() => { try { return JSON.parse(configText); } catch { return null; } });
  let latestSelfplay = $derived(job?.iterations.map((item) => item.stages['selfplay.metrics.json']).filter(Boolean).at(-1));
  const number = (n, digits = 0) => n == null ? '—' : new Intl.NumberFormat('en', { maximumFractionDigits: digits }).format(n);
  const pretty = (value) => JSON.stringify(value, null, 2);
  const basename = (path) => path.split('/').filter(Boolean).at(-1);
  let speedOptions = $derived({
    samples_per_second: ['처리량', throughputUnit],
    total_step_seconds: ['업데이트 시간', 'seconds / step'],
    peak_cuda_allocated_bytes: ['GPU 할당 메모리', ''],
  });
  const fieldGroups = [
    { id: 'learning', title: '학습', fields: [
      ['training', 'learning_rate', 'Learning rate'], ['training', 'batch_size', 'Batch size'],
      ['training', 'train_steps_per_iteration', 'Updates / iteration'],
    ] },
    { id: 'selfplay', title: '자기 대국', fields: [
      ['workers', 'games_per_iteration', 'Games / iteration'], ['mcts', 'simulations', 'MCTS simulations'],
      ['workers', 'search_threads', 'Search threads'], ['workers', 'logical_lanes', 'Logical lanes'],
    ] },
    { id: 'limits', title: '실행 제한', fields: [
      ['run_budget', 'max_iterations', 'Iterations'], ['run_budget', 'max_wall_clock_seconds', '시간 제한 (초)'],
    ] },
  ];

  function newRunDir(model) {
    return `artifacts/panel/runs/${model.toLowerCase()}/run-${new Date().toISOString().replace(/[:.]/g, '-')}`;
  }
  async function api(path, body) {
    const response = await fetch(`/api/${path}`, body === undefined ? {} : {
      method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body),
    });
    const data = await response.json();
    if (!response.ok) throw new Error(data.error);
    return data;
  }
  async function refresh() {
    if (refreshing) return;
    refreshing = true;
    let version = selectionVersion;
    try {
      const data = await api('status');
      if (version !== selectionVersion) return;
      jobs = data.jobs;
      resources = data.resources;
      if (resources) resourceHistory = appendResourceSample(resourceHistory, resources);
      if (!jobs.some((item) => item.pid === selectedPid)) {
        selectRun(jobs[0]?.pid ?? null);
      }
      const selected = jobs.find((item) => item.pid === selectedPid);
      version = selectionVersion;
      if (selected) {
        const previous = detail?.pid === selected.pid && detail.startTicks === selected.startTicks ? detail : null;
        const fromIteration = previous?.iterations.at(-1)?.iteration ?? 0;
        const update = await api(`runs/${selected.pid}?fromIteration=${fromIteration}&startTicks=${selected.startTicks}`);
        if (version !== selectionVersion) return;
        detail = { ...update, pid: selected.pid, startTicks: selected.startTicks,
          iterations: [...(previous?.iterations ?? []).filter(item => item.iteration < fromIteration), ...update.iterations] };
      }
      error = '';
      await tick();
      if (followLog && logElement) logElement.scrollTop = logElement.scrollHeight;
    } catch (cause) { if (version === selectionVersion) error = cause.message; }
    finally {
      refreshing = false;
      if (version !== selectionVersion) refresh();
    }
  }
  function zoomCharts([from, to]) {
    const full = from === firstStep && to === lastStep;
    chartStart = full ? null : from;
    chartEnd = full ? null : to;
  }
  function selectRun(pid) {
    selectedPid = pid;
    detail = null;
    selectionVersion++;
    tab = 'console';
    chartStart = null;
    chartEnd = null;
  }
  function choosePreset(path) {
    const preset = presets.find((item) => item.path === path);
    configText = pretty(preset.config);
    presetPath = path;
    runDir = newRunDir(preset.config.model_name);
    launchError = '';
  }
  function editField(section, key, value) {
    const updated = JSON.parse(configText);
    updated[section][key] = value;
    configText = pretty(updated);
  }
  async function start() {
    starting = true;
    launchError = '';
    try {
      const result = await api('runs', { runDir, config: JSON.parse(configText), ...(warmStart.trim() && { warmStart: warmStart.trim() }) });
      selectRun(result.pid);
      resourceView = null;
      launcherOpen = false;
      runDir = newRunDir(draft.model_name);
      await refresh();
    } catch (cause) { launchError = cause.message; }
    finally { starting = false; }
  }
  async function stop(pid) {
    try { await api(`runs/${pid}/stop`, {}); await refresh(); }
    catch (cause) { error = cause.message; }
  }
  function copySettings() {
    configText = pretty(job.config);
    runDir = newRunDir(job.config.model_name);
    launcherOpen = true;
    launchError = '';
  }
  onMount(() => {
    refresh();
    api('configs').then((data) => {
      presets = data.configs.filter((item) => item.config.model_name);
      const preset = presets.find((item) => item.name === 'min-expert-cyclic-local-pilot.json') ?? presets[0];
      if (preset) choosePreset(preset.path);
    }).catch((cause) => { error = cause.message; });
    const timer = setInterval(refresh, 1000);
    return () => clearInterval(timer);
  });
</script>

<div class="workspace">
  <aside class="sidebar">
    <a href="/" class="sidebar-home" aria-label="AlphaDiamond 메인 화면">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="m3 10 9-7 9 7v10a1 1 0 0 1-1 1h-5v-7H9v7H4a1 1 0 0 1-1-1Z" /></svg>
      <span>AlphaDiamond<small>메인 화면</small></span>
    </a>
    <div class="sidebar-heading"><h2>실행 중</h2><span class="count">{jobs.length}</span></div>
    <nav aria-label="실행 중인 훈련과 평가">
      {#each jobs as item (item.pid)}
        <button aria-current={!resourceView && !launcherOpen && item.pid === selectedPid ? 'true' : undefined} class:active={!resourceView && !launcherOpen && item.pid === selectedPid} class="run-item" onclick={() => { resourceView = null; launcherOpen = false; selectRun(item.pid); refresh(); }}>
          <span class="run-model">{item.kind === 'evaluation' ? 'Expert arena' : item.config?.model_name ?? 'Training'} <span class="status-dot"></span></span>
          <strong>{basename(item.runDir)}</strong>
          <span class="run-stage">{item.state?.stage ?? 'STARTING'} <span>{#if item.kind === 'evaluation'}{number(item.evaluation?.summary ? item.evaluation.summary.completed_matches + item.evaluation.summary.aborted_matches : null)} / {number(item.evaluation?.summary?.scheduled_matches)}{:else}#{number((item.state?.iteration ?? 0) + 1)}{/if}</span></span>
          <small>PID {item.pid}</small>
        </button>
      {:else}<p class="sidebar-empty">실행 중인 훈련·평가가 없습니다.</p>{/each}
    </nav>
    <nav class="resource-nav" aria-label="리소스 모니터">
      {#each Object.entries(resourceSections) as [kind, section]}
        <button class="resource-item" class:active={resourceView === kind} aria-current={resourceView === kind ? 'page' : undefined} onclick={() => { resourceView = kind; launcherOpen = false; }}>
          <span class="resource-nav-title">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" aria-hidden="true">
              {#if kind === 'processor'}<rect x="5" y="5" width="14" height="14" rx="2" /><rect x="9" y="9" width="6" height="6" /><path d="M9 1v4m6-4v4M9 19v4m6-4v4M1 9h4m-4 6h4m14-6h4m-4 6h4" />
              {:else if kind === 'memory'}<rect x="2" y="6" width="20" height="12" rx="2" /><path d="M6 9v6m4-6v6m4-6v6m4-6v6M6 18v3m4-3v3m4-3v3m4-3v3" />
              {:else}<rect x="3" y="5" width="18" height="13" rx="2" /><circle cx="12" cy="11.5" r="3.5" /><path d="M7 18v3m4-3v3m4-3v3M1 5h2" />{/if}
            </svg>
            {section.title}
            <small>{Number.isFinite(resources?.[kind]?.usage) ? `${resources[kind].usage.toFixed(0)}%` : '—'}</small>
          </span>
          <ResourceChart history={resourceHistory} {kind} color={section.color} label={`${section.title} 사용률`} compact />
        </button>
      {/each}
    </nav>
    <div class="sidebar-bottom">
      <button onclick={() => { resourceView = null; launcherOpen = !launcherOpen; }} aria-expanded={launcherOpen}><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 5v14M5 12h14" /></svg>새 훈련</button>
    </div>
  </aside>

  <main>
    {#if error}<div class="error" role="alert">{error}</div>{/if}
    {#if resourceView}
      <ResourceMonitor kind={resourceView} section={resourceSections[resourceView]} history={resourceHistory} current={resources?.[resourceView]} />
    {:else if launcherOpen}
      <section class="launcher">
        <div class="section-heading"><h1>훈련 시작</h1><Tooltip id="close-tooltip" text="닫기"><button class="icon-button quiet" aria-label="닫기" aria-describedby="close-tooltip" onclick={() => { launcherOpen = false; }}><svg viewBox="0 0 24 24" aria-hidden="true"><path d="m6 6 12 12M6 18 18 6" /></svg></button></Tooltip></div>
        <form onsubmit={(event) => { event.preventDefault(); start(); }}>
          <section class="settings-section" aria-labelledby="launch-basics">
            <h2 id="launch-basics">기본 설정</h2>
            <div class="settings-section-body launch-paths">
              <label for="preset-select">설정 프리셋<Select id="preset-select" label="설정 프리셋" value={presetPath} options={presets.map(preset => ({ value: preset.path, label: preset.name }))} onchange={choosePreset} /></label>
              <label>새 훈련 폴더<input required bind:value={runDir} spellcheck="false" /></label>
              <label class="full">Warm-start 모델 경로 <span class="muted">선택 · 비워두면 scratch</span><input bind:value={warmStart} placeholder="models/min/2.0.1 또는 deployment export 경로" spellcheck="false" /></label>
            </div>
          </section>
          {#if draft}
            {#each fieldGroups as group}
              <section class="settings-section" aria-labelledby={`launch-${group.id}`}>
                <h2 id={`launch-${group.id}`}>{group.title}</h2>
                <div class="settings-section-body settings-grid">
                  {#each group.fields as [section, key, label]}
                    {#if draft[section] && key in draft[section]}<label>{label}<input type="number" step="any" value={draft[section][key]} placeholder={section === 'run_budget' ? '제한 없음' : undefined} onchange={(event) => editField(section, key, event.currentTarget.valueAsNumber)} /></label>{/if}
                  {/each}
                  {#if group.id === 'selfplay' && draft.self_play?.turn_order_group}<label for="turn-order-select">Turn-order group<Select id="turn-order-select" label="Turn-order group" value={draft.self_play.turn_order_group} options={['all', 'cyclic', 'reverse'].map(value => ({ value, label: value }))} onchange={value => editField('self_play', 'turn_order_group', value)} /></label>{/if}
                </div>
              </section>
            {/each}
            {#if draft.runtime}
              <section class="settings-section" aria-labelledby="launch-runtime">
                <h2 id="launch-runtime">실행 환경</h2>
                <div class="settings-section-body settings-grid">
                  <label>Device<input value={draft.runtime.device} onchange={(event) => editField('runtime', 'device', event.currentTarget.value)} /></label>
                  <label for="precision-select">Precision<Select id="precision-select" label="Precision" value={draft.runtime.precision} options={['fp32', 'fp16', 'bf16'].map(value => ({ value, label: value }))} onchange={value => editField('runtime', 'precision', value)} /></label>
                </div>
              </section>
            {/if}
          {/if}
          <details class="config-editor"><summary>전체 설정 JSON</summary><label><span class="sr-only">전체 훈련 설정 JSON</span><textarea bind:value={configText} aria-invalid={!draft} aria-describedby={!draft ? 'config-json-error' : undefined} spellcheck="false" rows="20"></textarea></label>{#if !draft}<p class="field-error" id="config-json-error" role="alert">JSON 형식을 확인해 주세요.</p>{/if}</details>
          {#if launchError}<p class="error" role="alert">{launchError}</p>{/if}
          <div class="launch-footer"><button class="primary" type="submit" disabled={starting || !draft}><svg viewBox="0 0 24 24" aria-hidden="true"><path d="m8 5 11 7-11 7Z" /></svg>{starting ? '시작 중…' : '훈련 시작'}</button></div>
        </form>
      </section>
    {:else if job}
      <div class="run-heading">
        <div><span class="eyebrow">{#if evaluating}Min · Expert arena{:else}{job.config?.model_name} · {job.config?.self_play?.turn_order_group}{/if}</span><h1>{basename(job.runDir)}</h1></div>
        <div class="run-actions">
          {#if !evaluating}<Tooltip id="copy-tooltip" text="설정 복사"><button class="icon-button quiet" aria-label="설정 복사" aria-describedby="copy-tooltip" onclick={copySettings}><svg viewBox="0 0 24 24" aria-hidden="true"><rect x="9" y="9" width="12" height="12" rx="2" /><path d="M5 15H4a1 1 0 0 1-1-1V4a1 1 0 0 1 1-1h10a1 1 0 0 1 1 1v1" /></svg></button></Tooltip>{/if}
          <Tooltip id="stop-tooltip" text="즉시 중지"><button class="icon-button danger" aria-label="즉시 중지" aria-describedby="stop-tooltip" onclick={() => stop(job.pid)}><svg viewBox="0 0 24 24" aria-hidden="true"><rect x="5" y="5" width="14" height="14" rx="1" /></svg></button></Tooltip>
        </div>
      </div>

      <section class="run-status" aria-label="현재 실행 상태">
        <div class="current-stage"><span class="status-label">현재 단계</span><strong><i class="status-dot"></i>{job.state?.stage ?? 'STARTING'}</strong></div>
        {#if evaluating}
          <div><span class="status-label">평가 경기 · 완료 + 중단</span><strong>{number(summary ? summary.completed_matches + summary.aborted_matches : null)}<small> / {number(summary?.scheduled_matches)}</small></strong></div>
        {:else}
          <div><span class="status-label">반복</span><strong>{number((job.state?.iteration ?? 0) + 1)}<small>{job.config?.run_budget?.max_iterations ? ` / ${number(job.config.run_budget.max_iterations)}` : ' · 제한 없음'}</small></strong></div>
          <div><span class="status-label">누적 학습 스텝</span><strong>{number(job.progress?.metrics?.training_step ?? job.state?.training_step)}</strong></div>
        {/if}
        {#if job.progress?.total > 0}
          <div class="stage-progress">
            <span class="status-label">{job.state.stage === 'TRAIN' ? '학습 업데이트' : '자기 대국 처리 · 완료·중단'}</span>
            <strong>{number(job.progress.completed)}<small> / {number(job.progress.total)} · {number(job.progress.completed / job.progress.total * 100, 1)}%</small></strong>
            <progress value={job.progress.completed} max={job.progress.total} aria-label="현재 단계 진행률"></progress>
          </div>
        {/if}
        <div class="runtime"><span>{#if !evaluating}{job.config?.runtime?.device} · {job.config?.runtime?.precision} · {/if}PID {job.pid}</span>{#if job.startedAt}<span>시작 {new Date(job.startedAt).toLocaleTimeString('ko-KR')}</span>{/if}</div>
      </section>

      {#if evaluating}
      <div class="stats evaluation-stats">
        <div><span>기록 / 예정</span><strong>{number(job.evaluation.matches?.length)}<small> / {number(summary?.scheduled_matches)}</small></strong></div>
        <div><span>완료</span><strong>{number(summary?.completed_matches)}</strong></div>
        <div><span>중단</span><strong>{number(summary?.aborted_matches)}</strong></div>
        <div><span>대기</span><strong>{number(summary?.pending_matches)}</strong></div>
        <div><span>경과 시간 <small>최근 기록</small></span><strong>{number(summary?.elapsed_seconds, 1)}<small> 초</small></strong></div>
      </div>
      {:else}
      <div class="stats">
        <div>
          <span>최근 손실 <small>1,000스텝 평균</small></span>
          <strong class="metric-value" title={lossAverage ? `학습 스텝 ${number(lossAverage.training_step - 999)}–${number(lossAverage.training_step)}` : '완료된 1,000스텝 구간 없음'}>
            <RollingNumber value={lossAverage?.total_loss} digits={4} />
            {#if lossChange !== null}<span class="metric-change" class:up={lossChange > 0} class:down={lossChange < 0} aria-label={`직전 1,000스텝 평균 대비 ${Math.abs(lossChange).toFixed(4)}포인트 ${lossChange > 0 ? '상승' : lossChange < 0 ? '하락' : '변동 없음'}`}>{lossChange === 0 ? '0.0000' : `${lossChange > 0 ? '↑' : '↓'} ${Math.abs(lossChange).toFixed(4)}`}</span>{/if}
          </strong>
        </div>
        <div>
          <span>학습 처리량</span>
          <div class="metric-value throughput-value">
            <strong>{#key throughputUnit}<RollingNumber value={throughputValue} />{/key}</strong>
            <button class="throughput-unit" type="button" aria-label={`처리량 단위: ${throughputUnit}`} onclick={() => {
              const units = Object.keys(throughputUnits);
              throughputUnit = units[(units.indexOf(throughputUnit) + 1) % units.length];
            }}>{throughputUnit}</button>
            {#if throughputChange !== null}<span class="metric-change" class:up={throughputChange > 0} class:down={throughputChange < 0} aria-label={`직전 10초 표시값 대비 ${number(Math.abs(throughputChange))} ${throughputUnit} ${throughputChange > 0 ? '상승' : throughputChange < 0 ? '하락' : '변동 없음'}`}>{throughputChange === 0 ? '0' : `${throughputChange > 0 ? '↑' : '↓'} ${number(Math.abs(throughputChange))}`}</span>{/if}
          </div>
        </div>
        <div><span>자기 대국 <small>최근 완료 단계</small></span><strong><RollingNumber value={latestSelfplay?.games.completed} /><small> / {number(latestSelfplay?.games.requested)}</small></strong></div>
      </div>

      <div class="charts">
        <Chart metrics={job.metrics} keys={['total_loss', 'policy_loss', 'value_loss']} labels={['전체', '정책', '가치']} title="학습 손실" {fromStep} {toStep} minStep={firstStep} maxStep={lastStep} onzoom={zoomCharts} />
        <div class="speed-chart"><div class="metric-select"><Select id="chart-metric" label="차트 지표" bind:value={speedMetric} options={Object.entries(speedOptions).map(([value, option]) => ({ value, label: option[0] }))} /></div><Chart metrics={job.metrics} keys={[speedMetric]} labels={[speedOptions[speedMetric][0]]} title="학습 성능" unit={speedOptions[speedMetric][1]} scale={speedMetric === 'samples_per_second' ? throughputFactor : 1} bytes={speedMetric === 'peak_cuda_allocated_bytes'} {fromStep} {toStep} minStep={firstStep} maxStep={lastStep} onzoom={zoomCharts} /></div>
      </div>
      {/if}

      <section class="detail-panel">
        <div class="detail-toolbar">
          <div class="tabs" role="tablist" aria-label="실행 세부 정보">{#each (evaluating ? [['console', '콘솔'], ['metadata', '전체 메타데이터']] : [['console', '콘솔'], ['config', '실행 설정'], ['metadata', '전체 메타데이터']]) as [key, label]}<button role="tab" aria-selected={tab === key} class:selected={tab === key} onclick={() => { tab = key; }}>{label}</button>{/each}</div>
          {#if tab === 'console'}
            {#if job.logAvailable}<Tooltip id="console-tooltip" text="전체 콘솔 열기"><a class="icon-button" aria-label="전체 콘솔 열기" aria-describedby="console-tooltip" href={`/api/runs/${job.pid}/log?startTicks=${job.startTicks}`} target="_blank" rel="noreferrer"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M14 3h7v7m0-7L10 14M10 3H5a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-5" /></svg></a></Tooltip>{/if}
            <label class="follow"><input type="checkbox" bind:checked={followLog} />자동 스크롤</label>
          {/if}
        </div>
        {#if tab === 'console'}
          {#if job.log}<pre class="console" bind:this={logElement}>{job.log}</pre>{:else}<p class="console-empty">출력 없음</p>{/if}
        {:else if tab === 'config'}<pre>{pretty(job.config)}</pre>
        {:else}<pre>{pretty(evaluating ? { state: job.state, tf32Requests: job.tf32Requests, evaluation: job.evaluation } : { state: job.state, tf32Requests: job.tf32Requests, iterations: job.iterations })}</pre>{/if}
      </section>
      <details class="execution-details">
        <summary>실행 정보 · 경로, TF32, 명령</summary>
        <p class="path">{job.runDir}</p>
        <p class="cadence">TF32 환경 요청 (/proc): {#if job.tf32Requests}TORCH_ALLOW_TF32_CUBLAS_OVERRIDE={job.tf32Requests.TORCH_ALLOW_TF32_CUBLAS_OVERRIDE ?? '미설정'} · NVIDIA_TF32_OVERRIDE={job.tf32Requests.NVIDIA_TF32_OVERRIDE ?? '미설정'}{:else}확인 중 또는 읽기 불가{/if}. 실제 커널 사용 여부를 뜻하지 않습니다.</p>
        <p class="command"><code>{Array.isArray(job.command) ? job.command.join(' ') : job.command}</code></p>
      </details>
    {:else}
      <section class="welcome"><h1>실행 중인 작업이 없습니다</h1><p>실행 중인 훈련의 설정, 로그와 지표를 확인하세요.<br />CLI에서 시작한 작업도 자동으로 연결됩니다.</p><button class="primary" onclick={() => { launcherOpen = true; }}>+ 새 훈련 시작</button></section>
    {/if}
  </main>
</div>
