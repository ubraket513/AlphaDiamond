<script>
  let { value, digits = 0 } = $props();
  let element;
  let previous;
  const baseDuration = 1800, durationStep = 160;
  let text = $derived(Number.isFinite(value)
    ? new Intl.NumberFormat('en', { minimumFractionDigits: digits, maximumFractionDigits: digits }).format(value)
    : '—');

  $effect(() => {
    const before = previous;
    previous = value;
    if (!Number.isFinite(before) || !Number.isFinite(value) || value === before) return;
    const duration = baseDuration + (text.length - 1) * durationStep;
    const color = value > before ? '#15803d' : '#dc2626';
    const animation = element.animate([
      { color, offset: 0 },
      { color, offset: duration / (duration + 300) },
      { color: 'inherit' },
    ], { duration: duration + 300, easing: 'linear' });
    return () => animation.cancel();
  });
</script>

<span class="rolling-number" bind:this={element}>
  <span class="sr-only">{text}</span>
  <span class="reels" aria-hidden="true">
    {#each [...text] as char, index (text.length - index)}
      {#if /\d/.test(char)}
        <span class="digit"><span class="strip" style:--roll-duration={`${baseDuration + index * durationStep}ms`} style:transform={`translateY(-${Number(char) * 10}%)`}>
          {#each [0, 1, 2, 3, 4, 5, 6, 7, 8, 9] as digit}<span>{digit}</span>{/each}
        </span></span>
      {:else}<span>{char}</span>{/if}
    {/each}
  </span>
</span>

<style>
  .rolling-number { display: inline-block; font-variant-numeric: tabular-nums; vertical-align: bottom; }
  .reels { display: flex; height: 1.25em; line-height: 1.25; }
  .digit { display: inline-block; overflow: hidden; width: 1ch; }
  .strip { display: flex; flex-direction: column; transition: transform var(--roll-duration) cubic-bezier(.16, .55, .2, 1); }
  .strip > span { height: 1.25em; flex: none; }
  @media (prefers-reduced-motion: reduce) { .strip { transition: none; } }
</style>
