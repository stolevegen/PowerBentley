<!-- Copyright (c) 2025 David Bertet. Licensed under the MIT License. -->

<script>
  import { onMount, onDestroy, tick } from 'svelte'
  import { slide } from 'svelte/transition'
  import { sendMessage, onMessageType } from 'src/lib/ws.svelte.js'
  import * as Card from '$lib/components/ui/card'
  import * as Select from '$lib/components/ui/select'
  import * as Alert from '$lib/components/ui/alert'
  import { Button } from '$lib/components/ui/button'
  import { Checkbox } from '$lib/components/ui/checkbox'
  import { Input } from '$lib/components/ui/input'
  import { Label } from '$lib/components/ui/label'
  import { Slider } from '$lib/components/ui/slider'
  import { Separator } from '$lib/components/ui/separator'
  import { Badge } from '$lib/components/ui/badge'
  import { Skeleton } from '$lib/components/ui/skeleton'
  import {
    Trash2,
    Plus,
    Settings2,
    Save,
    RefreshCw,
    Edit2,
    Check,
    X,
    AlertTriangle,
    Zap,
  } from 'lucide-svelte'

  import SectionHeader from 'src/components/common/SectionHeader.svelte'

  // State management
  let profiles = $state([])
  let selectedProfileId = $state('default')
  let selectedProfile = $derived(profiles.find((c) => c.id === selectedProfileId) || profiles[0])
  let newProfileName = $state('')
  let isValidnewProfileName = $derived(
    newProfileName.trim() &&
      profiles.every((c) => c.id !== newProfileName.toLowerCase().replace(/\s+/g, '_')),
  )
  let isAddingProfile = $state(false)
  let hasUnsavedChanges = $state(false)
  let loading = $state(true)
  let saving = $state(false)

  // Name editing state
  let editingProfileId = $state(null)
  let editingName = $state('')

  // WebSocket unsubscribe functions
  let profilesUnsub = $state(null)
  let saveUnsub = $state(null)
  let deleteUnsub = $state(null)

  // Track changes for unsaved indicator
  let originalValues = $state({})

  $effect(() => {
    if (selectedProfile) {
      const current = {
        maxForward: selectedProfile.maxForward,
        maxBackward: selectedProfile.maxBackward,
        isDragMode: selectedProfile.isDragMode,
      }
      const original = originalValues[selectedProfileId]

      if (original) {
        hasUnsavedChanges =
          !isAddingProfile &&
          (current.maxForward !== original.maxForward ||
            current.maxBackward !== original.maxBackward ||
            current.isDragMode !== original.isDragMode)
      }
    }
  })

  function selectProfile(profileId) {
    if (selectedProfileId === profileId) {
      selectedProfile.maxForward = originalValues[selectedProfileId].maxForward
      selectedProfile.maxBackward = originalValues[selectedProfileId].maxBackward
      selectedProfile.isDragMode = originalValues[selectedProfileId].isDragMode
      return
    }

    // Store original values when switching
    if (selectedProfile) {
      originalValues[selectedProfileId] = {
        maxForward: selectedProfile.maxForward,
        maxBackward: selectedProfile.maxBackward,
        isDragMode: selectedProfile.isDragMode,
      }
    }

    selectedProfileId = profileId
    hasUnsavedChanges = false
  }

  function startAddingProfile() {
    // Create a temporary profile ID and immediately select it
    const tempId = `temp_${Date.now()}`
    const tempProfile = {
      id: tempId,
      name: '',
      maxForward: 50,
      maxBackward: 25,
      isDragMode: false,
      isTemporary: true,
    }

    // Add to profiles temporarily and select it
    profiles = [...profiles, tempProfile]
    selectedProfileId = tempId

    // Set up original values for the temp profile
    originalValues[tempId] = {
      maxForward: 50,
      maxBackward: 25,
      isDragMode: false,
    }

    isAddingProfile = true
    hasUnsavedChanges = false
  }

  // Focus effect for new profile input
  $effect(() => {
    if (isAddingProfile) {
      // Use querySelector to find the input element directly
      tick().then(() => {
        const inputElement = document.querySelector('[placeholder="Profile name"]')
        if (inputElement && typeof inputElement.focus === 'function') {
          inputElement.focus()
        }
      })
    }
  })

  function addProfile() {
    if (!isValidnewProfileName) return

    const newId = newProfileName.toLowerCase().replace(/\s+/g, '_')

    // Update the temporary profile to be permanent
    const tempProfile = profiles.find((c) => c.isTemporary)
    if (tempProfile) {
      // Remove the temporary profile
      profiles = profiles.filter((c) => !c.isTemporary)
      delete originalValues[tempProfile.id]

      // Create the real profile
      const newProfile = {
        id: newId,
        name: newProfileName.trim(),
        maxForward: tempProfile.maxForward, // Use current slider values
        maxBackward: tempProfile.maxBackward,
        isDragMode: tempProfile.isDragMode,
      }

      // Save the new profile immediately
      saving = true
      sendMessage({
        type: 'save_profile',
        profile: newProfile,
      })

      profiles = [...profiles, newProfile]
      selectedProfileId = newId

      // Store values for the real profile
      originalValues[newId] = {
        maxForward: newProfile.maxForward,
        maxBackward: newProfile.maxBackward,
        isDragMode: newProfile.isDragMode,
      }
    }

    newProfileName = ''
    isAddingProfile = false
    hasUnsavedChanges = false
  }

  function startEditingName(profileId, currentName) {
    editingProfileId = profileId
    editingName = currentName
  }

  function cancelEditingName() {
    editingProfileId = null
    editingName = ''
  }

  function saveProfileName(profileId) {
    if (!editingName.trim()) {
      cancelEditingName()
      return
    }

    const profile = profiles.find((c) => c.id === profileId)
    if (profile) {
      const oldName = profile.name
      profile.name = editingName.trim()

      // Send update to server
      saving = true
      sendMessage({
        type: 'save_profile',
        profile: profile,
      })
    }

    editingProfileId = null
    editingName = ''
  }

  function handleNameKeydown(e, profileId) {
    if (e.key === 'Enter') {
      saveProfileName(profileId)
    } else if (e.key === 'Escape') {
      cancelEditingName()
    }
  }

  function deleteProfile(profileId) {
    sendMessage({
      type: 'delete_profile',
      profile_id: profileId,
    })
  }

  function saveProfile() {
    if (selectedProfile) {
      saving = true
      sendMessage({
        type: 'save_profile',
        profile: selectedProfile,
      })
    }
  }

  function loadProfiles() {
    loading = true
    sendMessage({ type: 'get_profiles' })
  }

  function updateSliderValue(type, value) {
    if (selectedProfile) {
      selectedProfile[type] = Array.isArray(value) ? value[0] : value
    }
  }

  function handleKeydown(e) {
    if (e.key === 'Enter') {
      addProfile()
    } else if (e.key === 'Escape') {
      cancelAddProfile()
    }
  }

  function cancelAddProfile() {
    // Remove any temporary profile
    const tempProfile = profiles.find((c) => c.isTemporary)
    if (tempProfile) {
      profiles = profiles.filter((c) => !c.isTemporary)
      delete originalValues[tempProfile.id]
    }

    // Restore previous selection
    selectedProfileId = profiles[profiles.length - 1].id

    isAddingProfile = false
    newProfileName = ''
    hasUnsavedChanges = false
  }

  onMount(() => {
    // Set up WebSocket listeners
    profilesUnsub = onMessageType('profiles_data', (data) => {
      profiles = data.profiles || []
      loading = false
      hasUnsavedChanges = false
      saving = false

      // Initialize original values
      profiles.forEach((profile) => {
        originalValues[profile.id] = {
          maxForward: profile.maxForward,
          maxBackward: profile.maxBackward,
          isDragMode: profile.isDragMode,
        }
      })

      // Set selected profile to first available if current selection doesn't exist
      if (!profiles.find((c) => c.id === selectedProfileId)) {
        selectedProfileId = profiles[0]?.id || 'default'
      }
    })

    loadProfiles()

    return () => {
      if (profilesUnsub) profilesUnsub()
      if (saveUnsub) saveUnsub()
      if (deleteUnsub) deleteUnsub()
    }
  })

  onDestroy(() => {
    if (profilesUnsub) profilesUnsub()
    if (saveUnsub) saveUnsub()
    if (deleteUnsub) deleteUnsub()
  })
