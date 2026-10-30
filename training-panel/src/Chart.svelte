<script>
  import { onMount } from 'svelte';
  import { Chart, LineController, LineElement, PointElement, LinearScale, Tooltip, Legend, Filler } from 'chart.js';
  import { getRelativePosition } from 'chart.js/helpers';
  import { chartData, formatBytes, zoomRange } from './chart.js';
  Chart.register(LineController, LineElement, PointElement, LinearScale, Tooltip, Legend, Filler);
  Chart.defaults.font.family = 'Pretendard, sans-serif';
  let { metrics, keys, labels, title, unit = '', bytes = false, scale = 1, fromStep, toStep, minStep, maxStep, onzoom } = $props();
  let canvas;
  let chart = $state.raw();
  const colors = ['#2f6fca', '#b17224', '#8457b5'];
  let series = $derived(chartData(metrics, keys, fromStep, toStep, scale));
  let hasData = $derived(series.some((s) => s.data.some((p) => p.y !== null)));

  onMount(() => {
    chart = new Chart(canvas, {
      type: 'line',
      data: { datasets: [] },
      options: {
        responsive: true, maintainAspectRatio: false, animation: false, parsing: false,
        interaction: { mode: 'nearest', axis: 'x', intersect: false },
        elements: { point: { radius: 0, hitRadius: 8, hoverRadius: 3 }, line: { borderWidth: 1.5 } },
        plugins: {
          legend: { align: 'start', labels: { color: '#526176', usePointStyle: true, pointStyle: 'line', boxWidth: 20, padding: 20, font: { size: 13 } } },
          tooltip: { backgroundColor: '#fff', titleColor: '#1b2637', bodyColor: '#526176', borderColor: '#cbd4e0', borderWidth: 1, padding: 12 },
        },
        scales: {
          x: { type: 'linear', title: { display: true, text: '학습 스텝', color: '#5e6d80' },
            grid: { display: false }, border: { color: '#dce2e9' }, ticks: { color: '#5e6d80', maxTicksLimit: 6, maxRotation: 0 } },
          y: { grid: { color: '#e7ecf2' }, border: { display: false }, ticks: { color: '#5e6d80', maxTicksLimit: 5 } },
        },
      },
    });
    const wheel = (event) => {
      const { x, y } = getRelativePosition(event, chart);
      const area = chart.chartArea;
      if (!hasData || !event.deltaY || x < area.left || x > area.right || y < area.top || y > area.bottom) return;
      event.preventDefault();
      const delta = event.deltaY * (event.deltaMode === 1 ? 16 : event.deltaMode === 2 ? chart.height : 1);
      const factor = Math.exp(Math.max(-1, Math.min(1, delta * 0.002)));
      onzoom(zoomRange(fromStep, toStep, chart.scales.x.getValueForPixel(x), factor, minStep, maxStep));
    };
    canvas.addEventListener('wheel', wheel, { passive: false });
    return () => {
      canvas.removeEventListener('wheel', wheel);
      chart.destroy();
    };
  });

  $effect(() => {
    const data = series;
    if (!chart) return;
    chart.data.datasets = data.map((s, i) => ({
      label: labels[i], data: s.data, borderColor: colors[i], fill: 'start',
      backgroundColor: ({ chart: { ctx, chartArea } }) => {
        if (!chartArea) return 'transparent';
        const gradient = ctx.createLinearGradient(0, chartArea.top, 0, chartArea.bottom);
        gradient.addColorStop(0, `${colors[i]}26`);
        gradient.addColorStop(1, `${colors[i]}00`);
        return gradient;
      },
      pointRadius: s.data.length === 1 ? 3 : 0,
      hidden: chart.data.datasets[i]?.label === labels[i] && !chart.isDatasetVisible(i),
    }));
    chart.options.scales.x.min = fromStep;
    chart.options.scales.x.max = toStep;
    chart.options.scales.y.title = { display: Boolean(unit), text: unit, color: '#5e6d80' };
    chart.options.scales.y.ticks.callback = (value) => bytes ? formatBytes(value) : value.toLocaleString('en');
    chart.options.plugins.tooltip.callbacks.title = (items) => `학습 스텝 ${items[0].parsed.x.toLocaleString('en', { maximumFractionDigits: 1 })} · 구간 평균`;
    chart.options.plugins.tooltip.callbacks.label = (item) => `${item.dataset.label}: ${bytes ? formatBytes(item.parsed.y) : `${item.formattedValue}${unit ? ` ${unit}` : ''}`}`;
    chart.update('none');
  });
</script>

<section class="chart-panel">
  <div class="section-heading"><h2>{title}</h2></div>
  <div class="chart-plot" class:chart-waiting={!hasData}>
    <canvas bind:this={canvas} aria-label={`${title}: 학습 스텝 ${fromStep}–${toStep}, ${labels.join(', ')}`}>
      {#each series as s, i}<p>{labels[i]}: {s.data.at(-1)?.y ?? '기록 없음'}</p>{/each}
    </canvas>
    {#if !hasData}<p class="chart-empty">기록 없음</p>{/if}
  </div>
</section>
