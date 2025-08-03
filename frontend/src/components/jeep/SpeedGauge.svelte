<!-- Copyright (c) 2025 David Bertet. Licensed under the MIT License. -->

<script>
  let { value = 0, isEmergencyStop = false } = $props()

  // Calculate stroke dash offset based on value (0-100%)
  // Original: 0% is 440, 100% is 147
  let strokeDashOffset = $derived(isEmergencyStop ? 147 : 440 - (440 - 147) * (value / 100))

  // Round the value for display
  let displayValue = $derived(value != null ? Math.round(value) : '--')
</script>

<div class="relative flex items-center justify-center w-[280px] h-[240px] overflow-hidden mx-auto">
  <div class="flex pt-[18px] z-10">
    <div class="text-[80px] font-semibold text-foreground">
      {#if isEmergencyStop}
        🚫
      {:else}
        {displayValue}
      {/if}
    </div>
    <div class="self-end pb-4 text-foreground" class:hidden={isEmergencyStop}>%</div>
  </div>

  <svg
    class="absolute top-0 left-0"
    style="filter: url(#shadow);"
    xmlns="http://www.w3.org/2000/svg"
    version="1.1"
    preserveAspectRatio="xMidYMid meet"
    viewBox="0 0 180 180"
  >
    <defs>
      <filter id="shadow" color-interpolation-filters="sRGB">
        <feDropShadow
          dx="3"
          dy="3"
          stdDeviation="5"
          flood-opacity="0.7"
          flood-color="var(--shadow-dark)"
        />
        <feDropShadow
          dx="-3"
          dy="-3"
          stdDeviation="3"
          flood-opacity="0.15"
          flood-color="var(--shadow-light)"
        />
      </filter>
      <linearGradient id="GradientColor">
        <stop offset="0%" stop-color="var(--gauge-100)" />
        <stop offset="50%" stop-color="var(--gauge-50)" />
        <stop offset="100%" stop-color="var(--gauge-0)" />
      </linearGradient>
    </defs>

    <!-- Background circle -->
    <circle class="gauge_background" cx="90" cy="90" r="70" stroke-linecap="round" />

    <!-- Foreground progress circle -->
    <circle
      class="gauge_frontground"
      cx="90"
      cy="90"
      r="70"
      stroke-linecap="round"
      style:stroke-dashoffset={strokeDashOffset}
      style:stroke={isEmergencyStop ? 'red' : 'url(#GradientColor)'}
    />
  </svg>
</div>

<style>
  /* Custom CSS variables for gauge colors */
  :global(:root) {
    --gauge-0: #27ae60;
    --gauge-50: #f39c12;
    --gauge-100: #e74c3c;
  }

  /* SVG-specific styles that can't be handled by Tailwind */
  .gauge_frontground {
    fill: none;
    stroke: url(#GradientColor);
    stroke-width: 20px;
    stroke-dasharray: 440;
    stroke-dashoffset: 147;
    transform-origin: 50% 50%;
    transform: rotate(150deg);
    transition: stroke-dashoffset 0.3s ease;
  }

  .gauge_background {
    fill: none;
    stroke: var(--background); /* --background color */
    stroke-width: 20px;
    stroke-dasharray: 440;
    stroke-dashoffset: 147;
    transform-origin: 50% 50%;
    transform: rotate(150deg);
  }
</style>
