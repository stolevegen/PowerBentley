// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

let isConnected = false

var emergencyStop = false

export function generateMockResponse(data) {
  switch (data.type) {
    case 'ping':
      return [
        {
          type: 'pong',
        },
      ]

    case 'time_update':
      return [
        {
          type: 'time_update_response',
          success: true,
          current_time: Math.floor(Date.now() / 1000),
          formatted_time: new Date().toISOString(),
        },
      ]

    case 'wifi_scan':
      // Generate 3-6 random networks
      const numNetworks = Math.floor(Math.random() * 4) + 3 // 3 to 6
      const baseNames = ['Mock_Network', 'Test_AP', 'ESP32', 'OfficeNet', 'CafeWiFi', 'IoTNet']
      const networks = Array.from({ length: numNetworks }, (_, i) => ({
        ssid: `${baseNames[i % baseNames.length]}`,
        rssi: -30 - Math.floor(Math.random() * 60), // -30 to -89
        secure: i < 3,
      }))
      return [
        {
          type: 'wifi_list',
          networks,
        },
      ]

    case 'wifi_status':
      return [
        {
          type: 'wifi_status',
          status: isConnected ? wifiConnected : wifiDisconnected,
        },
      ]

    case 'wifi_connect':
      if (data.password === 'password') {
        isConnected = true
        settings.wifi.setup = true
        settings.wifi.connected = true
        return [
          {
            type: 'wifi_status',
            status: wifiConnected,
          },
          {
            type: 'settings',
            ...settings,
          },
        ]
      } else {
        return [
          { type: 'error', message: 'Password incorrect! Can you guess what my "password" is?' },
        ]
      }

    case 'wifi_disconnect':
      isConnected = false
      settings.wifi.setup = false
      settings.wifi.connected = false

      return [
        {
          type: 'wifi_status',
          status: wifiDisconnected,
        },
        {
          type: 'settings',
          ...settings,
        },
      ]

    case 'get_system_info':
      return [
        {
          type: 'system_info',
          settings: systemInfo,
        },
      ]

    case 'get_settings':
      return [
        {
          type: 'settings',
          ...settings,
        },
      ]

    case 'setup_mode':
      settings.powerWheel.setupMode = data.is_enabled
      return [
        {
          type: 'settings',
          ...settings,
        },
      ]

    case 'get_wiring':
      return [getWiringResponse()]

    case 'set_wiring':
      configMode = data.mode
      isAdcThrottle = data.is_adc_throttle
      forward = parseInt(data.inputs.forward, 10)
      backward = parseInt(data.inputs.backward, 10)
      throttle = data.inputs.throttle ? parseInt(data.inputs.throttle, 10) : null
      forwardMotor = data.outputs.forward_motor
      backwardMotor = data.outputs.backward_motor

      return [getWiringResponse()]

    case 'set_current_profile':
      current_profile_id = data.profile_id

      return [
        {
          type: 'read_all_response',
          current_speed: settings.powerWheel.setupMode ? 0 : getFakeSpeed(),
          current_profile_id: current_profile_id,
          emergency_stop: emergencyStop,
        },
      ]

    case 'emergency_stop':
      emergencyStop = data.is_enabled

      return [
        {
          type: 'read_all_response',
          current_speed: settings.powerWheel.setupMode ? 0 : getFakeSpeed(),
          current_profile_id: current_profile_id,
          emergency_stop: emergencyStop,
        },
      ]

    case 'read_all':
      return [
        {
          type: 'read_all_response',
          current_speed: settings.powerWheel.setupMode ? 0 : getFakeSpeed(),
          current_profile_id: current_profile_id,
          emergency_stop: emergencyStop,
        },
      ]

    case 'read_throttle':
      return [
        {
          type: 'read_throttle_response',
          current_throttle: (getFakeSpeed() / 100.0) * 3.3,
        },
      ]

    case 'get_profiles':
      return [
        {
          type: 'profiles_data',
          profiles: Object.values(profiles),
        },
      ]

    case 'save_profile':
      profiles[data.profile.id] = data.profile
      return [
        {
          type: 'profiles_data',
          profiles: Object.values(profiles),
        },
      ]

    case 'delete_profile':
      if (current_profile_id === data.profile_id) {
        return [
          {
            type: 'error',
            message: 'Cannot delete the current profile. Please select another profile first.',
          },
        ]
      }

      delete profiles[data.profile_id]

      return [
        {
          type: 'profiles_data',
          profiles: Object.values(profiles),
        },
      ]
  }
}

// Wiring configuration
var configMode = 'dual_input' // 'dual_input' or 'speed_direction'
var forward = '32'
var backward = '33'
var throttle = '34'
var isAdcThrottle = false // Whether throttle is analog (ADC) or digital
var forwardMotor = '18'
var backwardMotor = '19'
var minThreshold = '1.0'
var maxThreshold = '2.6'

function getWiringResponse() {
  if (configMode === 'dual_input') {
    return {
      type: 'wiring_response',
      mode: configMode,
      is_adc_throttle: isAdcThrottle,
      min_threshold: minThreshold,
      max_threshold: maxThreshold,
      inputs: { forward: forward, backward: backward },
      outputs: { forward_motor: forwardMotor, backward_motor: backwardMotor },
    }
  } else {
    return {
      type: 'wiring_response',
      mode: configMode,
      is_adc_throttle: isAdcThrottle,
      min_threshold: minThreshold,
      max_threshold: maxThreshold,
      inputs: { forward: forward, backward: backward, throttle: throttle },
      outputs: { forward_motor: forwardMotor, backward_motor: backwardMotor },
    }
  }
}

