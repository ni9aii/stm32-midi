# Протокол тестирования на железе — stm32-midi v0.1.0-rc1

Этот документ предназначен для человека, который прошивает устройство и подтверждает готовность к релизу.

## Требования

- STM32F103C8T6 плата согласно схеме (`docs/Schematic.pdf`)
- ST-Link/V2 или совместимый SWD-программатор
- `arm-none-eabi-gcc` (для сборки) или готовый `.bin`/`.elf` из CI-артефактов
- `st-flash` или `openocd`
- USB-кабель (USB-C для платы, USB-A для хоста)
- Любой компьютер с Linux, macOS или Windows

---

## Шаг 1 — Сборка прошивки

```sh
cd firmware
git submodule update --init --recursive
cmake -S . -B build-stm32 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/stm32f103c8-toolchain.cmake \
  -DSTM32_MIDI_BUILD_HOST_TESTS=OFF
cmake --build build-stm32
```

Убедиться что сборка завершилась без ошибок и файл существует:

```sh
ls -lh build-stm32/stm32-midi.elf build-stm32/stm32-midi.bin
arm-none-eabi-size build-stm32/stm32-midi.elf
```

Размер `.text + .data` должен быть меньше 60 KB.

---

## Шаг 2 — Прошивка

Подключить ST-Link к SWD-разъёму (`SWCLK=PA14`, `SWDIO=PA13`, `GND`, `3.3V`).

**Через CMake:**
```sh
cmake --build build-stm32 --target flash
```

**Или через openocd (рекомендуется):**
```sh
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build-stm32/stm32-midi.elf verify reset exit"
```

**Или через st-flash:**
```sh
st-flash write build-stm32/stm32-midi.bin 0x08000000
```

Ожидаемый результат: прошивка завершена без ошибок, устройство сброшено.

---

## Шаг 3 — Проверка USB-перечисления

Подключить плату к компьютеру через USB-C. Подождать 2–3 секунды.

**Linux:**
```sh
lsusb | grep 1209
# ожидается: Bus XXX Device XXX: ID 1209:0001
aconnect -l
# ожидается: клиент "STM32 MIDI Keyboard" в списке MIDI-портов
```

**macOS:**
- Открыть `Audio MIDI Setup` → `Window` → `Show MIDI Studio`
- Устройство `STM32 MIDI Keyboard` должно появиться в списке

**Windows:**
- `Device Manager` → `Sound, video and game controllers`
- Должно быть `STM32 MIDI Keyboard` без знака ошибки

---

## Шаг 4 — Проверка MIDI-событий

Открыть любой MIDI-монитор (например, `kmidimon` на Linux, `MIDI Monitor` на macOS, `MIDI-OX` на Windows).

Нажать и отпустить несколько клавиш матрицы:

| Ожидаемое событие | Значение |
|---|---|
| Note On при нажатии | Channel 1, Note 36–60, Velocity 100 |
| Note Off при отпускании | Channel 1, Note 36–60, Velocity 0 |
| Нет дребезга | повторных событий нет при плавном нажатии |

Проверить все 25 клавиш (5 строк × 5 столбцов).

---

## Шаг 5 — Проверка переподключения USB

1. Отключить USB-кабель → подождать 2 с → подключить снова.
2. Устройство должно переперечислиться без зависания.
3. MIDI-события должны работать как прежде.

---

## Шаг 6 — Результат

Заполнить и отправить автору:

```
Дата тестирования:
ОС хоста:
Программатор:

[ ] Шаг 1 — сборка: OK / FAIL
[ ] Шаг 2 — прошивка: OK / FAIL
[ ] Шаг 3 — USB-перечисление: OK / FAIL
[ ] Шаг 4 — MIDI-события (все 25 клавиш): OK / FAIL (какие?)
[ ] Шаг 5 — переподключение: OK / FAIL

Комментарии:
```

При FAIL — приложить вывод команд и описание поведения.