</script>

<SectionHeader title="Profile Settings" subtitle="Manage driving profiles" />

<div class="space-y-6">
  <!-- Profile Selector -->
  <Card.Root>
    <Card.Header>
      <div class="space-y-3 sm:space-y-0">
        <!-- Title Row -->
        <div class="flex items-center justify-between">
          <div class="flex items-center space-x-2">
            <Settings2 class="h-5 w-5 text-muted-foreground" />
            <Card.Title class="text-lg sm:text-xl">Existing Profiles</Card.Title>
          </div>
          <!-- Desktop: All buttons in header, Mobile: Only refresh -->
          <div class="flex items-center gap-2">
            <Button
              onclick={loadProfiles}
              variant="ghost"
              size="sm"
              class="h-8 shrink-0"
              disabled={loading}
            >
              <RefreshCw class="h-4 w-4 {loading ? 'animate-spin' : ''}" />
            </Button>
            <!-- Desktop buttons -->
            <div class="hidden sm:flex items-center">
              {#if hasUnsavedChanges}
                <div class="relative" transition:slide={{ duration: 250, axis: 'x' }}>
                  <Button onclick={saveProfile} size="sm" class="h-8 mr-2" disabled={saving}>
                    <Save class="h-4 w-4 mr-1" />
                    Save
                  </Button>
                  {#if saving}
                    <RefreshCw class="absolute inset-0 m-auto h-5 w-5 animate-spin" />
                  {/if}
                </div>
              {/if}
              <Button
                onclick={startAddingProfile}
                variant="outline"
                size="sm"
                class="h-8"
                disabled={loading}
              >
                <Plus class="h-4 w-4 mr-1" />
                New Profile
              </Button>
            </div>
          </div>
        </div>

        <!-- Mobile: Status badge and action buttons -->
        <div class="sm:hidden space-y-3">
          <div class="flex items-center gap-2">
            <div class="relative flex-1">
              <Button
                onclick={saveProfile}
                size="sm"
                class="h-8 w-full"
                disabled={saving || !hasUnsavedChanges}
              >
                <Save class="h-4 w-4 mr-1" />
                Save
              </Button>
              {#if saving}
                <RefreshCw class="absolute inset-0 m-auto h-5 w-5 animate-spin" />
              {/if}
            </div>

            <div class="flex-1">
              <Button
                onclick={startAddingProfile}
                variant="outline"
                size="sm"
                class="h-8 w-full"
                disabled={loading}
              >
                <Plus class="h-4 w-4 mr-1" />
                New
              </Button>
            </div>
          </div>
        </div>
      </div>
    </Card.Header>
    <Card.Content class="space-y-4">
      {#if loading}
        <div class="flex flex-wrap gap-2">
          {#each Array(3) as _}
            <Skeleton class="h-8 w-24" />
          {/each}
        </div>
        <Separator />
        <div class="flex items-center justify-between">
          <div class="flex items-center space-x-2">
            <span class="text-sm text-muted-foreground hidden sm:inline">Selected:</span>
            <Skeleton class="h-6 w-20" />
          </div>
          <div class="flex items-center space-x-2">
            <Skeleton class="h-7 w-18" />
            <Skeleton class="h-7 w-20" />
          </div>
        </div>
      {:else}
        <div class="space-y-4">
          <!-- Profile Selection Buttons -->
          <div class="flex flex-wrap gap-2">
            {#each profiles as profile (profile.id)}
              {#if profile.isTemporary && isAddingProfile}
                <!-- New profile input mode -->
                <div class="flex items-center border rounded-md overflow-hidden bg-muted/30">
                  <div class="flex items-center px-3 py-1 bg-primary border-r">
                    <Input
                      bind:value={newProfileName}
                      placeholder="Profile name"
                      class="h-6 px-1 py-0 text-sm shadow-none focus-visible:border-none focus-visible:ring-[0px] border-none text-primary-foreground placeholder:text-[#bbb] font-bold placeholder:font-normal bg-primary dark:bg-primary"
                      onkeydown={handleKeydown}
                    />
                  </div>

                  <!-- Action buttons -->
                  <div class="flex">
                    <Button
                      variant="ghost"
                      size="sm"
                      onclick={addProfile}
                      disabled={!isValidnewProfileName}
                      class="h-8 w-8 p-0 rounded-none text-green-600 hover:text-green-700 hover:bg-green-50 disabled:opacity-50"
                      title="Add profile"
                    >
                      <Check class="h-3 w-3" />
                    </Button>

                    <Button
                      variant="ghost"
                      size="sm"
                      onclick={cancelAddProfile}
                      class="h-8 w-8 p-0 rounded-none text-muted-foreground hover:text-foreground hover:bg-muted border-l"
                      title="Cancel"
                    >
                      <X class="h-3 w-3" />
                    </Button>
                  </div>
                </div>
              {:else if !profile.isTemporary}
                <!-- Regular selection button (no action buttons here) -->
                <Button
                  variant={selectedProfileId === profile.id ? 'default' : 'outline'}
                  size="sm"
                  onclick={() => selectProfile(profile.id)}
                  class="h-8"
                  disabled={isAddingProfile ||
                    (selectedProfileId !== profile.id && hasUnsavedChanges)}
                >
                  {profile.name}
                </Button>
              {/if}
            {/each}
          </div>

          <!-- Edit/Delete controls for selected profile -->
          {#if selectedProfile && !selectedProfile.isTemporary && !isAddingProfile}
            <Separator />
            <div class="flex items-center justify-between">
              <div class="flex items-center space-x-2">
                <span class="text-sm text-muted-foreground hidden sm:inline">Selected:</span>
                {#if editingProfileId === selectedProfile.id}
                  <!-- Inline editing mode -->
                  <div class="flex items-center space-x-2">
                    <Input
                      bind:value={editingName}
                      class="h-7 px-2 py-1 text-sm min-w-32"
                      onkeydown={(e) => handleNameKeydown(e, selectedProfile.id)}
                      autofocus
                    />
                    <Button
                      variant="ghost"
                      size="sm"
                      onclick={() => saveProfileName(selectedProfile.id)}
                      class="h-7 w-7 p-0 text-green-600 hover:text-green-700"
                      title="Save name"
                    >
                      <Check class="h-3 w-3" />
                    </Button>
                    <Button
                      variant="ghost"
                      size="sm"
                      onclick={cancelEditingName}
                      class="h-7 w-7 p-0 text-muted-foreground hover:text-foreground"
                      title="Cancel"
                    >
                      <X class="h-3 w-3" />
                    </Button>
                  </div>
                {:else}
                  <span class="font-medium">{selectedProfile.name}</span>
                {/if}
              </div>

              {#if editingProfileId !== selectedProfile.id}
                <div class="flex items-center space-x-1">
                  <Button
                    variant="ghost"
                    size="sm"
                    onclick={() => startEditingName(selectedProfile.id, selectedProfile.name)}
                    class="h-7 text-muted-foreground hover:text-foreground"
                    title="Edit name"
                  >
                    <Edit2 class="h-3 w-3 mr-1" />
                    Edit
                  </Button>

                  {#if profiles.filter((c) => !c.isTemporary).length > 1}
                    <Button
                      variant="ghost"
                      size="sm"
                      onclick={() => deleteProfile(selectedProfile.id)}
                      class="h-7 text-muted-foreground hover:text-destructive"
                      title="Delete profile"
                    >
                      <Trash2 class="h-3 w-3 mr-1" />
                      Delete
                    </Button>
                  {/if}
                </div>
              {/if}
            </div>
          {/if}
        </div>
      {/if}

      <!-- Profile Parameters -->
      {#if loading}
        <div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
          {#each Array(2) as _}
            <Card.Root>
              <Card.Header>
                <Skeleton class="h-6 w-32" />
                <Skeleton class="h-4 w-68 mt-2" />
              </Card.Header>
              <Card.Content class="space-y-6">
                <div class="space-y-3">
                  <div class="flex items-center justify-between">
                    <Skeleton class="h-4 w-12" />
                    <Skeleton class="h-6 w-8" />
                  </div>
                  <Skeleton class="h-2 w-full" />
                  <div class="flex justify-between">
                    <Skeleton class="h-4 w-4" />
                    <Skeleton class="h-4 w-4" />
                    <Skeleton class="h-4 w-4" />
                  </div>
                </div>
              </Card.Content>
            </Card.Root>
          {/each}
        </div>
      {:else if selectedProfile}
        <div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
          <!-- Max Forward -->
          <Card.Root class="transition-shadow hover:shadow-md">
            <Card.Header>
              <Card.Title>Max Forward</Card.Title>
              <Card.Description>Set the maximum forward speed percentage</Card.Description>
            </Card.Header>
            <Card.Content class="space-y-6">
              <div class="space-y-3">
                <div class="flex items-center justify-between">
                  <Label for="max-forward" class="text-sm font-medium">Value</Label>
                  <div class="flex items-center space-x-2">
                    <Badge variant="outline" class="font-mono">
                      {selectedProfile.maxForward}
                    </Badge>
                  </div>
                </div>
                <Slider
                  id="max-forward"
                  min={20}
                  max={100}
                  step={1}
                  value={[selectedProfile.maxForward]}
                  onValueChange={(value) => updateSliderValue('maxForward', value)}
                  class="w-full"
                />
                <div class="flex justify-between text-xs text-muted-foreground">
                  <span>20</span>
                  <span>60</span>
                  <span>100</span>
                </div>
              </div>
            </Card.Content>
          </Card.Root>

          <!-- Max Backward -->
          <Card.Root class="transition-shadow hover:shadow-md">
            <Card.Header>
              <Card.Title>Max Backward</Card.Title>
              <Card.Description>Set the maximum backward speed percentage</Card.Description>
            </Card.Header>
            <Card.Content class="space-y-6">
              <div class="space-y-3">
                <div class="flex items-center justify-between">
                  <Label for="max-backward" class="text-sm font-medium">Value</Label>
                  <div class="flex items-center space-x-2">
                    <Badge variant="outline" class="font-mono">
                      {selectedProfile.maxBackward}
                    </Badge>
                  </div>
                </div>
                <Slider
                  id="max-backward"
                  min={10}
                  max={40}
                  step={1}
                  value={[selectedProfile.maxBackward]}
                  onValueChange={(value) => updateSliderValue('maxBackward', value)}
                  class="w-full"
                />
                <div class="flex justify-between text-xs text-muted-foreground">
                  <span>10</span>
                  <span>25</span>
                  <span>40</span>
                </div>
              </div>
            </Card.Content>
          </Card.Root>
        </div>

        <Card.Root>
          <Card.Header>
            <Card.Title class="flex items-center gap-2"><Zap class="size-4" />Drag Mode</Card.Title>
            <Card.Description>Enable this mode to make the car more responsive.</Card.Description>
          </Card.Header>
          <Card.Content>
            <div class="flex items-center space-x-2 pb-4">
              <Checkbox
                id="setup-mode"
                checked={selectedProfile.isDragMode}
                onCheckedChange={(checked) => (selectedProfile.isDragMode = checked)}
              />
              <Label
                for="setup-mode"
                class="text-sm font-medium leading-none peer-disabled:cursor-not-allowed peer-disabled:opacity-70"
              >
                Enable Drag Mode
              </Label>
            </div>

            {#if selectedProfile.isDragMode}
              <Alert.Root
                class="border-red-200 dark:border-red-400/40 bg-red-50 dark:bg-red-900/10"
              >
                <AlertTriangle class="h-4 w-4 !text-red-600" />
                <Alert.Description class="text-red-800 dark:text-red-700">
                  Drag mode increases acceleration and may damage gearbox/drivetrain.
                </Alert.Description>
              </Alert.Root>
            {/if}
          </Card.Content>
        </Card.Root>
      {:else if !loading && profiles.length > 0}
        <!-- No profile selected state -->
        <Card.Root>
          <Card.Content class="flex items-center justify-center py-12">
            <div class="text-center">
              <Settings2 class="h-12 w-12 text-muted-foreground mx-auto mb-4" />
              <p class="text-muted-foreground mb-2">No profile selected</p>
              <p class="text-sm text-muted-foreground">
                Select a profile above to view and edit its parameters
              </p>
            </div>
          </Card.Content>
        </Card.Root>
      {:else if !loading}
        <Card.Root>
          <Card.Content class="flex items-center justify-center py-12">
            <div class="text-center">
              <Settings2 class="h-12 w-12 text-muted-foreground mx-auto mb-4" />
              <p class="text-muted-foreground">No profiles available</p>
              <Button onclick={loadProfiles} variant="outline" size="sm" class="mt-4">
                <RefreshCw class="h-4 w-4 mr-2" />
                Reload
              </Button>
            </div>
          </Card.Content>
        </Card.Root>
      {/if}
    </Card.Content>
  </Card.Root>
</div>
