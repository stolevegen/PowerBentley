// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

export const adc1Pins = [
  { value: '36', label: 'GPIO 36' }, // ADC1_CH0
  { value: '37', label: 'GPIO 37' }, // ADC1_CH1
  { value: '38', label: 'GPIO 38' }, // ADC1_CH2
  { value: '39', label: 'GPIO 39' }, // ADC1_CH3
  { value: '32', label: 'GPIO 32' }, // ADC1_CH4
  { value: '33', label: 'GPIO 33' }, // ADC1_CH5
  { value: '34', label: 'GPIO 34' }, // ADC1_CH6
  { value: '35', label: 'GPIO 35' }, // ADC1_CH7
]

export const pwmPins = [
  { value: '2', label: 'GPIO 2' },
  { value: '4', label: 'GPIO 4' },
  { value: '5', label: 'GPIO 5' },
  { value: '12', label: 'GPIO 12' },
  { value: '13', label: 'GPIO 13' },
  { value: '14', label: 'GPIO 14' },
  { value: '15', label: 'GPIO 15' },
  { value: '16', label: 'GPIO 16' },
  { value: '17', label: 'GPIO 17' },
  { value: '18', label: 'GPIO 18' },
  { value: '19', label: 'GPIO 19' },
  { value: '21', label: 'GPIO 21' },
  { value: '22', label: 'GPIO 22' },
  { value: '23', label: 'GPIO 23' },
  { value: '25', label: 'GPIO 25' },
  { value: '26', label: 'GPIO 26' },
  { value: '27', label: 'GPIO 27' },
  { value: '32', label: 'GPIO 32' },
  { value: '33', label: 'GPIO 33' },
]

export const digitalInputPins = [
  { value: '2', label: 'GPIO 2' },
  { value: '4', label: 'GPIO 4' },
  { value: '5', label: 'GPIO 5' },
  { value: '12', label: 'GPIO 12' },
  { value: '13', label: 'GPIO 13' },
  { value: '14', label: 'GPIO 14' },
  { value: '15', label: 'GPIO 15' },
  { value: '16', label: 'GPIO 16' },
  { value: '17', label: 'GPIO 17' },
  { value: '18', label: 'GPIO 18' },
  { value: '19', label: 'GPIO 19' },
  { value: '21', label: 'GPIO 21' },
  { value: '22', label: 'GPIO 22' },
  { value: '23', label: 'GPIO 23' },
  { value: '25', label: 'GPIO 25' },
  { value: '26', label: 'GPIO 26' },
  { value: '27', label: 'GPIO 27' },
  { value: '32', label: 'GPIO 32' },
  { value: '33', label: 'GPIO 33' },
]

// Color scheme mappings
export const colorSchemes = {
  blue: {
    border: 'border-blue-400',
    bg: 'bg-blue-50 dark:bg-blue-950',
    borderColor: 'border-blue-300',
    text: 'text-blue-700 dark:text-blue-300',
    descText: 'text-blue-600 dark:text-blue-400',
    indicator: 'bg-blue-500',
  },
  green: {
    border: 'border-green-400',
    bg: 'bg-green-50 dark:bg-green-950',
    borderColor: 'border-green-300',
    text: 'text-green-700 dark:text-green-300',
    descText: 'text-green-600 dark:text-green-400',
    indicator: 'bg-green-500',
  },
  orange: {
    border: 'border-orange-400',
    bg: 'bg-orange-50 dark:bg-orange-950',
    borderColor: 'border-orange-300',
    text: 'text-orange-700 dark:text-orange-300',
    descText: 'text-orange-600 dark:text-orange-400',
    indicator: 'bg-orange-500',
  },
  yellow: {
    border: 'border-yellow-400',
    bg: 'bg-yellow-50 dark:bg-yellow-950',
    borderColor: 'border-yellow-300',
    text: 'text-yellow-700 dark:text-yellow-300',
    descText: 'text-yellow-600 dark:text-yellow-400',
    indicator: 'bg-yellow-500',
  },
  purple: {
    border: 'border-purple-400',
    bg: 'bg-purple-50 dark:bg-purple-950',
    borderColor: 'border-purple-300',
    text: 'text-purple-700 dark:text-purple-300',
    descText: 'text-purple-600 dark:text-purple-400',
    indicator: 'bg-purple-500',
  },
  red: {
    border: 'border-red-400',
    bg: 'bg-red-50 dark:bg-red-950',
    borderColor: 'border-red-300',
    text: 'text-red-700 dark:text-red-300',
    descText: 'text-red-600 dark:text-red-400',
    indicator: 'bg-red-500',
  },
}
