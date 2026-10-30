<script>
  let { id, text, children } = $props();
  let hovered = $state(false), focused = $state(false), dismissed = $state(false);
  let visible = $derived((hovered || focused) && !dismissed);
</script>

<svelte:window onkeydown={event => { if (event.key === 'Escape') dismissed = true; }} />
<span class="tooltip" class:open={visible} role="presentation"
  onmouseenter={() => { hovered = true; dismissed = false; }} onmouseleave={() => { hovered = false; }}
  onfocusin={() => { focused = true; dismissed = false; }} onfocusout={() => { focused = false; }}>
  {@render children()}
  <span {id} class="tooltip-bubble" role="tooltip" hidden={!visible}>{text}</span>
</span>

<style>
  .tooltip { position: relative; display: inline-flex; }
  :global(.detail-toolbar) > .tooltip { margin-left: auto; }
  .tooltip.open::after { content: ''; position: absolute; top: 100%; right: 0; width: 100%; height: 8px; }
  .tooltip-bubble { position: absolute; z-index: 10; top: calc(100% + 8px); right: 0; width: max-content; max-width: min(280px, calc(100vw - 24px)); padding: 6px 10px; border: 1px solid #1a1b1f1a; border-radius: 8px; background: #fff; color: #1a1b1f; box-shadow: 0 4px 16px #1a1b1f14; font-size: 12px; line-height: 20px; font-weight: 400; }
</style>
