#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>

#define KEYBOARD_ROWS 5u
#define KEYBOARD_COLS 5u
#define KEYBOARD_KEY_COUNT (KEYBOARD_ROWS * KEYBOARD_COLS)
_Static_assert(KEYBOARD_KEY_COUNT <= 32, "KEYBOARD_KEY_COUNT exceeds uint32_t bitmask width");
#define KEYBOARD_SCAN_PERIOD_MS 1u
#define KEYBOARD_DEBOUNCE_MS 3u

typedef uint32_t (*keyboard_scan_fn)(void);

void keyboard_init(void);
void keyboard_init_with_scanner(keyboard_scan_fn fn);
uint32_t keyboard_scan_changed(uint32_t now_ms);
uint32_t keyboard_state(void);
uint8_t keyboard_note_for_key(uint8_t key);

#endif
