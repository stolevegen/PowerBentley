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
    description,
    disabledPins = [],
    showTransition = true,
  } = $props()

  const currentScheme = $derived(validationMessage ? colorSchemes.red : colorSchemes[colorScheme])
  const hasError = $derived(!!validationMessage)
</script>

<div
  class="bg-white dark:bg-slate-800 border-2 {currentScheme.border} rounded-lg p-4 shadow-md transition-all duration-250"
  transition:slide={{ duration: showTransition ? 250 : 0, axis: 'y' }}
>
  <div class="flex items-center space-x-3 mb-3">
    {#if hasError}
      <AlertTriangle class="w-4 h-4 text-red-500" />
    {:else}
      <div class="w-4 h-4 rounded-full {currentScheme.indicator}"></div>
    {/if}
    <Label class="font-semibold {currentScheme.text}">
      {label}
    </Label>
  </div>

  <Select.Root type="single" bind:value>
    <Select.Trigger
      class="w-full h-12 font-mono text-base {currentScheme.bg} {currentScheme.borderColor}"
    >
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

  {#if hasError}
    <p class="text-sm text-red-600 dark:text-red-400 mt-2 flex items-center gap-1">
      {validationMessage}
    </p>
  {:else}
    <p class="text-sm {currentScheme.descText} mt-2">
      {description}
    </p>
  {/if}
</div>
