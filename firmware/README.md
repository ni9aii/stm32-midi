# Прошивка stm32-midi

Минимальная USB-MIDI прошивка для STM32F103C8T6 из схемы проекта. Сборка управляется через CMake.

## Host-тесты

Host-тесты проверяют чистый USB-MIDI packet builder и не требуют ARM GCC или железа. Текущий test покрывает Note On/Off и clamp MIDI data bytes в 0–127.

```sh
cd firmware
cmake -S . -B build-test
cmake --build build-test --target midi_packets_test
ctest --test-dir build-test --output-on-failure
clang -fsyntax-only -Wall -Wextra \
  -Iinclude \
  -I../libopencm3/include \
  -DSTM32F1 -DSTM32F103C8 \
  src/main.c src/keyboard.c src/midi_packet.c src/usb_midi.c
```

## Firmware build

Для сборки firmware нужен ARM GCC toolchain, в частности `arm-none-eabi-gcc`.

```sh
cd firmware
cmake -S . -B build-stm32 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/stm32f103c8-toolchain.cmake
cmake --build build-stm32 --target stm32-midi
```

Если libopencm3 еще не собран, CMake попытается собрать `libopencm3_stm32f1.a` через внешний target `libopencm3_stm32f1`.

Целевые файлы после firmware build:

- `build-stm32/stm32-midi.elf`
- `build-stm32/stm32-midi.bin`
- `build-stm32/stm32-midi.hex`

## Прошивка

При наличии `st-flash`:

```sh
cmake --build build-stm32 --target flash
```

Или вручную:

```sh
st-flash write build-stm32/stm32-midi.bin 0x08000000
```

## Ограничения текущей проверки

В этой среде не установлен `arm-none-eabi-gcc`, поэтому полноценная сборка `.elf` через CMake пока не выполняется. Host-тесты через CTest и `clang -fsyntax-only` используются как предварительная проверка без железа.

## Что делает прошивка

1. Настраивает STM32F103C8T6:
   - HSE 8 MHz;
   - PLL до 72 MHz;
   - SysTick 1 ms;
   - USB FS.
2. Сканирует матрицу 5×5:
   - строки через 74HC238: `PB0`, `PB1`, `PB11`;
   - столбцы через 74HC151: `PA1`, `PA2`, `PB12`;
   - вход чтения: `PB13`.
3. Дребезг обрабатывается программно: `keyboard_scan_changed(now_ms)` принимает SysTick millisecond timestamp и требует 3 ms стабильного состояния.
4. При изменении состояния клавиши отправляет USB-MIDI:
   - `Note On`, channel 1, velocity 100;
   - `Note Off`, channel 1, velocity 0.
5. Ноты идут подряд от MIDI note 36 по индексу клавиши `row * 5 + col`.

## USB-MIDI

Устройство описывается как USB Audio/MIDI class device:

- VID/PID: `1209:0001`
- endpoint: `0x81`, Bulk IN, 64 байта
- один embedded MIDI IN jack и один embedded MIDI OUT jack

Текущая версия только передает события клавиатуры в USB-хост.
