<script>
  import ResourceChart from './ResourceChart.svelte';
  import { formatBytes } from './chart.js';
  let { kind, section, history, current } = $props();
  let showLogicalCpus = $state(false);
  const percent = value => Number.isFinite(value) ? `${value.toFixed(1)}%` : '—';
  const frequency = value => Number.isFinite(value) ? (value >= 1000 ? `${(value / 1000).toFixed(2)} GHz` : `${value.toFixed(0)} MHz`) : '—';
  const watts = value => Number.isFinite(value) ? `${value.toFixed(1)} W` : '—';
  const properties = $derived(current?.properties);
</script>

<section class="resource-monitor" aria-label={section.title}>
  <div class="resource-heading"><h1>{current?.name ?? section.title}</h1>{#if current?.name}<span class="muted">{section.title}</span>{/if}</div>
  {#if kind === 'processor'}
    <label class="resource-option">
      <span>Show Usages of Logical CPUs</span>
      <input class="resource-switch" type="checkbox" role="switch" bind:checked={showLogicalCpus} />
    </label>
  {/if}
  <h2 class="resource-section-title">Usage</h2>
  {#if kind === 'processor' && showLogicalCpus}
    <div class="logical-cpu-grid">
      {#each current?.cores ?? [] as core (core.id)}
        <section class="resource-metric logical-cpu">
          <ResourceChart {history} {kind} coreId={core.id} color={section.color} label={`CPU ${core.id + 1} usage`} small />
          <h2>CPU {core.id + 1} · {frequency(core.frequencyMHz)}</h2>
          <p>{percent(core.usage)}</p>
        </section>
      {/each}
    </div>
  {:else}
    <div class="resource-metrics" class:gpu-metrics={kind === 'gpu'}>
      {#each section.metrics as [metric, label, used, total] (metric)}
        <section class="resource-metric" class:half={metric === 'encoderUsage' || metric === 'decoderUsage'}>
          <ResourceChart {history} {kind} {metric} color={section.color} {label} />
          <h2>{label}</h2>
          <p>{#if used}{formatBytes(current?.[used])} / {formatBytes(current?.[total])} · {/if}{percent(current?.[metric])}</p>
        </section>
      {/each}
      {#if kind === 'gpu'}
        <dl class="resource-details gpu-details">
          <div><dt>GPU Frequency</dt><dd>{frequency(current?.frequencyMHz)}</dd></div>
          <div><dt>Video Memory Frequency</dt><dd>{frequency(current?.memoryFrequencyMHz)}</dd></div>
          <div><dt>Power Usage</dt><dd>{watts(current?.powerWatts)} / {watts(current?.powerLimitWatts)}</dd></div>
        </dl>
      {/if}
    </div>
  {/if}
  {#if kind === 'memory'}
    <h2 class="resource-section-title">Properties</h2>
    <dl class="resource-details">
      <div><dt>Slots Used</dt><dd>{properties ? `${properties.slotsUsed} of ${properties.slotsTotal ?? '—'}` : '—'}</dd></div>
      <div><dt>Speed</dt><dd>{properties?.speed ?? '—'}</dd></div>
      <div><dt>Form Factor</dt><dd>{properties?.formFactor ?? '—'}</dd></div>
      <div><dt>Type</dt><dd>{properties?.type ?? '—'}</dd></div>
      <div><dt>Type Detail</dt><dd>{properties?.typeDetail ?? '—'}</dd></div>
    </dl>
  {:else if kind === 'gpu'}
    <h2 class="resource-section-title">Sensors</h2>
    <dl class="resource-details">
      <div><dt>Temperature</dt><dd>{Number.isFinite(current?.temperatureC) ? `${current.temperatureC.toFixed(0)} °C` : '—'}</dd></div>
    </dl>
  {/if}
</section>
