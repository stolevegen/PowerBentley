<!-- Copyright (c) 2025 David Bertet. Licensed under the MIT License. -->

<script>
  import { onMount, onDestroy } from 'svelte'
  import * as Card from '$lib/components/ui/card/index.js'
  import { Button } from '$lib/components/ui/button/index.js'
  import { Slider } from '$lib/components/ui/slider/index.js'
  import * as Select from '$lib/components/ui/select'
  import { Skeleton } from '$lib/components/ui/skeleton'
  import { sendMessage, onMessageType } from 'src/lib/ws.svelte.js'
  import { Settings2, Save, Check, RefreshCw, Zap, ArrowUp, ArrowDown } from 'lucide-svelte'
  import { cn } from '$lib/utils.js'
  import { settingsState } from 'src/lib/settings.svelte.js'

  import SpeedGauge from 'src/components/jeep/SpeedGauge.svelte'
  import DisabledSection from 'src/components/common/DisabledSection.svelte'

  let emergencyStop = $state(false)
  let currentSpeed = $state(null)

  let profiles = $state([])
  let selectedProfileId = $state('')
  let selectedProfile = $derived(profiles.find((c) => c.id === selectedProfileId))

  let loading = $state(false)
  let applyingProfile = $state(false)
  let applyingEmergencyStop = $state(false)

  // WebSocket unsubscribe function
  let readAllUnsub = $state(null)
  let readSpeedUnsub = $state(null)
  let profilesUnsub = $state(null)

  function loadProfiles() {
    loading = true

    sendMessage({ type: 'get_profiles' })
  }

  function selectProfile(profileId) {
    if (applyingProfile || selectedProfileId === profileId) return

    selectedProfileId = profileId

    applyingProfile = true

    sendMessage({
      type: 'set_current_profile',
      profile_id: profileId,
    })
  }

  onMount(() => {
    loading = true

    profilesUnsub = onMessageType('profiles_data', (data) => {
      profiles = data.profiles || []
      loading = false
    })

    readAllUnsub = onMessageType('read_all_response', (data) => {
      applyingEmergencyStop = false
      applyingProfile = false

      selectedProfileId = data.current_profile_id || profiles[0].id
      emergencyStop = data.emergency_stop
      currentSpeed = Math.abs(data.current_speed)
    })

    readSpeedUnsub = onMessageType('read_speed_response', (data) => {
      currentSpeed = Math.abs(data.current_speed)
    })

    loadProfiles()
    sendMessage({ type: 'read_all' })

    return () => {
      if (readAllUnsub) readAllUnsub()
      if (readSpeedUnsub) readSpeedUnsub()
      if (profilesUnsub) profilesUnsub()
    }
  })

  onDestroy(() => {
    if (readAllUnsub) readAllUnsub()
    if (readSpeedUnsub) readSpeedUnsub()
    if (profilesUnsub) profilesUnsub()
  })

  function toggleEmergencyStop() {
    if (applyingEmergencyStop) return
    applyingEmergencyStop = true
    sendMessage({ type: 'emergency_stop', is_enabled: !emergencyStop })
  }
</script>

<DisabledSection
  isEnabled={!settingsState.powerWheel?.setupMode}
  title="Setup Mode enabled"
  message="Car is disabled for safety. Finish setup in Wiring tab"
>
  <div>
    <div class="max-w-2xl mx-auto space-y-6">
      <div>
        <div class="relative">
          <SpeedGauge value={currentSpeed} isEmergencyStop={emergencyStop} />
          {#if selectedProfile?.isDragMode}
            <Zap class="absolute right-0 bottom-8 size-6" />
          {/if}
        </div>

        <!-- Emergency Stop Button -->
        <Button
          onclick={toggleEmergencyStop}
          size="lg"
          class="relative w-full text-lg p-7 font-bold {emergencyStop
            ? 'bg-green-600 hover:bg-green-700'
            : ''}"
        >
          {emergencyStop ? 'Restart' : 'Emergency Stop'}
          {#if applyingEmergencyStop}
            <RefreshCw class="absolute right-5 h-5 w-5 ml-2 animate-spin" />
          {/if}
        </Button>
      </div>

      <Card.Root class="bg-card text-foreground gap-2 pt-4">
        <Card.Header>
          <div class="flex items-center justify-between">
            <Card.Title>Choose Profile</Card.Title>
          </div>
        </Card.Header>
        <Card.Content class="space-y-2">
          {#if profiles.length > 0}
            <div class="space-y-2">
              <div class="grid grid-cols-1 gap-2">
                {#each profiles as profile (profile.id)}
                  <Button
                    variant={selectedProfileId === profile.id ? 'default' : 'outline'}
                    class="w-full h-auto p-4 justify-between"
                    onclick={() => selectProfile(profile.id)}
                    disabled={(applyingProfile && selectedProfileId !== profile.id) ||
                      applyingEmergencyStop}
                  >
                    <div class="flex flex-col items-start space-y-1">
                      <div class="flex items-center space-x-2">
                        <span class="font-medium">{profile.name}</span>
                      </div>
                      <div
                        class={cn(
                          'flex text-xs',
                          selectedProfileId === profile.id ? '' : 'text-muted-foreground',
                        )}
                      >
                        <ArrowUp class="size-4" />
                        {profile.maxForward}% • <ArrowDown class="size-4 ps-0.5" />
                        {profile.maxBackward}%
                        {#if profile.isDragMode}
                          • <Zap class="size-4 ps-0.5" /> Drag mode
                        {/if}
                      </div>
                    </div>

                    {#if selectedProfileId === profile.id}
                      {#if applyingProfile}
                        <RefreshCw class="h-5 w-5 mr-2 animate-spin" />
                      {:else}
                        <Check class="h-5 w-5 mr-2" />
                      {/if}
                    {/if}
                  </Button>
                {/each}
              </div>
            </div>
          {:else if loading}
            <!-- Loading State -->
            <div class="space-y-2">
              {#each Array(2) as _}
                <Skeleton class="h-18.5 w-full border-1" />
              {/each}
            </div>
          {:else}
            <!-- No profiles -->
            <div class="p-6 text-center bg-muted/30 rounded-lg border-2 border-dashed">
              <Settings2 class="h-8 w-8 text-muted-foreground mx-auto mb-2" />
              <p class="text-sm text-muted-foreground mb-3">No profiles available</p>
              <Button onclick={loadProfiles} variant="outline" size="sm">
                <RefreshCw class="h-4 w-4 mr-2" />
                Load Profiles
              </Button>
            </div>
          {/if}
        </Card.Content>
      </Card.Root>
    </div>
  </div>
</DisabledSection>
