// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

import { onMessageType, sendMessage } from 'src/lib/ws.svelte.js'

// Settings state
export const settingsState = $state({})

// Initialize settings
export function initializeSettings() {
  // Request settings from server
  sendMessage({ type: 'get_settings' })

  // Handle settings response
  const unsubscribe = onMessageType('settings', (data) => {
    settingsState.ota = data.ota
    settingsState.wifi = data.wifi
    settingsState.powerWheel = data.powerWheel
  })

  return unsubscribe
}
