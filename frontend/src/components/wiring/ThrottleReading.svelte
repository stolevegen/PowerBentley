<script>
  import { Slider } from '$lib/components/ui/slider'
  import { Badge } from '$lib/components/ui/badge'
  import { Zap } from 'lucide-svelte'
  import { cn } from '$lib/utils.js'

  let {
    isAdcThrottle,
    currentThrottle,
    minThreshold = $bindable(),
    maxThreshold = $bindable(),
  } = $props()

  let minValue = 0
  let maxValue = 3.3

  // Calculate the processed throttle percentage for analog mode
  let processedThrottle = $derived(
    isAdcThrottle
      ? Math.max(
          0,
          Math.min(100, ((currentThrottle - minThreshold) / (maxThreshold - minThreshold)) * 100),
        )
      : currentThrottle * 100,
  )

  let processedPercentage = $derived((currentThrottle * 100) / maxValue)

  // Digital throttle state
  let digitalState = $derived(currentThrottle > 0.5 ? 'ON' : 'OFF')

  // Convert thresholds to slider format [min, max]
  let sliderValues = $derived([minThreshold, maxThreshold])

  function handleSliderChange(values) {
    minThreshold = values[0]
    maxThreshold = values[1]
  }
</script>

<!-- Current Throttle Value Display -->
<div class="space-y-4 p-4 rounded-lg border">
  <div class="flex items-center justify-between">
    <h4 class="font-medium flex items-center gap-2">
      <Zap class="h-4 w-4" />
      Current Throttle Reading
    </h4>
    <Badge variant={currentThrottle > 0 ? 'default' : 'secondary'}>
      {#if isAdcThrottle}
        {currentThrottle.toFixed(2)}V
      {:else}
        {digitalState}
      {/if}
    </Badge>
  </div>

  <!-- Visual representation -->
  {#if isAdcThrottle}
    <p>{processedThrottle.toFixed(0)}%</p>

    <!-- Analog Display -->
    <div class="space-y-3">
      <!-- Processed throttle bar -->
      <div class="relative pb-10">
        <!-- Background progress bar with zones -->
        <div class="h-4 rounded-full overflow-hidden relative">
          <!-- Current value progress bar -->
          <div class={cn('absolute top-0 h-full bg-gray-700')} style="left: 0; width: 100%"></div>
          <div
            class={cn(
              'absolute top-0 h-full transition-all duration-200 z-0',
              processedPercentage > 90
                ? 'bg-red-500'
                : processedPercentage > 50
                  ? 'bg-yellow-500'
                  : 'bg-green-500',
            )}
            style="left: 0; width: {processedPercentage}%"
          ></div>
        </div>

        <div class="absolute inset-0 flex items-start px-1">
          <Slider
            value={sliderValues}
            onValueChange={handleSliderChange}
            max={maxValue}
            min={minValue}
            withThumb={true}
            step={0.01}
            class="w-full [&_[data-slot=slider-track]]:bg-transparent 
                    [&_[data-slot=slider-range]]:bg-transparent 
                    [&_[data-slot=slider-thumb]]:rounded-sm [&_[data-slot=slider-thumb]]:!w-3 [&_[data-slot=slider-thumb]]:!h-8 [&_[data-slot=slider-thumb]]:shadow-lg [&_[data-slider-thumb]]:border-2
                    [&_[data-slot=slider-thumb]]:bg-green-600 [&_[data-slot=slider-thumb]]:border-slate-800
                    [&_[data-slider-thumb]:last-child]:bg-red-500 [&_[data-slider-thumb]:last-child]:border-red-600"
          />
        </div>
      </div>
    </div>
  {:else}
    <!-- Digital Display -->
    <div class="flex items-center justify-center py-4">
      <div
        class={cn(
          'w-16 h-16 rounded-full border-4 flex items-center justify-center font-bold text-lg transition-all duration-200',
          currentThrottle > 0.5
            ? 'border-green-500 bg-green-100 text-green-700'
            : 'border-gray-300 bg-gray-100 text-gray-500',
        )}
      >
        {digitalState}
      </div>
    </div>
  {/if}
</div>
