<!-- Copyright (c) 2025 David Bertet. Licensed under the MIT License. -->

<script>
  import * as Card from '$lib/components/ui/card'
  import * as Alert from '$lib/components/ui/alert'
  import { Badge } from '$lib/components/ui/badge'
  import { AlertTriangle, CheckCircle, Info, Activity } from 'lucide-svelte'
  import { cn } from '$lib/utils.js'

  let { task, class: className } = $props()

  // Convert stack HWM to a meaningful assessment
  let stackLevel = $derived.by(() => {
    const hwmWords = task.stack_hwm_words
    // Low HWM means less free space (higher usage)
    if (hwmWords < 100) return 'critical' // Less than 400 bytes free
    if (hwmWords < 200) return 'warning' // Less than 800 bytes free
    return 'good'
  })

  let progressColor = $derived.by(() => {
    switch (stackLevel) {
      case 'critical':
        return 'bg-red-500'
      case 'warning':
        return 'bg-yellow-500'
      default:
        return 'bg-green-500'
    }
  })

  let badgeVariant = $derived.by(() => {
    switch (stackLevel) {
      case 'critical':
        return 'destructive'
      case 'warning':
        return 'default'
      default:
        return 'muted'
    }
  })

  let cardClasses = $derived.by(() => {
    const base = 'transition-all duration-200 hover:shadow-md'
    switch (stackLevel) {
      case 'critical':
        return `${base} border-red-400`
      case 'warning':
        return `${base} border-yellow-400`
      default:
        return `${base}`
    }
  })

  let statusText = $derived.by(() => {
    switch (stackLevel) {
      case 'critical':
        return 'Low Stack'
      case 'warning':
        return 'Monitor'
      default:
        return 'Healthy'
    }
  })
</script>

<Card.Root class={cn(cardClasses, 'gap-2', className)}>
  <Card.Header>
    <div class="flex items-center justify-between">
      <div class="flex items-center gap-2">
        <!-- Status Icon -->
        {#if stackLevel === 'critical'}
          <AlertTriangle class="h-4 w-4 text-red-500" />
        {:else if stackLevel === 'warning'}
          <AlertTriangle class="h-4 w-4 text-yellow-500" />
        {:else}
          <CheckCircle class="h-4 w-4 text-green-500" />
        {/if}

        <Card.Title class="text-lg font-semibold">{task.name}</Card.Title>

        <!-- Task State -->
        <div class="flex items-center gap-1 ml-2">
          {#if task.state.toLowerCase() === 'running'}
            <Activity class="h-3 w-3 text-green-500" />
          {:else if task.state.toLowerCase() === 'blocked'}
            <div class="h-3 w-3 rounded-full bg-blue-500"></div>
          {:else if task.state.toLowerCase() === 'suspended'}
            <div class="h-3 w-3 rounded-full bg-gray-500"></div>
          {:else if task.state.toLowerCase() === 'ready'}
            <div class="h-3 w-3 rounded-full bg-yellow-500"></div>
          {:else}
            <div class="h-3 w-3 rounded-full bg-gray-300"></div>
          {/if}
          <span class="text-xs text-muted-foreground capitalize">{task.state}</span>
        </div>
      </div>

      <div class="flex items-center gap-2">
        <Badge variant={badgeVariant} class="text-xs">
          {statusText}
        </Badge>
        <Badge variant="outline" class="text-xs">
          P{task.priority}
        </Badge>
      </div>
    </div>
  </Card.Header>

  <Card.Content class="space-y-4">
    <!-- Stack Details -->
    <div class="grid grid-cols-2 gap-4 text-sm">
      <div class="space-y-1">
        <div class="text-muted-foreground">Stack HWM</div>
        <div class="font-medium">
          {task.stack_hwm_bytes}
          <div class="inline-block h-2 w-2 rounded-full {progressColor}"></div>
        </div>
        <div class="text-xs text-muted-foreground/80">
          {task.stack_hwm_words} words
        </div>
      </div>
      <div class="space-y-1">
        <div class="text-muted-foreground">Task #</div>
        <div class="font-medium">{task.task_number}</div>
        {#if task.core_id !== undefined}
          <div class="text-xs text-muted-foreground/80">Core {task.core_id}</div>
        {/if}
      </div>
    </div>

    <!-- Information and Warning Alerts -->
    {#if stackLevel === 'critical'}
      <Alert.Root class="border-red-200 dark:border-red-400/40 bg-red-50 dark:bg-red-900/10">
        <AlertTriangle class="h-4 w-4 !text-red-600" />
        <Alert.Description class="text-red-800 dark:text-red-700">
          Very low stack free space detected ({task.stack_hwm_bytes}). Consider increasing stack
          size.
        </Alert.Description>
      </Alert.Root>
    {:else if stackLevel === 'warning'}
      <Alert.Root
        class="border-yellow-200 dark:border-yellow-400/40 bg-yellow-50 dark:bg-yellow-900/10"
      >
        <AlertTriangle class="h-4 w-4 !text-yellow-600" />
        <Alert.Description class="text-yellow-800 dark:text-yellow-600">
          Stack free space is getting low ({task.stack_hwm_bytes}). Monitor task behavior.
        </Alert.Description>
      </Alert.Root>
    {/if}
  </Card.Content>
</Card.Root>
