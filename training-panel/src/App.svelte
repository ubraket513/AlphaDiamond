<script>
  import { onMount, tick } from 'svelte';
  import Chart from './Chart.svelte';

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
  let lastUpdate = $state(null);
  let followLog = $state(true);
  let logElement = $state();
  let speedMetric = $state('samples_per_second');
  let job = $derived(jobs.find((item) => item.pid === selectedPid));
  let latest = $derived(job?.metrics.at(-1));
  let draft = $derived.by(() => { try { return JSON.parse(configText); } catch { return null; } });
  let latestSelfplay = $derived(job?.iterations.map((item) => item.stages['selfplay.metrics.json']).filter(Boolean).at(-1));
  const number = (n, digits = 0) => n == null ? '—' : new Intl.NumberFormat('en', { maximumFractionDigits: digits }).format(n);
  const pretty = (value) => JSON.stringify(value, null, 2);
  const basename = (path) => path.split('/').filter(Boolean).at(-1);
  const speedOptions = {
    samples_per_second: ['처리량', 'samples / s'],
    total_step_seconds: ['업데이트 시간', 'seconds / step'],
    peak_cuda_allocated_bytes: ['GPU 할당 메모리', 'bytes'],
  };
  const fields = [
    ['training', 'learning_rate', 'Learning rate'], ['training', 'batch_size', 'Batch size'],
    ['training', 'train_steps_per_iteration', 'Updates / iteration'], ['workers', 'games_per_iteration', 'Games / iteration'],
    ['mcts', 'simulations', 'MCTS simulations'], ['run_budget', 'max_iterations', 'Iterations'],
    ['run_budget', 'max_wall_clock_seconds', '시간 제한 (초)'], ['workers', 'search_threads', 'Search threads'],
    ['workers', 'logical_lanes', 'Logical lanes'],
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
    try {
      const data = await api('status');
      jobs = data.jobs;
      if (!jobs.some((item) => item.pid === selectedPid)) selectedPid = jobs[0]?.pid ?? null;
      lastUpdate = new Date();
      error = '';
      await tick();
      if (followLog && logElement) logElement.scrollTop = logElement.scrollHeight;
    } catch (cause) { error = cause.message; }
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
      selectedPid = result.pid;
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
    const timer = setInterval(refresh, 1500);
    return () => clearInterval(timer);
  });
</script>

<header class="topbar">
  <a href="/" class="brand">AlphaDiamond <span>TRAINING</span></a>
  <div class="header-actions"><span class="live"><i style:background={error ? '#ffaaa9' : '#79dfb5'}></i>{error ? '연결 오류' : '로컬 연결'}</span><button class="primary" onclick={() => { launcherOpen = !launcherOpen; }}>+ 새 훈련</button></div>
</header>

