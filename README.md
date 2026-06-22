# stm32-midi

Минимальная прошивка USB-MIDI клавиатуры для схемы STM32F103C8T6 из репозитория.

## Что внутри

|\`- `docs/Schematic.pdf` — электрическая принципиальная схема.
|- `docs/BOM.pdf` — перечень элементов/BOM.
|- `firmware/` — прошивка на **libopencm3** для STM32F103C8T6.
|- `docs/hardware.md` — расшифровка схемы, BOM и назначений GPIO.

## Аппаратная часть

Плата построена вокруг STM32F103C8T6:

- USB-C подключен к встроенному USB FS: `PA11`/`PA12` через 22 Ом.
- Тактирование: внешний кварц 8 MHz, PLL до 72 MHz.
- SWD: `PA14/SWCLK`, `PA13/SWDIO`, `NRST`.
- Матрица клавиш 5×5 с диодами:
  - строки выбираются через 74HC238 (`PB0`, `PB1`, `PB11`);
  - столбцы читаются через 74HC151 (`PA1`, `PA2`, `PB12`);
  - сигнал выбранного столбца читается с `PB13`;
  - активный уровень нажатия — логический `0`.
- LED `VD26` — индикатор питания, подключен напрямую через R6.

## Быстрый старт

```sh
cd firmware
git submodule update --init --recursive
cmake -S . -B build-test
cmake --build build-test --target midi_packets_test
ctest --test-dir build-test --output-on-failure
```

Для firmware build нужен `arm-none-eabi-gcc`:

```sh
cmake -S . -B build-stm32 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/stm32f103c8-toolchain.cmake
cmake --build build-stm32 --target stm32-midi
```

Прошивка отправляет USB-MIDI `Note On`/`Note Off` для 25 клавиш. Канал MIDI — 1, velocity при нажатии — 100, при отпускании — 0. Ноты идут подряд от MIDI note 36.

## Проверка без железа

Текущий повторяемый gate в этой среде:

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

Host-тест проверяет USB-MIDI packet builder и clamp MIDI data bytes в 0–127. `clang -fsyntax-only` проверяет firmware sources against libopencm3 headers without linking ARM firmware.

## Прошивка на плату

Если установлен `st-flash`:

```sh
cmake --build build-stm32 --target flash
```

Или вручную:

```sh
st-flash write build-stm32/stm32-midi.bin 0x08000000
```

## Ограничения текущей версии

Это минимальная рабочая версия:

- нет конфигурации нот/velocity;
- нет обработки USB-MIDI входящих сообщений;
- нет энергосбережения;
- нет аппаратной отладки на реальном устройстве в этой среде.

См. `docs/hardware.md` для подробной карты пинов и BOM.
