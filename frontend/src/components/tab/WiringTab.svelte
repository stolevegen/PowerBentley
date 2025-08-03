<!-- Copyright (c) 2025 David Bertet. Licensed under the MIT License. -->

<script>
  import { onMount, onDestroy } from 'svelte'
  import { sendMessage, onMessageType } from 'src/lib/ws.svelte.js'
  import * as Card from '$lib/components/ui/card'
  import * as RadioGroup from '$lib/components/ui/radio-group'
  import * as Select from '$lib/components/ui/select'
  import { Checkbox } from '$lib/components/ui/checkbox'
  import { Label } from '$lib/components/ui/label'
  import { Button } from '$lib/components/ui/button'
  import { Input } from '$lib/components/ui/input'
  import { Badge } from '$lib/components/ui/badge'
  import { cn } from '$lib/utils.js'
  import { Lock, LockOpen, Activity } from 'lucide-svelte'
  import { adc1Pins, pwmPins, digitalInputPins } from 'src/lib/esp32_utils.js'

  import SectionHeader from 'src/components/common/SectionHeader.svelte'
  import DisabledSection from 'src/components/common/DisabledSection.svelte'
  import MobileWiring from 'src/components/wiring/MobileWiring.svelte'
  import DesktopWiring from 'src/components/wiring/DesktopWiring.svelte'
  import ThrottleReading from 'src/components/wiring/ThrottleReading.svelte'
  import { settingsState } from 'src/lib/settings.svelte.js'

  // Configuration state
  let configMode = $state(null) // 'dual_input' or 'speed_direction'
  let isAdcThrottle = $state(false) // Whether the throttle is an ADC input
  let inputsConfig = $state({
    forward: '',
    backward: '',
    throttle: '',
  })
  let outputsConfig = $state({
    forwardMotor: '',
    backwardMotor: '',
  })
  let currentThrottle = $state(0.0)

  let applyingConfig = $state(false)

  $effect(() => {
    if (configMode !== 'speed_direction') {
      inputsConfig.throttle = ''
    } else {
      if (inputsConfig.throttle) {
        return
      }
      // search first available ADC pin
      var index = 0
      while (
        outputsConfig.forwardMotor === adc1Pins[index].value ||
        outputsConfig.backwardMotor === adc1Pins[index].value ||
        inputsConfig.forward === adc1Pins[index].value ||
        inputsConfig.backward === adc1Pins[index].value
      )
        index++
      inputsConfig.throttle = adc1Pins[index].value
    }
  })

  $effect(() => {
    if (settingsState.powerWheel?.setupMode) {
      sendMessage({ type: 'read_throttle' })
    } else {
      currentThrottle = 0
    }
  })

  // WebSocket handlers
  let configUnsub = $state(null)
  let throttleUnsub = $state(null)

  onMount(() => {
    configUnsub = onMessageType('wiring_response', (data) => {
      applyingConfig = false

      inputsConfig = {
        forward: data.inputs.forward.toString(),
        backward: data.inputs.backward.toString(),
        throttle: data.inputs.throttle?.toString(),
      }
      outputsConfig = {
        forwardMotor: data.outputs.forward_motor.toString(),
        backwardMotor: data.outputs.backward_motor.toString(),
      }
      configMode = data.mode
      isAdcThrottle = data.is_adc_throttle
      minThreshold = data.min_threshold
      maxThreshold = data.max_threshold
    })

    throttleUnsub = onMessageType('read_throttle_response', (data) => {
      if (settingsState.powerWheel?.setupMode) {
        currentThrottle = data.current_throttle
      } else {
        currentThrottle = 0
      }

      setTimeout(() => {
        if (settingsState.powerWheel?.setupMode) {
          sendMessage({ type: 'read_throttle' })
        }
      }, 500)
    })

    sendMessage({ type: 'get_wiring' })

    return () => {
      if (configUnsub) configUnsub()
      if (throttleUnsub) throttleUnsub()
    }
  })

  onDestroy(() => {
    if (configUnsub) configUnsub()
    if (throttleUnsub) throttleUnsub()
  })

  function saveConfiguration() {
    if (applyingConfig) return
    applyingConfig = true

    const config = {
      type: 'set_wiring',
      mode: configMode,
      is_adc_throttle: isAdcThrottle,
      min_threshold: minThreshold,
      max_threshold: maxThreshold,
      inputs:
        configMode === 'dual_input'
          ? {
              forward: parseInt(inputsConfig.forward),
              backward: parseInt(inputsConfig.backward),
            }
          : {
              forward: parseInt(inputsConfig.forward),
              backward: parseInt(inputsConfig.backward),
              throttle: parseInt(inputsConfig.throttle),
            },
      outputs: {
        forward_motor: parseInt(outputsConfig.forwardMotor),
        backward_motor: parseInt(outputsConfig.backwardMotor),
      },
    }

    sendMessage(config)
  }

  // Calibration settings for analog throttle
  let minThreshold = $state(0) // Minimum voltage for 0%
  let maxThreshold = $state(0) // Maximum voltage for 100%

  const usedPins = $derived(
    [
      inputsConfig.forward,
      inputsConfig.backward,
      inputsConfig.throttle,
      outputsConfig.forwardMotor,
      outputsConfig.backwardMotor,
    ].filter(Boolean),
  )

  const getDisabledPins = (excludePin) => usedPins.filter((pin) => pin !== excludePin)

  // Pin configuration definitions
  const inputPinConfigs = $derived.by(() => {
    const configs = {}

    // Speed control (only in speed_direction mode)
    if (configMode === 'speed_direction') {
      configs.throttle = {
        key: 'throttle',
        label: 'Speed Control GPIO',
        shortLabel: 'Speed',
        value: inputsConfig.throttle,
        pinOptions: {
          pins: isAdcThrottle ? adc1Pins : digitalInputPins,
          groupLabel: isAdcThrottle ? 'ADC1 Pins' : 'Digital pins',
        },
        colorScheme: 'blue',
        description: 'Controls motor speed',
        validation: isAdcThrottle
          ? getValidationMessage(inputsConfig.throttle, 'adc1', 'Speed Control GPIO')
          : getValidationMessage(inputsConfig.throttle, 'digital', 'Speed Control GPIO'),
        disabledPins: getDisabledPins(inputsConfig.throttle),
      }
    }

    // Forward input
    configs.forward = {
      key: 'forward',
      label: configMode === 'dual_input' ? 'Forward Speed GPIO' : 'Forward Shifter GPIO',
      shortLabel: configMode === 'dual_input' ? 'Forward Speed' : 'Forward Dir.',
      value: inputsConfig.forward,
      pinOptions: {
        pins: configMode === 'dual_input' && isAdcThrottle ? adc1Pins : digitalInputPins,
        groupLabel: configMode === 'dual_input' && isAdcThrottle ? 'ADC1 Pins' : 'Digital pins',
      },
      colorScheme: 'green',
      description:
        configMode === 'dual_input' ? 'Forward throttle input' : 'Forward position from shifter',
      validation:
        configMode === 'dual_input' && isAdcThrottle
          ? getValidationMessage(inputsConfig.forward, 'adc1', 'Forward Direction GPIO')
          : getValidationMessage(inputsConfig.forward, 'digital', 'Forward Shifter GPIO'),
      disabledPins: getDisabledPins(inputsConfig.forward),
    }

    // Backward input
    configs.backward = {
      key: 'backward',
      label: configMode === 'dual_input' ? 'Backward Speed GPIO' : 'Backward Shifter GPIO',
      shortLabel: configMode === 'dual_input' ? 'Backward Speed' : 'Backward Dir.',
      value: inputsConfig.backward,
      pinOptions: {
        pins: configMode === 'dual_input' && isAdcThrottle ? adc1Pins : digitalInputPins,
        groupLabel: configMode === 'dual_input' && isAdcThrottle ? 'ADC1 Pins' : 'Digital pins',
      },
      colorScheme: 'orange',
      description:
        configMode === 'dual_input' ? 'Reverse throttle input' : 'Reverse position from shifter',
      validation:
        configMode === 'dual_input' && isAdcThrottle
          ? getValidationMessage(inputsConfig.backward, 'adc1', 'Reverse Direction GPIO')
          : getValidationMessage(inputsConfig.backward, 'digital', 'Reverse Shifter GPIO'),
      disabledPins: getDisabledPins(inputsConfig.backward),
    }

    return configs
  })

  const outputPinConfigs = $derived({
    // Forward motor output
    forwardMotor: {
      key: 'forwardMotor',
      label: 'Forward Motor',
      shortLabel: 'Forward Motor',
      value: outputsConfig.forwardMotor,
      pinOptions: { pins: pwmPins, groupLabel: 'PWM Pins' },
      colorScheme: 'yellow',
      description: 'PWM signal for going forward',
      validation: null, // No validation needed for outputs
      disabledPins: getDisabledPins(outputsConfig.forwardMotor),
    },

    // Backward motor output
    backwardMotor: {
      key: 'backwardMotor',
      label: 'Backward Motor',
      shortLabel: 'Backward Motor',
      value: outputsConfig.backwardMotor,
      pinOptions: { pins: pwmPins, groupLabel: 'PWM Pins' },
      colorScheme: 'purple',
      description: 'PWM signal for going backward',
      validation: null, // No validation needed for outputs
      disabledPins: getDisabledPins(outputsConfig.backwardMotor),
    },
  })

  // Validation functions
  function isPinValid(pinValue, requiredPinType) {
    switch (requiredPinType) {
      case 'adc1':
        return adc1Pins.some((pin) => pin.value === pinValue)
      case 'digital':
        return digitalInputPins.some((pin) => pin.value === pinValue)
      case 'pwm':
        return pwmPins.some((pin) => pin.value === pinValue)
      default:
        return true
    }
  }

  function getValidationMessage(pinValue, requiredPinType, pinName) {
    if (isPinValid(pinValue, requiredPinType)) return null

    switch (requiredPinType) {
      case 'adc1':
        return `${pinName} must use an ADC1-compatible pin for dual input mode`
      case 'digital':
        return `${pinName} must use a digital input pin for speed/direction mode`
      default:
        return `${pinName} uses an incompatible pin type`
    }
  }

  const hasValidationErrors = $derived(
    Object.values(inputPinConfigs).some((config) => config.validation) ||
      Object.values(outputPinConfigs).some((config) => config.validation),
  )

  // Create bindable getters and setters for each pin
  const pinBindings = $derived.by(() => {
    return {
      throttle: {
        get value() {
          return inputsConfig.throttle
        },
        set value(v) {
          inputsConfig.throttle = v
        },
      },
      forward: {
        get value() {
          return inputsConfig.forward
        },
        set value(v) {
          inputsConfig.forward = v
        },
      },
      backward: {
        get value() {
          return inputsConfig.backward
        },
        set value(v) {
          inputsConfig.backward = v
        },
      },
      forwardMotor: {
        get value() {
          return outputsConfig.forwardMotor
        },
        set value(v) {
          outputsConfig.forwardMotor = v
        },
      },
      backwardMotor: {
        get value() {
          return outputsConfig.backwardMotor
        },
        set value(v) {
          outputsConfig.backwardMotor = v
        },
      },
    }
  })
