# stm32-midi

[![CI](https://github.com/ni9aii/stm32-midi/actions/workflows/ci.yml/badge.svg)](https://github.com/ni9aii/stm32-midi/actions/workflows/ci.yml)

Прошивка USB-MIDI клавиатуры для STM32F103C8T6. Матрица 5×5, USB Audio/MIDI class, отправляет `Note On`/`Note Off` для 25 клавиш.

## Что внутри

```
firmware/          прошивка (libopencm3, CMake)
docs/hardware.md   схема, BOM, карта GPIO
docs/Schematic.pdf принципиальная схема
docs/BOM.pdf       перечень элементов
```

## Аппаратная часть

Плата построена вокруг STM32F103C8T6:

- USB-C: встроенный USB FS, `PA11`/`PA12` через 22 Ом
- Тактирование: внешний кварц 8 MHz, PLL до 72 MHz
- SWD: `PA14/SWCLK`, `PA13/SWDIO`, `NRST`
- Матрица клавиш 5×5 с диодами:
  - строки — 74HC238 (`PB0`, `PB1`, `PB11`)
  - столбцы — 74HC151 (`PA1`, `PA2`, `PB12`)
  - выход мультиплексора — `PB13`, активный уровень нажатия `0`
- LED `VD26` — индикатор питания

## Быстрый старт

### Host-тесты (без железа)

```sh
cd firmware
git submodule update --init --recursive
cmake -S . -B build-test
cmake --build build-test
ctest --test-dir build-test --output-on-failure
```

Два теста: MIDI packet builder и логика дебаунса клавиатуры.

### Сборка прошивки (требует `arm-none-eabi-gcc`)

```sh
cmake -S . -B build-stm32 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/stm32f103c8-toolchain.cmake \
  -DSTM32_MIDI_BUILD_HOST_TESTS=OFF
cmake --build build-stm32
```

## Прошивка на плату

Через CMake-цель:

```sh
cmake --build build-stm32 --target flash
```

Или вручную через `st-flash`:

```sh
st-flash write build-stm32/stm32-midi.bin 0x08000000
```

Через OpenOCD (рекомендуется, подробнее в `TESTING_INSTRUCTIONS.md`):

```sh
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build-stm32/stm32-midi.elf verify reset exit"
```

## Что делает прошивка

- Сканирует матрицу 5×5 каждые 1 мс
- Дебаунс 3 мс программный
- USB серийный номер генерируется из уникального 96-bit ID устройства
- USB MIDI `Note On` (velocity 100) / `Note Off` (velocity 0) для нот 36–60
- TX-only: входящие MIDI-сообщения не обрабатываются

## Известные ограничения до финального релиза

- USB PID `0x0001` — тестовый. Ждёт регистрации на [pid.codes](https://pid.codes)
- Прошивка не тестировалась на реальном железе

См. `docs/hardware-test-checklist.md` для протокола проверки на устройстве.
