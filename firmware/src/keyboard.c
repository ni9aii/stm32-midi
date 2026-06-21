#include "keyboard.h"

#include "hardware_config.h"

#include <libopencm3/cm3/common.h>
#include <libopencm3/stm32/gpio.h>

#define ROW_COUNT KEYBOARD_ROWS
#define COL_COUNT KEYBOARD_COLS
#define KEY_COUNT KEYBOARD_KEY_COUNT

#define ROW_SELECT_COUNT STM32_MIDI_ROW_SELECT_COUNT
#define ROW_SELECT_PIN_MASK STM32_MIDI_ROW_SELECT_PIN_MASK
#define COL_SELECT_COUNT STM32_MIDI_MUX_SELECT_COUNT
#define MUX_PA_PIN_MASK STM32_MIDI_MUX_SELECT_PA_PIN_MASK
#define MUX_PB_PIN_MASK STM32_MIDI_MUX_SELECT_PB_PIN
#define MUX_OUTPUT_PIN STM32_MIDI_MUX_OUTPUT_PIN

static const uint16_t row_select_pins[ROW_SELECT_COUNT] = {
    GPIO0,
    GPIO1,
    GPIO11,
};

struct gpio_pin {
  uint32_t port;
  uint16_t pin;
};

static const struct gpio_pin col_pins[COL_SELECT_COUNT] = {
    {GPIOA, GPIO1},  /* PA1 = S0 */
    {GPIOA, GPIO2},  /* PA2 = S1 */
    {GPIOB, GPIO12}, /* PB12 = S2 */
};

static uint32_t stable_state;
static uint32_t last_scan_ms;
static uint8_t debounce[KEY_COUNT];

static void settle_mux_inputs(void) {
  /* 74HC151/74HC238 tpd_max ≈ 23 ns @ 3.3V; 16 NOPs @ 72 MHz ≈ 222 ns (≥2× margin).
     Adjust after measuring on real hardware. */
  for (volatile uint32_t i = 0; i < 16; i++) {
    __asm__("nop");
  }
}

static void select_row(uint8_t row) {
  gpio_clear(GPIOB, ROW_SELECT_PIN_MASK);

  for (uint8_t bit = 0; bit < ROW_SELECT_COUNT; bit++) {
    if ((row & (1u << bit)) != 0) {
      gpio_set(GPIOB, row_select_pins[bit]);
    }
  }
}

static void select_col(uint8_t col) {
  gpio_clear(GPIOA, MUX_PA_PIN_MASK);
  gpio_clear(GPIOB, MUX_PB_PIN_MASK);

  for (uint8_t bit = 0; bit < COL_SELECT_COUNT; bit++) {
    if ((col & (1u << bit)) != 0) {
      gpio_set(col_pins[bit].port, col_pins[bit].pin);
    }
  }
}

static uint32_t scan_matrix_raw(void) {
  uint32_t raw = 0;

  for (uint8_t row = 0; row < ROW_COUNT; row++) {
    select_row(row);

    for (uint8_t col = 0; col < COL_COUNT; col++) {
      select_col(col);
      settle_mux_inputs();

      if (!gpio_get(GPIOB, MUX_OUTPUT_PIN)) {
        const uint8_t key = (row * COL_COUNT) + col;
        raw |= (1u << key);
      }
    }
  }

  select_row(0);
  select_col(0);

  return raw;
}

void keyboard_init(void) {
  stable_state = 0;
  last_scan_ms = 0;

  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    debounce[i] = 0;
  }
}

uint32_t keyboard_scan_changed(uint32_t now_ms) {
  if ((now_ms - last_scan_ms) < KEYBOARD_SCAN_PERIOD_MS) {
    return 0;
  }

  last_scan_ms = now_ms;

  const uint32_t raw = scan_matrix_raw();
  uint32_t next_state = stable_state;

  for (uint8_t key = 0; key < KEY_COUNT; key++) {
    const uint32_t bit = (1u << key);
    const bool pressed = (raw & bit) != 0;

    if (pressed) {
      if (debounce[key] < UINT8_MAX) {
        debounce[key]++;
      }
      if (debounce[key] >= KEYBOARD_DEBOUNCE_MS) {
        next_state |= bit;
      }
    } else {
      if (debounce[key] > 0) {
        debounce[key]--;
      }
      if (debounce[key] == 0) {
        next_state &= ~bit;
      }
    }
  }

  const uint32_t changed = next_state ^ stable_state;
  stable_state = next_state;

  return changed;
}

uint32_t keyboard_state(void) { return stable_state; }

uint8_t keyboard_note_for_key(uint8_t key) {
  if (key >= KEY_COUNT) {
    return 0;
  }

  return 36u + key;
}
