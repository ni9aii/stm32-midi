#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "hardware_config.h"
#include "keyboard.h"
#include "usb_midi.h"

#define SYSTICK_RELOAD (STM32_MIDI_SYSTEM_CORE_CLOCK_HZ / 1000u)
#define MIDI_VELOCITY 100u

static volatile uint32_t system_millis;

void sys_tick_handler(void) { system_millis++; }

static void clock_setup(void) {
  rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
}

static void gpio_setup(void) {
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_USB);

  gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                STM32_MIDI_USB_D_MINUS_PIN | STM32_MIDI_USB_D_PLUS_PIN);

  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                STM32_MIDI_ROW_SELECT_PIN_MASK | STM32_MIDI_MUX_SELECT_PB_PIN);

  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                STM32_MIDI_MUX_SELECT_PA_PIN_MASK);

  gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                STM32_MIDI_MUX_OUTPUT_PIN);
}

static void systick_setup(void) {
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_set_reload(SYSTICK_RELOAD - 1u);
  systick_interrupt_enable();
  systick_counter_enable();
}

int main(void) {
  uint32_t last_scan_ms = 0;

  clock_setup();
  systick_setup();
  gpio_setup();
  keyboard_init();
  if (!usb_midi_init()) {
    while (1) {
      /* USB init failed — halt. */
    }
  }

  while (1) {
    const uint32_t now = system_millis;

    if ((now - last_scan_ms) >= 1u) {
      last_scan_ms = now;

      const uint32_t changed = keyboard_scan_changed(now);
      const uint32_t state = keyboard_state();

      for (uint8_t key = 0; key < KEYBOARD_KEY_COUNT; key++) {
        const uint32_t bit = (1u << key);

        if ((changed & bit) == 0) {
          continue;
        }

        const uint8_t note = keyboard_note_for_key(key);

        if ((state & bit) != 0) {
          usb_midi_note_on(note, MIDI_VELOCITY);
        } else {
          usb_midi_note_off(note, 0u);
        }
      }
    }

    usb_midi_poll();
  }
}