var current_profile_id = 'default'

let profiles = {
  default: {
    id: 'default',
    name: 'Default',
    maxForward: 60,
    maxBackward: 20,
    isDragMode: false,
  },
  high_performance: {
    id: 'high_performance',
    name: 'High Performance',
    maxForward: 100,
    maxBackward: 40,
    isDragMode: true,
  },
  eco_mode: {
    id: 'eco_mode',
    name: 'Eco Mode',
    maxForward: 50,
    maxBackward: 15,
    isDragMode: false,
  },
}

let settings = {
  ota: {
    requiresPassword: true,
  },
  wifi: {
    connected: false,
    setup: false,
  },
  powerWheel: {
    setupMode: false,
  },
}

let wifiDisconnected = {
  mode: 'AP+STA',
  mac: '12:34:56:78:90:ab',
  sta: { connected: false, configured_ssid: '' },
  ap: {
    ssid: 'PowerJeep',
    channel: 10,
    auth_mode: 'WPA_WPA2_PSK',
    ip: '192.168.4.1',
    mac: 'ab:cd:ef:12:34:56',
    connected_stations: 1,
    max_connections: 2,
  },
}

let wifiConnected = {
  mode: 'STA',
  mac: '12:34:56:78:90:ab',
  sta: {
    connected: true,
    ssid: 'Wi Believe I Can Fi',
    rssi: -48,
    channel: 8,
    auth_mode: 'WPA2_PSK',
    ip: '192.168.1.10',
    gateway: '192.168.1.1',
    netmask: '255.255.255.0',
  },
}

let systemInfo = {
  device: {
    status: 'Online and operational',
    reset_reason: 'Power-on reset',
    uptime: '5 minutes, 42seconds',
    time: new Date().toLocaleString('en-US'),
  },
  system: { idf_version: '5.4.0', freertos_tasks: 12 },
  hardware: {
    chip_model: 'ESP32',
    chip_revision: 301,
    cpu_cores: 2,
    flash_size: '4.0 MB',
  },
  memory: {
    heap_total: '269.8 KB',
    heap_free: '163.2 KB',
    heap_used: '106.6 KB',
    heap_usage: '39%',
    heap_largest_free_block: '108.0 KB',
    heap_min_free_ever: '145.4 KB',
    internal_total: '302.0 KB',
    internal_free: '194.6 KB',
    internal_usage: '35%',
  },
  psram: { psram_total: '0 bytes', psram_free: '0 bytes', psram_usage: '0%' },
  spiffs: {
    status: 'Mounted and operational',
    partition_size: '960.0 KB',
    partition_label: 'storage',
    partition_address: '0x310000',
    total_space: '875.3 KB',
    used_space: '171.3 KB',
    free_space: '704.0 KB',
    usage: '19%',
    files_count: 4,
    total_size: '168.8 KB',
  },
  tasks: {
    status: 'OK',
    count: 4,
    list: [
      {
        name: 'httpd',
        priority: 5,
        state: 'Running',
        stack_hwm_words: 64,
        stack_hwm_bytes: '256 B',
        task_number: 11,
        core_id: 0,
      },
      {
        name: 'IDLE0',
        priority: 0,
        state: 'Ready',
        stack_hwm_words: 1688,
        stack_hwm_bytes: '6.6 KB',
        task_number: 6,
        core_id: 0,
      },
      {
        name: 'IDLE1',
        priority: 0,
        state: 'Ready',
        stack_hwm_words: 1692,
        stack_hwm_bytes: '6.6 KB',
        task_number: 5,
        core_id: 1,
      },
      {
        name: 'broadcast_task',
        priority: 3,
        state: 'Blocked',
        stack_hwm_words: 128, // Lower HWM indicates this task used more stack
        stack_hwm_bytes: '512 B',
        task_number: 12,
        core_id: 0,
      },
    ],
  },
}

export function getFakeSpeed(timeValue = Date.now() / 1000) {
  const clampedMaxSpeed = Math.max(20, Math.min(100, profiles[current_profile_id].maxForward))

  // Create pseudo-random but deterministic behavior based on time (faster)
  const t1 = timeValue * 0.4
  const t2 = timeValue * 0.2
  const t3 = timeValue * 0.08

  // Combine multiple sine waves for complex behavior
  const baseSpeed = (Math.sin(t1) + 1) * 0.5 * clampedMaxSpeed
  const variation1 = Math.sin(t2 * 2.3) * 0.2 * clampedMaxSpeed
  const variation2 = Math.sin(t3 * 3.7) * 0.15 * clampedMaxSpeed
  const randomness = Math.sin(timeValue * 0.3) * Math.sin(timeValue * 0.5) * 0.1 * clampedMaxSpeed

  let speed = baseSpeed + variation1 + variation2 + randomness

  // Add occasional "braking" events (more frequent)
  if (Math.sin(timeValue * 0.12) > 0.95) {
    speed *= 0.3 // Hard brake
  }

  // Keep within bounds
  speed = Math.max(0, Math.min(clampedMaxSpeed, speed))

  return Math.round(speed * 10) / 10
}
