# Changelog

## v0.1.0-rc1 (2026-06-22)

Первый release candidate. Ожидает: регистрация USB PID на pid.codes, тест на реальном железе.

### Добавлено

- Прошивка USB-MIDI для STM32F103C8T6 на libopencm3
  - сканирование матрицы 5×5 через 74HC238/74HC151
  - программный дебаунс 3 мс
  - USB Audio/MIDI class descriptor (TX-only, bulk IN endpoint)
  - `Note On`/`Note Off` для нот 36–60, velocity 100/0
- USB серийный номер из 96-bit уникального ID устройства (регистры `0x1FFFF7E8`)
- Host-тесты: MIDI packet builder, логика дебаунса клавиатуры (без железа)
- CI: host-тесты на ubuntu/macOS, сборка firmware через `arm-none-eabi-gcc`, проверка размера прошивки (лимит 60 KB)
- Документация: схема, BOM, карта GPIO, инструкции установки и прошивки, протокол тестирования на железе

### Исправлено

- `select_col()` итерировала 5 бит над 3-элементным массивом адресных пинов
- `st-flash` в CMake-цели и документации передавал `.elf` вместо `.bin`
- Логика инициализации USB: при ошибке — явный halt вместо polling с `usb_dev == NULL`
- `system_millis` читался трижды за итерацию главного цикла (volatile TOCTOU)
- Отсутствовал `usbd_register_reset_callback` — `configured` не сбрасывался при USB bus reset

### Безопасность / надёжность

- `_Static_assert` на `KEYBOARD_KEY_COUNT <= 32` (bitmask width)
- `_Static_assert` на `USB_MIDI_QUEUE_LEN` как степень двойки
- Range guard в `select_row()`/`select_col()`
- `keyboard_note_for_key()` возвращает `0xFF` sentinel для невалидного ключа
- Compile flags для STM32: `-mthumb -mcpu=cortex-m3 -msoft-float -ffreestanding`
- `-Werror` включён для всех целей

### Известные ограничения

- USB `VID=0x1209 / PID=0x0001` — тестовый PID pid.codes, ждёт регистрации
- Нет обработки входящих USB MIDI сообщений
- Нет конфигурации нот и velocity через USB
