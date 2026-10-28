<script>
  import { chartData } from './chart.js';
  let { metrics, keys, labels, title, unit = '' } = $props();
  const colors = ['#79dfb5', '#9daeff', '#e9bc78'];
  let data = $derived(chartData(metrics, keys));
  const format = (n) => new Intl.NumberFormat('en', { maximumFractionDigits: 3 }).format(n);
</script>

<section class="chart-card">
  <div class="section-heading"><h2>{title}</h2><span class="muted">{unit}</span></div>
  <div class="legend">
    {#each labels as label, index}<span><i style:background={colors[index]}></i>{label}</span>{/each}
  </div>
  {#if data}
    <div class="plot">
      <div class="y-axis"><span>{format(data.y[1])}</span><span>{format(data.y[0])}</span></div>
      <svg viewBox="-1 -4 102 108" preserveAspectRatio="none" role="img" aria-label={`${title}: ${metrics.length}개 지표, step ${data.x[0]}–${data.x[1]}`}>
        {#each [0, 25, 50, 75, 100] as y}<line x1="0" y1={y} x2="100" y2={y} class="grid-line" />{/each}
        {#each data.lines as line, index}
          <polyline points={line.points} fill="none" stroke={colors[index]} stroke-width="1.7" vector-effect="non-scaling-stroke" />
          {#if metrics.length === 1}<circle cx="50" cy="50" r="1" fill={colors[index]} />{/if}
        {/each}
      </svg>
      <div class="x-axis"><span>{data.x[0]}</span><span>training step</span><span>{data.x[1]}</span></div>
    </div>
  {:else}
    <div class="chart-empty"><span class="empty-mark">⌁</span><p>첫 train 단계가 끝나면 그래프가 나타납니다.</p></div>
  {/if}
</section>
