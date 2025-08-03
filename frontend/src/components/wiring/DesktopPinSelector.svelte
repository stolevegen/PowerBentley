<!-- Copyright (c) 2025 David Bertet. Licensed under the MIT License. -->

<script>
  import { slide } from 'svelte/transition'
  import * as Select from '$lib/components/ui/select'
  import { Label } from '$lib/components/ui/label'
  import { AlertTriangle } from 'lucide-svelte'
  import { colorSchemes } from 'src/lib/esp32_utils.js'

  let {
    label,
    value = $bindable(),
    pinOptions,
    validationMessage = null,
    colorScheme = 'blue', // blue, green, orange, yellow, purple, red
    disabledPins = [],
  } = $props()

  const currentScheme = $derived(validationMessage ? colorSchemes.red : colorSchemes[colorScheme])
  const hasError = $derived(!!validationMessage)
</script>

<div
  class="bg-white dark:bg-slate-800 border-2 {currentScheme.border} rounded-lg p-4 shadow-lg min-w-[150px]"
>
  <div class="flex items-center space-x-2 mb-3">
    {#if hasError}
      <AlertTriangle class="w-3 h-3 text-red-500" />
    {:else}
      <div class="w-3 h-3 rounded-full {currentScheme.indicator}"></div>
    {/if}
    <Label class="text-xs sm:text-sm font-medium {currentScheme.text}">
      {label}
    </Label>
  </div>
  <Select.Root type="single" bind:value>
    <Select.Trigger class="w-full h-10 font-mono {currentScheme.bg} {currentScheme.borderColor}">
      GPIO {value}
    </Select.Trigger>
    <Select.Content>
      <Select.Group>
        <Select.Label>{pinOptions.groupLabel}</Select.Label>

        {#each pinOptions.pins as pin}
          <Select.Item
            value={pin.value}
            label={pin.label}
            disabled={disabledPins.includes(pin.value)}
          >
            {pin.label}
          </Select.Item>
        {/each}
      </Select.Group>
    </Select.Content>
  </Select.Root>
</div>
