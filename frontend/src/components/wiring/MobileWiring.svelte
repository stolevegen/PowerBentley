<!-- Copyright (c) 2025 David Bertet. Licensed under the MIT License. -->

<script>
  import * as Card from '$lib/components/ui/card'
  import { Button } from '$lib/components/ui/button'
  import { Skeleton } from '$lib/components/ui/skeleton'
  import { RefreshCw } from 'lucide-svelte'
  import { adc1Pins, pwmPins, digitalInputPins } from 'src/lib/esp32_utils.js'

  import MobilePinSelector from 'src/components/wiring/MobilePinSelector.svelte'
  import ValidationStatus from 'src/components/wiring/ValidationStatus.svelte'

  let {
    configMode,
    inputPinConfigs = $bindable(),
    outputPinConfigs = $bindable(),
    applyingConfig,
    pinBindings,
    hasValidationErrors,
    saveConfiguration,
  } = $props()
</script>

<Card.Root class="block sm:hidden">
  <Card.Header>
    <Card.Title>Pin Configuration</Card.Title>
    <Card.Description>Configure your connections</Card.Description>
  </Card.Header>
  <Card.Content>
    <div class="space-y-4">
      {#if configMode === null}
        <!-- Skeleton Loading State -->
        {#each Array(2) as _, i}
          <div
            class="bg-white dark:bg-slate-800 border-2 border-gray-200 dark:border-gray-700 rounded-lg p-4 shadow-md"
          >
            <div class="flex items-center space-x-3 mb-3">
              <Skeleton class="w-4 h-4 rounded-full" />
              <Skeleton class="h-5 w-40" />
            </div>
            <Skeleton class="w-full h-11 rounded-md" />
            <Skeleton class="h-4 w-36 mt-2" />
          </div>
        {/each}

        <!-- Save Configuration Skeleton -->
        <div class="flex flex-col gap-4 items-end justify-between mt-6">
          <div
            class="p-4 w-full bg-gray-50 dark:bg-gray-900 border border-gray-200 dark:border-gray-800 rounded-lg"
          >
            <Skeleton class="h-5 w-full" />
          </div>
          <Skeleton class="h-10 w-full rounded-md" />
        </div>
      {:else}
        <!-- Inputs Section -->
        <h2 class="text-lg font-semibold mt-6 mb-4">Inputs</h2>
        {#each Object.entries(inputPinConfigs) as [key, config] (key)}
          <MobilePinSelector
            label={config.label}
            bind:value={pinBindings[config.key].value}
            pinOptions={config.pinOptions}
            validationMessage={config.validation}
            colorScheme={config.colorScheme}
            description={config.description}
            disabledPins={config.disabledPins}
            showTransition={true}
          />
        {/each}

        <!-- Outputs Section -->
        <h2 class="text-lg font-semibold mt-6 mb-4">Outputs</h2>
        {#each Object.entries(outputPinConfigs) as [key, config] (key)}
          <MobilePinSelector
            label={config.label}
            bind:value={pinBindings[config.key].value}
            pinOptions={config.pinOptions}
            colorScheme={config.colorScheme}
            description={config.description}
            disabledPins={config.disabledPins}
            showTransition={false}
          />
        {/each}

        <!-- Save Configuration -->
        <div class="flex flex-col gap-4 items-end justify-between mt-6">
          <ValidationStatus hasErrors={hasValidationErrors} />

          <Button
            class="relative w-full"
            onclick={saveConfiguration}
            variant="default"
            disabled={hasValidationErrors || applyingConfig}
          >
            Save Configuration
            {#if applyingConfig === true}
              <RefreshCw class="absolute right-5 h-5 w-5 ml-2 animate-spin" />
            {/if}
          </Button>
        </div>
      {/if}
    </div>
  </Card.Content>
</Card.Root>
