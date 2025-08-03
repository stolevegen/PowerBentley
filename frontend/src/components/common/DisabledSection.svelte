<!-- Copyright (c) 2025 David Bertet. Licensed under the MIT License. -->

<script>
  import { Lock } from 'lucide-svelte'
  import { cn } from '$lib/utils.js'

  let { isEnabled, title, message, class: className, children } = $props()
</script>

<div class="relative transition-all duration-200">
  {#if !isEnabled}
    <div>
      <div
        class="absolute inset-0 z-10 bg-black/10 backdrop-blur-[1px] rounded-lg flex items-center justify-center"
      >
        <div
          class="bg-white p-4 rounded-lg shadow-lg border-2 border-orange-200 text-center max-w-sm"
        >
          <Lock class="h-8 w-8 text-orange-600 mx-auto mb-2" />
          <p class="font-medium text-orange-900 mb-1">{title}</p>
          <p class="text-sm text-orange-700">{message}</p>
        </div>
      </div>
    </div>
  {/if}

  <div class={cn(className, !isEnabled && 'opacity-60 pointer-events-none select-none')}>
    {@render children?.()}
  </div>
</div>
