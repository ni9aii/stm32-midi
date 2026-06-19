# Инструкции по тестированию STM32 MIDI

## Генеральный план

Этот документ содержит все необходимые шаги для тестирования сборки, прошивки и работы STM32 MIDI проекта.

## Содержание

1. [Требования](#требования)
2. [Настройка среды](#настройка-среды)
3. [Проверка хост-тестов](#проверка-хост-тестов)
4. [Сборка прошивки](#сборка-прошивки)
5. [Проверка прошивки](#проверка-прошивки)
6. [Прошивка на устройство](#прошивка-на-устройство)
7. [Проверка работы](#проверка-работы)

## Требования

- Клонированный репозиторий stm32-midi с инициализированным субмодулем libopencm3
- CMake 3.20+ с поддержкой cross-compilation
- ARM GCC компилятор (arm-none-eabi-gcc)
- st-flash (для прошивки через ST-Link/V2)

## Настройка среды

```bash
cd /path/to/stm32-midi
# Инициализировать субмодули
git submodule update --init --recursive
# Экспорт переменных среды для ARM компилятора (если необходимо)
export PATH="/opt/arm-none-eabi/bin:$PATH"
```

## Проверка хост-тестов

Для предварительной проверки платы, не требующей процессора ARM, выполните следующее:

```bash
cd firmware
# Создать и собрать тесты в unix-среде
mkdir -p build-test
cmake -S . -B build-test
cmake --build build-test --target midi_packets_test
# Запустить тесты с помощью CTest
ctest --test-dir build-test --output-on-failure
```

**Что это проверяет:**
- USB-MIDI packet builder работает правильно
- Обработка захода данных (clamp) в формате 0-127
- Обработка Note On/Off событий

## Сборка прошивки

Для сборки ARM-версии прошивки выполните следующее:

```bash
cd firmware
# Создать сборку STM32
cmake -S . -B build-stm32 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/stm32f103c8-toolchain.cmake
# Собрать целевое binary
****cmake --build build-stm32 --target stm32-midi
```

**Целевые файлы после сборки:**
- `build-stm32/stm32-midi.elf` - ELF-подобный бинарный файл для загрузчика
- `build-stm32/stm32-midi.bin` - бинарный образ
- `build-stm32/stm32-midi.hex` - HEX образ для вариаций загрузчика

## Проверка прошивки

Для синтаксической и семантической проверки перед записыванием (требует инструментов C компилятора, но не загрузчика):

```bash
cd firmware
# Проверка синтаксиса USB и общего SLA
clang -fsyntax-only -Wall -Wextra \
  -Iinclude \
  -I../libopencm3/include \
  -DSTM32F1 -DSTM32F103C8 \
  src/main.c src/keyboard.c src/midi_packet.c src/usb_midi.c
```

## Прошивка на устройство

### С использованием целевого правила CMake

Если `st-flash` (Драйвер ST-Link/V2) присутствует в PATH:

```bash
cd firmware
cmake --build build-stm32 --target flash
```

### Ручная запись

Если вы предпочитаете контролировать процесс:

```bash
cd firmware
st-flash write build-stm32/stm32-midi.elf 0x08000000
```

**Примечание о подключении:** Проект ожидает аппаратный управляющий байт
- 32-битный тактовый выход 72 МГц (HSE 8 МГц с PLL)
- USB-FS, сгенерированный из внутреннего RC (не рекомендуется)
- 64-байтовый буфер

## Проверка работы

### Хост для наблюдения MIDI

Используйте любой универсальный MIDI-HOST (например, тестер MIDI, функция записи в реализациях DAW, MIDI через USB-UART-бридж)

### Мониторинг платы с использованием последовательного терминала

Плата не имеет встроенного UART-серьезного порта, для наблюдения за уровнем MIDI используются USB-хосты. Для встроенной отладки планируется использование встроенного SWD с помощью мостика JTAG-to-UART.

### Маркировка клавиш

В соответствии со схемой и hardware.md:
- Индекс ноты = `row * 5 + col` (0-based)
- Стартовая MIDI нота = 36 (C2)
- Номера клавиш: строка 0 = SB1-SB5, строка 1 = SB6-SB10, и т.д.

## Вердификация

### Вариант 1: Набор тестов на PC

Все тесты должны пройти успешно:

```bash
cd firmware
# Хост-тесты
mkdir -p build-test
cmake -S . -B build-test
cmake --build build-test --target midi_packets_test
ctest --test-dir build-test --output-on-failure
```

### Вариант 2: Сборка ARM и статическая проверка

Если присутствует tool-chain, выполните:

```bash
cd firmware
mkdir -p build-stm32
cmake -S . -B build-stm32 -DCMAKE_TOOLCHAIN_FILE=cmake/stm32f103c8-toolchain.cmake
cmake --build build-stm32 --target stm32-midi
# Предварительное подключение (нефизическая проверка)
clang -fsyntax-only -Wall -Wextra \
  -Iinclude -I../libopencm3/include \
  -DSTM32F1 -DSTM32F103C8 \
  src/main.c src/keyboard.c src/midi_packet.c src/usb_midi.c
```

### Вариант 3: Полная прошивка (требуется аппаратное устройство)

Используйте `st-flash` как описано выше и подключите устройство, через которое вы сможете наблюдать MIDI трафик.

## Распространенные проблемы и решения

### 1. CMake не может найти libopencm3

#### Симптом

```
FATAL_ERROR "libopencm3 not found at <path>/stm32-midi/libopencm3"
```

#### Решение

- Проверьте инициализацию субмодулей: `git submodule update --init --recursive`
- Для ARM-сборки убедитесь, что субмодуль загружен и содержит required файлы в include/ и ld/

### 2. arm-none-eabi-gcc отсутствует

#### Симптом

```
Фатальная ошибка: unrecognized command ‘-mcpu=cortex-m3’
```

#### Решение

Установите ARM компилятор для Rust-среды (устанавливается в arch-linux-CI-pitfalls.md).

### 3. st-flash отсутствует

#### Симптом

```
Фатальная ошибка: Файл не найден: st-flash
```

#### Решение

```bash
# Arch Linux
sudo pacman -S st-link
# Debian/Ubuntu
sudo apt-get install stlink
```

### 4. Клиент MIDI не принимает данные

#### Симптом

Не наблюдает ничего, даже при нажатии клавиш.

#### Решения

- Проверьте данные на USB-хосте (используйте USB-Universal тестер MIDI)
- Проверьте связь питания USB (пины PA11/PA12, согласованные 22 Ом)
- Убедитесь, что плата установлена (светодиод VD26 должен загореться)

### 5. Некоторые клавиши работают зеркально

#### Симптом

Нажатия воспринимаются наоборот относительно схемы.

#### Решение

Проверьте физическое расположение элементов на плате и назначение линий 74HC151/74HC238 (см. hardware.md для деталей).

### 6. Все клавиши инвертированы

#### Симптом

Натиски воспринимаются как отпускания (или наоборот).

#### Решение

Проверьте ориентацию диодов и pin-коды `Y`/`W` на мультиплексорах.

### 7. USB неизвестен

#### Симптом

Хост не определяет USB-устройство.

#### Решения

- Проверьте +3.3V питание (проверка через VD26)
- Проверьте 8 МГц HSE на плате (кварц Z1)
- Убедитесь, что присутствуют pull-up на D+ (R4,R5 в соответствии со схемой)

## Распространенные переменные среды

```bash
# Для хост-тестирования (unix)
cd firmware
export CC=clang

# Для ARM-сборки (необязательно, автоматически выбирается tool-chain)
cd firmware
# Собирается автоматически в CMakeLists.txt
```

## Автоматизация проверки

Добавьте тестовый скрипт в корень проекта:

```bash
#!/bin/bash
cd "$(dirname "$0")"
cd firmware && mkdir -p build-test && cmake -S . -B build-test && \
cmake --build build-test --target midi_packets_test && \
ctest --test-dir build-test --output-on-failure
echo "Хост-тестирование успешно!"
```

## Смотрите также

- `docs/hardware.md` - полная аппаратная схема и BOM
- `docs/Schematic.pdf` - электрическая принципиальная схема
- `docs/BOM.pdf` - перечень элементов платы
- `firmware/CMakeLists.txt` - конфигурация сборки для ARM
- `firmware/README.md` - детальная документация сборки

## Версия

Этот документ соответствует коммиту: $(git log --oneline -1)
Дата изменения: $(date -u +%Y-%m-%dT%H:%M:%SZ)