<div class="workspace">
  <aside class="sidebar">
    <div class="sidebar-heading"><h2>실행 중</h2><span class="count">{jobs.length}</span></div>
    <nav aria-label="실행 중인 훈련">
      {#each jobs as item (item.pid)}
        <button class:active={item.pid === selectedPid} class="run-item" onclick={() => { selectedPid = item.pid; tab = 'console'; }}>
          <span class="run-model">{item.config?.model_name ?? 'Training'} <span class="status-dot"></span></span>
          <strong>{basename(item.runDir)}</strong>
          <span class="run-stage">{item.state?.stage ?? 'STARTING'} <span>#{number((item.state?.iteration ?? 0) + 1)}</span></span>
          <small>PID {item.pid}</small>
        </button>
      {:else}<p class="sidebar-empty">실행 중인 훈련이 없습니다.</p>{/each}
    </nav>
    <div class="sidebar-note"><span>CLI + WEB</span><p>CLI나 Agent가 시작한 훈련도<br />이 화면에 자동으로 나타납니다.</p></div>
  </aside>

  <main>
    {#if error}<div class="error" role="alert">{error}</div>{/if}

    {#if launcherOpen}
      <section class="launcher">
        <div class="section-heading"><div><span class="eyebrow">NEW RUN</span><h1>훈련 시작</h1></div><button class="quiet" onclick={() => { launcherOpen = false; }}>닫기 ×</button></div>
        <form onsubmit={(event) => { event.preventDefault(); start(); }}>
          <div class="launch-paths">
            <label>설정 프리셋<select value={presetPath} onchange={(event) => choosePreset(event.currentTarget.value)}>{#each presets as preset}<option value={preset.path}>{preset.name}</option>{/each}</select></label>
            <label>새 훈련 폴더<input required bind:value={runDir} spellcheck="false" /></label>
            <label class="full">Warm-start 모델 경로 <span class="muted">선택 · 비워두면 scratch</span><input bind:value={warmStart} placeholder="models/min/2.0.1 또는 deployment export 경로" spellcheck="false" /></label>
          </div>
          {#if draft}
            <div class="settings-grid">
              {#each fields as [section, key, label]}
                {#if draft[section]?.[key] != null}<label>{label}<input type="number" step="any" value={draft[section][key]} onchange={(event) => editField(section, key, event.currentTarget.valueAsNumber)} /></label>{/if}
              {/each}
              {#if draft.runtime}
                <label>Device<input value={draft.runtime.device} onchange={(event) => editField('runtime', 'device', event.currentTarget.value)} /></label>
                <label>Precision<select value={draft.runtime.precision} onchange={(event) => editField('runtime', 'precision', event.currentTarget.value)}><option>fp32</option><option>fp16</option><option>bf16</option></select></label>
              {/if}
              {#if draft.self_play?.turn_order_group}<label>Turn-order group<select value={draft.self_play.turn_order_group} onchange={(event) => editField('self_play', 'turn_order_group', event.currentTarget.value)}><option>all</option><option>cyclic</option><option>reverse</option></select></label>{/if}
            </div>
          {/if}
          <details class="config-editor"><summary>전체 설정 JSON</summary><label><span class="sr-only">전체 훈련 설정 JSON</span><textarea bind:value={configText} spellcheck="false" rows="20"></textarea></label></details>
          {#if launchError}<p class="error" role="alert">{launchError}</p>{/if}
          <div class="launch-footer"><span class="muted">{draft?.model_name ?? '설정 JSON을 확인하세요'} · 기존 native CLI로 실행</span><button class="primary" type="submit" disabled={starting || !draft}>{starting ? '시작 중…' : '훈련 시작 →'}</button></div>
        </form>
      </section>
    {/if}

    {#if job}
      <div class="run-heading">
        <div><span class="eyebrow">{job.config?.model_name ?? 'TRAINING'} / {job.config?.self_play?.turn_order_group ?? 'all'}</span><h1>{basename(job.runDir)}</h1><p class="path">{job.runDir}</p></div>
        <div class="run-actions"><button class="quiet" onclick={copySettings}>설정 복사</button><button class="danger" onclick={() => stop(job.pid)}><span>■</span> 즉시 중지</button></div>
      </div>

      <div class="run-meta"><span class="stage"><i class="status-dot"></i>{job.state?.stage ?? 'STARTING'}</span><span>{job.config?.runtime?.device} · {job.config?.runtime?.precision}</span><span>PID {job.pid}</span>{#if job.startedAt}<span>시작 {new Date(job.startedAt).toLocaleTimeString('ko-KR')}</span>{/if}</div>

      <div class="stats">
        <div><span>ITERATION</span><strong>{number((job.state?.iteration ?? 0) + 1)}<small> / {number(job.config?.run_budget?.max_iterations)}</small></strong></div>
        <div><span>TRAINING STEP</span><strong>{number(job.state?.training_step)}</strong></div>
        <div><span>SELF-PLAY GAMES <small>최근 완료 단계</small></span><strong>{number(latestSelfplay?.games.completed)}<small> / {number(latestSelfplay?.games.requested)}</small></strong></div>
        <div><span>TOTAL LOSS</span><strong>{number(latest?.total_loss, 4)}</strong></div>
        <div><span>THROUGHPUT</span><strong>{number(latest?.samples_per_second)}<small> samples/s</small></strong></div>
      </div>

      <div class="charts">
        <Chart metrics={job.metrics} keys={['total_loss', 'policy_loss', 'value_loss']} labels={['Total', 'Policy', 'Value']} title="Loss" />
        <div class="speed-chart"><label class="metric-select"><span class="sr-only">차트 지표</span><select bind:value={speedMetric}>{#each Object.entries(speedOptions) as [key, value]}<option value={key}>{value[0]}</option>{/each}</select></label><Chart metrics={job.metrics} keys={[speedMetric]} labels={[speedOptions[speedMetric][0]]} title="Performance" unit={speedOptions[speedMetric][1]} /></div>
      </div>
      <p class="cadence">상태·지표는 native 단계가 끝날 때 기록됩니다. 실행 중 출력은 아래 콘솔에서 확인할 수 있습니다.</p>

      <section class="detail-panel">
        <div class="detail-toolbar">
          <div class="tabs" role="tablist" aria-label="훈련 세부 정보">{#each [['console', '콘솔'], ['config', '실행 설정'], ['metadata', '전체 메타데이터']] as [key, label]}<button role="tab" aria-selected={tab === key} class:selected={tab === key} onclick={() => { tab = key; }}>{label}</button>{/each}</div>
          {#if tab === 'console'}<label class="follow"><input type="checkbox" bind:checked={followLog} />자동 스크롤</label>{/if}
        </div>
        {#if tab === 'console'}<pre class="console" bind:this={logElement}>{job.log || '아직 CLI 출력이 없습니다.'}</pre>
        {:else if tab === 'config'}<pre>{pretty(job.config)}</pre>
        {:else}<pre>{pretty({ state: job.state, iterations: job.iterations })}</pre>{/if}
      </section>
      <p class="command"><span>CLI</span><code>{Array.isArray(job.command) ? job.command.join(' ') : job.command}</code></p>
    {:else if !launcherOpen}
      <section class="welcome"><span class="eyebrow">ALPHADIAMOND / TRAINING</span><h1>훈련을 한눈에.</h1><p>실행 중인 훈련의 설정, 로그와 지표를 확인하세요.<br />CLI에서 시작한 작업도 자동으로 연결됩니다.</p><button class="primary" onclick={() => { launcherOpen = true; }}>+ 새 훈련 시작</button></section>
    {/if}
    <footer><span>AlphaDiamond Engine</span><span>{lastUpdate ? `${lastUpdate.toLocaleTimeString('ko-KR')} 업데이트 · 1.5초 간격` : '연결 중…'}</span></footer>
  </main>
</div>