</script>

<SectionHeader
  title="Wiring Configuration"
  subtitle="Configure input pins and control mode for your system"
/>

<div class="space-y-8">
  <Card.Root>
    <Card.Header>
      <Card.Title class="flex items-center gap-2">
        {#if settingsState.powerWheel?.setupMode}
          <LockOpen class="h-5 w-5 text-green-600" />
        {:else}
          <Lock class="h-5 w-5 text-orange-600" />
        {/if}
        Setup Mode
      </Card.Title>
      <Card.Description>
        {#if settingsState.powerWheel?.setupMode}
          Setup mode is enabled. You can now configure the wiring settings below.
        {:else}
          Enable setup mode to configure wiring settings. This will disable the car for safety.
        {/if}
      </Card.Description>
    </Card.Header>
    <Card.Content>
      <div class="flex items-center space-x-2">
        <Checkbox
          id="setup-mode"
          checked={settingsState.powerWheel?.setupMode}
          onCheckedChange={(checked) => sendMessage({ type: 'setup_mode', is_enabled: checked })}
        />
        <Label
          for="setup-mode"
          class="text-sm font-medium leading-none peer-disabled:cursor-not-allowed peer-disabled:opacity-70"
        >
          Enable Setup Mode
        </Label>
      </div>
    </Card.Content>
  </Card.Root>

  <DisabledSection
    isEnabled={settingsState.powerWheel?.setupMode}
    title="Configuration Locked"
    message="Enable setup mode above to edit these settings"
    class="space-y-2"
  >
    <!-- ADC Throttle Configuration -->
    <Card.Root>
      <Card.Header>
        <Card.Title class="flex items-center gap-2">
          <Activity class="h-5 w-5" />
          Throttle Input Configuration
        </Card.Title>
        <Card.Description>
          Configure how the throttle input should be read and calibrate thresholds
        </Card.Description>
      </Card.Header>
      <Card.Content class="space-y-6">
        <!-- Throttle Type Selection -->
        <div class="space-y-3">
          <div class="flex items-center space-x-3">
            <Checkbox id="adc-throttle" bind:checked={isAdcThrottle} />
            <Label
              for="adc-throttle"
              class="text-sm font-medium leading-none peer-disabled:cursor-not-allowed peer-disabled:opacity-70"
            >
              Use Analog Throttle Input
            </Label>
          </div>

          <p class="text-sm text-muted-foreground">
            {#if isAdcThrottle}
              Throttle will be read as an analog signal (0-3.3V range) for variable speed control.
            {:else}
              Throttle will be read as a digital signal (on/off) for basic speed control.
            {/if}
          </p>
        </div>

        <ThrottleReading {isAdcThrottle} {currentThrottle} bind:minThreshold bind:maxThreshold />
      </Card.Content>
    </Card.Root>

    <!-- Configuration Mode Selection -->
    <Card.Root>
      <Card.Header>
        <Card.Title>Control Mode</Card.Title>
        <Card.Description>
          Choose how you want to handle throttle input and shifter control
        </Card.Description>
      </Card.Header>
      <Card.Content>
        <RadioGroup.Root bind:value={configMode} class="space-y-4">
          <div class="space-y-2">
            <div class="flex items-center space-x-2">
              <RadioGroup.Item value="dual_input" id="dual_input" />
              <Label for="dual_input" class="font-medium">Dual Input Mode</Label>
              <Badge variant="secondary">Default</Badge>
            </div>
            <p class="text-sm text-muted-foreground ml-6">
              Input 1 for forward throttle, Input 2 for backward throttle. Non-zero values determine
              speed and direction.
            </p>
          </div>

          <div class="space-y-2">
            <div class="flex items-center space-x-2">
              <RadioGroup.Item value="speed_direction" id="speed_direction" />
              <Label for="speed_direction" class="font-medium">Speed + Direction Mode</Label>
            </div>
            <p class="text-sm text-muted-foreground ml-6">
              Input 1 controls throttle, Input 2 & 3 controls the shifter (Forward/Backward).
            </p>
          </div>
        </RadioGroup.Root>
      </Card.Content>
    </Card.Root>

    <!-- Mobile simplified view -->
    <MobileWiring
      {configMode}
      {inputPinConfigs}
      {outputPinConfigs}
      {pinBindings}
      {hasValidationErrors}
      {applyingConfig}
      {saveConfiguration}
    />

    <!-- Integrated Visual Configuration -->
    <DesktopWiring
      {isAdcThrottle}
      {configMode}
      {inputPinConfigs}
      {outputPinConfigs}
      {pinBindings}
      {hasValidationErrors}
      {applyingConfig}
      {saveConfiguration}
    />
  </DisabledSection>
</div>
