<!-- Copyright (c) 2025 David Bertet. Licensed under the MIT License. -->

<script>
  import { AlertTriangle } from 'lucide-svelte'

  let { hasErrors = false } = $props()
</script>

{#if hasErrors}
  <div
    class="p-4 w-full bg-red-50 dark:bg-red-950 border border-red-200 dark:border-red-800 rounded-lg"
  >
    <div class="flex items-center gap-2 mb-2">
      <AlertTriangle class="w-4 h-4 text-red-500" />
      <p class="text-sm font-medium text-red-600 dark:text-red-400">Pin Configuration Errors</p>
    </div>
    <p class="text-sm text-red-600 dark:text-red-400">
      Please fix the pin compatibility issues above before saving.
    </p>
  </div>
{/if}
