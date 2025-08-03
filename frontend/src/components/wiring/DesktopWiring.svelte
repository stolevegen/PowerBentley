<!-- Copyright (c) 2025 David Bertet. Licensed under the MIT License. -->

<script>
  import * as Card from '$lib/components/ui/card'
  import * as Select from '$lib/components/ui/select'
  import { Label } from '$lib/components/ui/label'
  import { Button } from '$lib/components/ui/button'
  import { Badge } from '$lib/components/ui/badge'
  import { Skeleton } from '$lib/components/ui/skeleton'
  import { cn } from '$lib/utils.js'
  import { RefreshCw } from 'lucide-svelte'
  import { adc1Pins, pwmPins, digitalInputPins } from 'src/lib/esp32_utils.js'

  import ValidationStatus from 'src/components/wiring/ValidationStatus.svelte'
  import DesktopPinSelector from 'src/components/wiring/DesktopPinSelector.svelte'

  let {
    isAdcThrottle,
    configMode,
    inputPinConfigs = $bindable(),
    outputPinConfigs = $bindable(),
    applyingConfig,
    pinBindings,
    hasValidationErrors,
    saveConfiguration,
  } = $props()
</script>

<Card.Root class="hidden sm:block">
  <Card.Header>
    <Card.Title>Interactive Pin Configuration</Card.Title>
    <Card.Description>Configure your connections using the visual flow diagram</Card.Description>
  </Card.Header>
  <Card.Content>
    {#if configMode === null}
      <!-- Skeleton Loading State -->
      <div
        class="bg-gradient-to-br from-slate-50 to-slate-100 dark:from-slate-900 dark:to-slate-800 rounded-xl border p-4 mt-2"
      >
        <!-- Mode indicator skeleton -->
        <div class="flex justify-between items-center mb-7">
          <Skeleton class="h-6 w-32 rounded-full" />
        </div>

        <!-- Simplified skeleton diagram -->
        <div class="flex flex-col items-center space-y-12">
          <!-- Power source skeleton -->
          <Skeleton class="h-18 w-24 rounded-lg" />

          <!-- Throttle components row skeleton -->
          <div class="flex items-center justify-center space-x-16">
            <Skeleton class="h-20 w-32 rounded-lg" />
          </div>

          <!-- Shifter components row skeleton -->
          <div class="flex items-center justify-center space-x-16">
            <Skeleton class="h-20 w-32 rounded-lg" />
          </div>

          <!-- GPIO selectors row skeleton -->
          <div class="flex items-center justify-center space-x-8">
            <Skeleton class="h-24 w-36 rounded-lg" />
            <Skeleton class="h-24 w-36 rounded-lg" />
          </div>

          <!-- ESP32 board skeleton -->
          <Skeleton class="h-35 w-90 rounded-xl" />
        </div>
      </div>

      <!-- Save section skeleton -->
      <div class="flex flex-col sm:flex-row gap-4 items-end sm:items-center justify-between mt-6">
        <div
          class="p-4 bg-gray-50 dark:bg-gray-900 border border-gray-200 dark:border-gray-800 rounded-lg"
        >
          <Skeleton class="h-4 w-50 mb-2" />
          <Skeleton class="h-4 w-full" />
        </div>
        <Skeleton class="h-10 w-36 rounded-md" />
      </div>
    {:else}
      <!-- Normal Content -->
      <div
        class="bg-gradient-to-br from-slate-50 to-slate-100 dark:from-slate-900 dark:to-slate-800 rounded-xl border p-4 mt-2"
      >
        <!-- Mode Indicator -->
        <div class="flex justify-between items-center mb-8">
          <Badge variant="outline" class="bg-white dark:bg-slate-800">
            {configMode === 'dual_input' ? 'Dual Input Mode' : 'Speed + Direction Mode'}
          </Badge>
        </div>

        <!-- Connection Flow -->
        <div class="flex flex-col items-center space-y-8">
          <!-- Power Source -->
          <div class="flex flex-col items-center m-0"></div>

          <!-- Input Components Row -->
          <div class="flex items-center justify-center space-x-16 m-0 items-end">
            {#if configMode === 'speed_direction'}
              <!-- Speed Control (left side) -->
              <div class="flex flex-col items-center space-y-4 m-0">
                {#if isAdcThrottle}
                  <div
                    class={cn(
                      'bg-gradient-to-br rounded-lg px-8 py-4 shadow-lg m-0 from-red-100 to-red-200 dark:from-red-900 dark:to-red-800 border-2 border-red-400',
                    )}
                  >
                    <div
                      class={cn(' font-bold text-center text-lg text-red-700 dark:text-red-300')}
                    >
                      +3.3V
                    </div>
                  </div>
                  <!-- Connection line continues -->
                  <div class="w-4 h-12 bg-gradient-to-b from-red-400 to-red-500 m-0"></div>
                {/if}

                <div class="flex items-center m-0">
                  <div
                    class="bg-gradient-to-br from-blue-100 to-blue-200 dark:from-blue-900 dark:to-blue-800 border-2 border-blue-400 rounded-lg px-6 py-4 shadow-lg m-0"
                  >
                    <div class="text-blue-700 dark:text-blue-300 font-bold text-center text-lg">
                      Throttle
                    </div>
                    <div class="text-blue-600 dark:text-blue-400 text-sm text-center">
                      Speed Input
                    </div>
                  </div>
                </div>

                <!-- Connection line continues -->
                <div class="w-4 h-45 bg-gradient-to-b from-blue-500 to-blue-600"></div>
              </div>
            {/if}
            <!-- Main Throttle/Shifter Column -->
            <div
              class={cn(
                'flex flex-col items-center space-y-4',
                configMode === 'speed_direction' ? 'mr-15' : '',
              )}
            >
              <div class="flex items-center justify-center space-x-24 m-0 gap-6">
                {#if isAdcThrottle && configMode === 'dual_input'}
                  <div
                    class={cn(
                      'bg-gradient-to-br rounded-lg px-8 py-4 shadow-lg m-0',
                      'from-red-100 to-red-200 dark:from-red-900 dark:to-red-800 border-2 border-red-400',
                    )}
                  >
                    <div
                      class={cn(' font-bold text-center text-lg', 'text-red-700 dark:text-red-300')}
                    >
                      +3.3V
                    </div>
                  </div>
                {/if}
                <div
                  class={cn(
                    'bg-gradient-to-br rounded-lg px-8 py-4 shadow-lg m-0',
                    'from-stone-100 to-stone-200 dark:from-stone-900 dark:to-stone-800 border-2 border-stone-400',
                  )}
                >
                  <div
                    class={cn(
                      ' font-bold text-center text-lg',
                      'text-stone-700 dark:text-stone-300',
                    )}
                  >
                    Neutral
                  </div>
                </div>
              </div>
              <!-- Branching connections -->
              {#if configMode === 'dual_input'}
                <div class="flex items-center justify-center space-x-18 m-0">
                  {#if isAdcThrottle}
                    <div class="w-4 h-12 bg-gradient-to-b from-red-400 to-red-500"></div>
                  {/if}
                  <div class="w-4 h-12 bg-gradient-to-b from-stone-400 to-stone-500"></div>
                </div>
              {/if}

              <!-- Power line down -->
              <div
                class={cn(
                  'flex items-center m-0',
                  configMode === 'speed_direction' ? 'w-full' : '',
                )}
              >
                {#if configMode === 'speed_direction'}
                  <!-- Horizontal power line continues -->
                  <div
                    class={cn('h-4 w-36 bg-gradient-to-b m-0', 'from-stone-400 to-stone-500')}
                  ></div>
                  <div
                    class={cn('w-4 bg-gradient-to-b m-0', 'from-stone-400 to-stone-500', 'h-45')}
                  ></div>
                {/if}
              </div>
              {#if configMode === 'dual_input'}
                <!-- Dual Input Throttle -->
                <div
                  class="bg-gradient-to-br from-blue-100 to-blue-200 dark:from-blue-900 dark:to-blue-800 border-2 border-blue-400 rounded-lg px-8 py-4 shadow-lg m-0"
                >
                  <div class="text-blue-700 dark:text-blue-300 font-bold text-center text-lg">
                    Throttle
                  </div>
                  <div class="text-blue-600 dark:text-blue-400 text-sm text-center">
                    Speed Input
                  </div>
                </div>
                <!-- Connection line -->
                <div class="w-4 h-12 bg-gradient-to-b from-blue-400 to-amber-400 m-0"></div>
              {/if}

              <!-- 3-Way Shifter -->
              <div class={cn(configMode === 'dual_input' ? '' : 'ml-18')}>
                <div
                  class="bg-gradient-to-br from-amber-100 to-amber-200 dark:from-amber-900 dark:to-amber-800 border-2 border-amber-400 rounded-lg px-6 py-4 shadow-lg m-0"
                >
                  <div class="text-amber-700 dark:text-amber-300 font-bold text-center text-lg m-0">
                    3-Way Shifter
                  </div>
                  <div class="text-amber-600 dark:text-amber-400 text-sm text-center">
                    Forward / Neutral / Reverse
                  </div>
                </div>

                <!-- Branching connections -->
                <div class="flex items-center justify-center space-x-24 m-0">
                  <div class="w-4 h-12 bg-gradient-to-b from-amber-400 to-green-700"></div>
                  <div class="w-4 h-12 bg-gradient-to-b from-amber-400 to-orange-700"></div>
                </div>
              </div>
            </div>
          </div>

          <div class="flex items-center justify-center space-x-8 m-0">
            {#if configMode === 'speed_direction'}
              <DesktopPinSelector
                label={inputPinConfigs.throttle.shortLabel}
                bind:value={pinBindings[inputPinConfigs.throttle.key].value}
                pinOptions={inputPinConfigs.throttle.pinOptions}
                validationMessage={inputPinConfigs.throttle.validation}
                colorScheme={inputPinConfigs.throttle.colorScheme}
                disabledPins={inputPinConfigs.throttle.disabledPins}
              />
            {/if}
            <!-- Forward GPIO -->
            <DesktopPinSelector
              label={inputPinConfigs.forward.shortLabel}
              bind:value={pinBindings[inputPinConfigs.forward.key].value}
              pinOptions={inputPinConfigs.forward.pinOptions}
              validationMessage={inputPinConfigs.forward.validation}
              colorScheme={inputPinConfigs.forward.colorScheme}
              disabledPins={inputPinConfigs.forward.disabledPins}
            />

            <!-- Backward GPIO -->
            <DesktopPinSelector
              label={inputPinConfigs.backward.shortLabel}
              bind:value={pinBindings[inputPinConfigs.backward.key].value}
              pinOptions={inputPinConfigs.backward.pinOptions}
              validationMessage={inputPinConfigs.backward.validation}
              colorScheme={inputPinConfigs.backward.colorScheme}
              disabledPins={inputPinConfigs.backward.disabledPins}
            />
          </div>

          <!-- Final connection lines to ESP32 -->
          <div class="flex items-center justify-center space-x-34 m-0">
            {#if configMode === 'speed_direction'}
              <div class="w-4 h-8 bg-gradient-to-b from-blue-500 to-slate-700"></div>
            {/if}
            <div class="w-4 h-8 bg-gradient-to-b from-green-500 to-slate-700"></div>
            <div class="w-4 h-8 bg-gradient-to-b from-orange-500 to-slate-700"></div>
          </div>

          <!-- ESP32 Board -->
          <div
            class={cn(
              ' w-90 bg-gradient-to-br from-slate-700 to-slate-800 dark:from-slate-200 dark:to-slate-300 border-2 border-slate-600 dark:border-slate-400 rounded-xl px-16 py-8 shadow-2xl',
            )}
          >
            <div class="text-white dark:text-slate-900 font-bold text-2xl text-center mb-2">
              ESP32
            </div>
            <div class="text-slate-300 dark:text-slate-600 text-sm text-center">
              Microcontroller Board
            </div>
          </div>
        </div>

        <!-- Connection lines to ESP32 -->
        <div class="flex items-center justify-center space-x-34 m-0">
          <div class="w-4 h-8 bg-gradient-to-b from-slate-700 to-yellow-500"></div>
          <div class="w-4 h-8 bg-gradient-to-b from-slate-700 to-purple-500"></div>
        </div>

        <!-- GPIO Pin Selectors Row -->
        <div class="flex items-center justify-center space-x-8 m-0">
          <!-- Forward motor GPIO -->
          <DesktopPinSelector
            label={outputPinConfigs.forwardMotor.shortLabel}
            bind:value={pinBindings[outputPinConfigs.forwardMotor.key].value}
            pinOptions={outputPinConfigs.forwardMotor.pinOptions}
            validationMessage={outputPinConfigs.forwardMotor.validation}
            colorScheme={outputPinConfigs.forwardMotor.colorScheme}
            disabledPins={outputPinConfigs.forwardMotor.disabledPins}
          />
          <!-- Backward motor GPIO -->
          <DesktopPinSelector
            label={outputPinConfigs.backwardMotor.shortLabel}
            bind:value={pinBindings[outputPinConfigs.backwardMotor.key].value}
            pinOptions={outputPinConfigs.backwardMotor.pinOptions}
            validationMessage={outputPinConfigs.backwardMotor.validation}
            colorScheme={outputPinConfigs.backwardMotor.colorScheme}
            disabledPins={outputPinConfigs.backwardMotor.disabledPins}
          />
        </div>
      </div>

      <!-- Save Configuration -->
      <div class="flex flex-col gap-4 items-end justify-between mt-6 w-full">
        <ValidationStatus hasErrors={hasValidationErrors} />

        <div class="flex relative items-center">
          <Button onclick={saveConfiguration} variant="default" disabled={applyingConfig}>
            Save Configuration
          </Button>
          {#if applyingConfig}
            <RefreshCw class="absolute inset-0 m-auto h-5 w-5 animate-spin" />
          {/if}
        </div>
      </div>
    {/if}
  </Card.Content>
</Card.Root>
