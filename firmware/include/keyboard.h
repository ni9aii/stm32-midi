#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>

#define KEYBOARD_ROWS 5u
#define KEYBOARD_COLS 5u
#define KEYBOARD_KEY_COUNT (KEYBOARD_ROWS * KEYBOARD_COLS)
#define KEYBOARD_DEBOUNCE_STABLE 3u

void keyboard_init(void);
uint32_t keyboard_scan_changed(void);
uint32_t keyboard_state(void);
uint8_t keyboard_note_for_key(uint8_t key);

#endif
