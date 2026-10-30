<script>
  let { id, label, options, value = $bindable(), onchange } = $props();
  let trigger, menu;
  let open = $state(false);
  let active = $state(0);
  let selected = $derived(options.find(option => option.value === value));

  function prepare(event) {
    open = event.newState === 'open';
    if (!open) return;
    active = Math.max(0, options.findIndex(option => option.value === value));
    const rect = trigger.getBoundingClientRect();
    const width = Math.min(Math.max(rect.width, 180), window.innerWidth - 24);
    const height = Math.min(options.length * 36 + 12, 300);
    const below = rect.bottom + height + 6 <= window.innerHeight;
    menu.dataset.placement = below ? 'below' : 'above';
    menu.style.width = `${width}px`;
    menu.style.left = `${Math.max(12, Math.min(rect.left, window.innerWidth - width - 12))}px`;
    menu.style.top = `${below ? rect.bottom + 6 : Math.max(12, rect.top - height - 6)}px`;
  }
  function choose(option) {
    value = option.value;
    onchange?.(value);
    menu.hidePopover();
    trigger.focus();
  }
  function keydown(event) {
    if (['ArrowDown', 'ArrowUp', 'Home', 'End'].includes(event.key)) {
      event.preventDefault();
      const wasOpen = open;
      if (!open) menu.showPopover();
      if (event.key === 'Home') active = 0;
      else if (event.key === 'End') active = options.length - 1;
      else if (wasOpen) active = (active + (event.key === 'ArrowDown' ? 1 : -1) + options.length) % options.length;
      document.getElementById(`${id}-option-${active}`)?.scrollIntoView({ block: 'nearest' });
    } else if (open && (event.key === 'Enter' || event.key === ' ')) {
      event.preventDefault();
      choose(options[active]);
    } else if (event.key === 'Tab') menu.hidePopover();
    else if (open && event.key.length === 1 && !event.ctrlKey && !event.metaKey && !event.altKey) {
      const index = options.findIndex((option, index) => index > active && option.label.toLowerCase().startsWith(event.key.toLowerCase()));
      const match = index >= 0 ? index : options.findIndex(option => option.label.toLowerCase().startsWith(event.key.toLowerCase()));
      if (match >= 0) {
        event.preventDefault();
        active = match;
        document.getElementById(`${id}-option-${active}`)?.scrollIntoView({ block: 'nearest' });
      }
    }
  }
</script>

<svelte:window onresize={() => menu?.hidePopover()} onscroll={() => menu?.hidePopover()} />
<div class="select">
  <button {id} bind:this={trigger} class="select-trigger" type="button" role="combobox"
    aria-label={label} aria-expanded={open} aria-haspopup="listbox" aria-controls={`${id}-menu`}
    aria-activedescendant={open ? `${id}-option-${active}` : undefined}
    popovertarget={`${id}-menu`} disabled={!options.length} onkeydown={keydown}>
    <span>{selected?.label ?? '—'}</span>
    <svg viewBox="0 0 24 24" aria-hidden="true"><path d="m7 10 5 5 5-5" /></svg>
  </button>
  <div id={`${id}-menu`} bind:this={menu} class="select-menu" popover="auto" role="listbox"
    aria-label={label} onbeforetoggle={prepare}>
    {#each options as option, index (option.value)}
      <button id={`${id}-option-${index}`} class="select-option" class:active={active === index}
        type="button" role="option" tabindex="-1" aria-selected={option.value === value}
        onpointermove={() => { active = index; }} onclick={() => choose(option)}>
        <span>{option.label}</span>
        {#if option.value === value}<svg viewBox="0 0 24 24" aria-hidden="true"><path d="m5 12 4 4 10-10" /></svg>{/if}
      </button>
    {/each}
  </div>
</div>
