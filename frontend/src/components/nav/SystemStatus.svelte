<!-- Copyright (c) 2025 David Bertet. Licensed under the MIT License. -->

<script>
  import { cn } from '$lib/utils'
  import { slide } from 'svelte/transition'

  import { AlertTriangle, Clock } from 'lucide-svelte'

  import { settingsState } from 'src/lib/settings.svelte.js'

  const systemState = $derived.by(() => {
    if (settingsState.powerWheel?.setupMode) {
      return {
        message: 'Finish setup in Wiring tab',
        style: 'bg-red-500 text-white border-red-600',
        icon: AlertTriangle,
        title: 'Car is disabled for safety',
      }
    }

    return null // operational state
  })
</script>

{#if systemState}
  <div
    class={cn(
      'flex items-center justify-between px-6 pt-2 pb-6 transition-all duration-300 fixed w-full -z-5000',
      systemState.style,
    )}
  >
    <div class="flex items-center gap-3">
      <systemState.icon class="size-5 flex-shrink-0" />
      <div class="flex flex-col">
        <span class="font-semibold text-sm">
          {systemState.title}
        </span>
        <span class="text-sm opacity-90">
          {systemState.message}
        </span>
      </div>
    </div>
  </div>

  <div class="relative -z-50" transition:slide={{ duration: 300, axis: 'y' }}>
    <div class="h-18 w-full"></div>

    <div
      class="hidden md:inline absolute top-0 left-[-16px] shadow-[6px_0_12px_rgba(0,0,0,0.15)] dark:shadow-[6px_0_12px_rgba(0,0,0,0.3)] w-4 h-full"
    ></div>
    <div
      class="absolute bottom-0 shadow-[0_-6px_12px_rgba(0,0,0,0.15)] dark:shadow-[0_-6px_12px_rgba(0,0,0,0.3)] rounded-t-xl h-4 w-full bg-background"
    ></div>
  </div>
{/if}
