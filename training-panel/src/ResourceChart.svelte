<script>
  import { onMount } from 'svelte';
  import { Chart, LineController, LineElement, PointElement, LinearScale, Tooltip, Filler } from 'chart.js';
  Chart.register(LineController, LineElement, PointElement, LinearScale, Tooltip, Filler);
  let { history, kind, metric = 'usage', color, label, compact = false, coreId = null, small = false } = $props();
  let canvas;
  let chart = $state.raw();
  onMount(() => {
    chart = new Chart(canvas, {
      type: 'line', data: { datasets: [] },
      options: {
        responsive: true, maintainAspectRatio: false, animation: false, parsing: false,
        elements: { point: { radius: 0 }, line: { borderWidth: 1.5 } },
        interaction: { mode: 'nearest', axis: 'x', intersect: false },
        plugins: { legend: { display: false }, tooltip: { enabled: !compact, callbacks: {
          title: items => new Date(items[0].parsed.x).toLocaleTimeString('ko-KR'),
          label: item => `${item.parsed.y.toFixed(1)}%`,
        } } },
        scales: {
          x: { type: 'linear', display: false },
          y: { display: false, min: 0, max: 100 },
        },
      },
    });
    return () => chart.destroy();
  });
  $effect(() => {
    const data = history.map(sample => ({ x: sample.timestamp,
      y: (coreId == null ? sample[kind]?.[metric] : sample.processor?.cores?.find(core => core.id === coreId)?.usage) ?? null }));
    if (!chart) return;
    chart.data.datasets = [{ data, borderColor: color, backgroundColor: `${color}26`, fill: 'start' }];
    const end = history.at(-1)?.timestamp ?? Date.now();
    chart.options.scales.x.min = end - 119 * 1000;
    chart.options.scales.x.max = end;
    chart.update('none');
  });
</script>

<div class="resource-chart" class:compact class:small style:background={`${color}0d`}>
  <canvas bind:this={canvas} aria-label={label} aria-hidden={compact}></canvas>
</div>

<style>
  .resource-chart { position: relative; height: 170px; border-radius: 9px; overflow: hidden; }
  .compact { height: 54px; border-radius: 3px; }
  .small { height: 80px; border-radius: 5px; }
  canvas { display: block; width: 100%; height: 100%; }
</style>
