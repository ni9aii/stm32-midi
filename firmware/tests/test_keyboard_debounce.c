#include "keyboard.h"

#include <assert.h>
#include <stdint.h>

/* Stub scanner — управляется тестом через глобальную переменную. */
static uint32_t stub_raw;
static uint32_t stub_scanner(void) { return stub_raw; }

/* Прогнать N итераций сканирования с шагом KEYBOARD_SCAN_PERIOD_MS. */
static uint32_t run_scans(uint32_t *now_ms, uint32_t count) {
  uint32_t changed = 0;
  for (uint32_t i = 0; i < count; i++) {
    *now_ms += KEYBOARD_SCAN_PERIOD_MS;
    changed |= keyboard_scan_changed(*now_ms);
  }
  return changed;
}

/* Клавиша 0 удерживается KEYBOARD_DEBOUNCE_MS циклов → должна стабилизироваться. */
static void test_key_press_stabilizes(void) {
  uint32_t now = 0;
  keyboard_init_with_scanner(stub_scanner);

  stub_raw = 0x1; /* клавиша 0 нажата */
  uint32_t changed = run_scans(&now, KEYBOARD_DEBOUNCE_MS);

  assert(changed & 0x1);               /* событие изменения */
  assert(keyboard_state() & 0x1);      /* состояние — нажата */
}

/* Дребезг (нажата 1 цикл, затем отпущена) не должен генерировать событие. */
static void test_bounce_below_threshold_ignored(void) {
  uint32_t now = 0;
  keyboard_init_with_scanner(stub_scanner);

  stub_raw = 0x1;
  now += KEYBOARD_SCAN_PERIOD_MS;
  keyboard_scan_changed(now); /* 1 цикл нажатия */

  stub_raw = 0x0;
  now += KEYBOARD_SCAN_PERIOD_MS;
  uint32_t changed = keyboard_scan_changed(now); /* сразу отпущена */

  assert((changed & 0x1) == 0);        /* нет события */
  assert((keyboard_state() & 0x1) == 0);
}

/* После стабилизации нажатия отпускание должно генерировать событие. */
static void test_key_release_after_press(void) {
  uint32_t now = 0;
  keyboard_init_with_scanner(stub_scanner);

  /* Нажатие */
  stub_raw = 0x1;
  run_scans(&now, KEYBOARD_DEBOUNCE_MS);
  assert(keyboard_state() & 0x1);

  /* Отпускание */
  stub_raw = 0x0;
  uint32_t changed = run_scans(&now, KEYBOARD_DEBOUNCE_MS);

  assert(changed & 0x1);               /* событие изменения */
  assert((keyboard_state() & 0x1) == 0);
}

/* Несколько клавиш одновременно. */
static void test_multiple_keys(void) {
  uint32_t now = 0;
  keyboard_init_with_scanner(stub_scanner);

  stub_raw = 0x7; /* клавиши 0, 1, 2 */
  uint32_t changed = run_scans(&now, KEYBOARD_DEBOUNCE_MS);

  assert((changed & 0x7) == 0x7);
  assert((keyboard_state() & 0x7) == 0x7);
}

/* Сканирование раньше KEYBOARD_SCAN_PERIOD_MS не должно возвращать изменений. */
static void test_scan_period_throttling(void) {
  uint32_t now = 0;
  keyboard_init_with_scanner(stub_scanner);

  stub_raw = 0x1;
  /* Вызов без продвижения времени — не должен сканировать. */
  uint32_t changed = keyboard_scan_changed(now);
  assert(changed == 0);
  assert((keyboard_state() & 0x1) == 0);
}

int main(void) {
  test_key_press_stabilizes();
  test_bounce_below_threshold_ignored();
  test_key_release_after_press();
  test_multiple_keys();
  test_scan_period_throttling();

  return 0;
}
