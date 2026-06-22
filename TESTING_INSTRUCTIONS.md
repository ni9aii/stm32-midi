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

### С использованием OpenOCD (предпочтительно)

OpenOCD (Open On-Chip Debugger) — это более универсальный и поддерживаемый инструмент для отладки STM32 через SWD, JTAG и другие интерфейсы, чем st-flash. Он поддерживает управление подключением, отладку и запись, а также совместим с множеством инструментов отладки от мира.

#### Минимальный скрипт OpenOCD для stlink

Создайте `openocd-stlink.cfg` в директории `firmware/`:

```bash
cat > firmware/openocd-stlink.cfg <<'EOF'
# STM32F103C8T6 через ST-Link/V2 на GPIO (SWDIO/PA13, SWCLK/PA14)
source [find interface/st-link.cfg]

# Настройки адаптера
adapter speed 2000
adapter srst pullup 0

# Конфигурации цели
target using-stm32f1x
target steady-clock 72000000
EOF
```

#### Прошивка с использованием OpenOCD

```bash
cd firmware
# Простая команда для прошивки через st-link с использованием OpenOCD
openocd -f interface/st-link.cfg -f target/stm32f1x.cfg \
  -c "program build-stm32/stm32-midi.elf verify reset run" \
  -c "shutdown"
```

#### Расширенная инструкция OpenOCD со всеми опциями

```bash
cd firmware
# Полная инструкция с обработкой ошибок
openocd -f interface/st-link.cfg -f target/stm32f1x.cfg \
  -c "adapter speed 2000" \
  -c "adapter srst pullup 0" \
  -c "init" \
  -c "reset halt" \
  -c "flash write_image erase build-stm32/stm32-midi.elf" \
  -c "reset run" \
  -c "shutdown"
```

#### Использование пользовательского скрипта OpenOCD

Создайте `flash-script.sh` для повторного использования:

```bash
#!/bin/bash
cd "$(dirname "$0")"
if [ ! -f "build-stm32/stm32-midi.elf" ]; then
  echo "Файл сборки не найден! Запустите процесс сначала"
  exit 1
fi

echo "Подключение через OpenOCD со st-link..."
openocd -f interface/st-link.cfg -f target/stm32f1x.cfg \
  -c "adapter speed 2000" \
  -c "adapter srst pullup 0" \
  -c "init" \
  -c "reset halt" \
  -c "program build-stm32/stm32-midi.elf verify reset run" \
  -c "echo Прошивка завершена" \
  -c "shutdown"

echo "✅ Прошивка STM32 MIDI успешно завершена через OpenOCD"
```

### Альтернатива: st-flash (для систем без OpenOCD)

Если OpenOCD недоступен, используйте st-flash (требует установки `stlink`):

```bash
cd firmware
st-flash write build-stm32/stm32-midi.bin 0x08000000
```

**Советы по установке для систем на базе Arch Linux (без AUR):**

```bash
# Для OpenOCD (предпочтительно)
sudo pacman -S openocd stlink
# Для st-flash
sudo pacman -S st-link
```

## Что предпочтительнее: OpenOCD против st-flash?

| Функция | OpenOCD | st-flash |
|---------|---------|----------|
| Интерфейсы | SWD, JTAG, Serial Wire J TAG, ARM-JTAG | USB-Serial (коммутируемые) |
| Контроль | Стоп на точке, запись памяти, отладка | Простая запись |
| Кросс-платформенность | Поддерживается в Linux, macOS, Windows | Преимущественно Linux |
| Инструменты отладки | GDB, Pestate и другие | Примитивное CLI |
| Поддержка оборудования | ST-Link/V2, CMSIS-DAP, J-Tag | ST-Link/V2 |
| Производительность | Более высокая стабильность | Быстрая проверка |

## Распространенные проблемы и решения с OpenOCD

### 1. Не удается подключиться к STM32

**Симптом:** `Хост не определяется` или `Cannot connect to target`

**Решение:**

1. Проверьте подключение SWD wires:
   - SWDIO <-> PA13
   - SWCLK <-> PA14
   - NRST <-> PA0/BOOT0 (опционально)

2. Убедитесь, что OpenOCD обнаруживает устройство:

```bash
openocd -f interface/st-link.cfg -f target/stm32f1x.cfg \
  -c "init" -c "halt" -c "echo Процесс HALTED" -c "shutdown"
```

3. Исправьте скорость адаптера по необходимости:

```bash
openocd -f interface/st-link.cfg -f target/stm32f1x.cfg \
  -c "adapter speed 1000" -c "init"
```

### 2. Прошивка не удается

**Симптом:** `Cannot program memory`, `Flash write error`

**Решение:**

1. Используйте команду `verify` после записи:

```bash
openocd -f interface/st-link.cfg -f target/stm32f1x.cfg \
  -c "program build-stm32/stm32-midi.elf verify"
```

2. Прошивка с использованием skip-прошивки (если устройство заблокировано):

```bash
openocd -f interface/st-link.cfg -f target/stm32f1x.cfg \
  -c "reset halt" \
  -c "flash write_image erase build-stm32/stm32-midi.elf" \
  -c "reset run"
```

### 3. Частые проблемы с совместимостью

**Проблема:** `Фатальная ошибка: Переопределено "init"`

**Решение:** Отключите и очистите соединение повторно:

```bash
openocd -f interface/st-link.cfg -f target/stm32f1x.cfg \
  -c "shutdown" -c "init" -c "halt"
```

## Страница выбора прошивателя (для автоматизации)

Создайте `firmware/index.md` для ориентации:

```markdown
# Выбор прошивателя для STM32 MIDI

## Когда использовать OpenOCD

- ✅ ARM-тип должен производить прошивку (штатные борты)
- ✅ Требуется отладка (остановка на точке, контроль памяти)
- ✅ Поддержка различных типчиков (ST-Link, CMSIS-DAP, ARM-JTAG)
- ✅ Хосты CI, которые могут установить OpenOCD

```bash
# Хост CI (Arch, Debian, Ubuntu)
sudo apt-get install openocd
```

## Когда использовать st-flash

- ✅ Простая драйверная прошивка без отладки
- ✅ Мобильные устройства (если отсутствует USB CI)
- ✅ Вредоустойчивость для железа (нет удаленного доступа)

```bash
# Для Arch (без AUR)
sudo pacman -S st-link
```

## Быстрый выбор последовательно

Выполните команду ниже, чтобы узнать, что доступно:

```bash
#!/bin/bash
# Пытаемся использовать OpenOCD, затем st-flash
cd firmware
if command -v openocd >/dev/null && command -v st-link >/dev/null; then
  echo "Обнаружен оба OpenOCD и st-link. Что вы предпочитаете:"
  echo "1) OpenOCD (рекомендуется)"
  echo "2) st-flash (быстрая проверка)"
  read -p "Выбор (1/2): " choice
  case $choice in
    1) openocd -f interface/st-link.cfg -f target/stm32f1x.cfg -c "program build-stm32/stm32-midi.elf verify reset run" -c "shutdown" ;;
    2) st-flash write build-stm32/stm32-midi.bin 0x08000000 ;;
    *) echo "Отменено" ;;
  esac
elif command -v openocd >/dev/null; then
  echo "Используется OpenOCD..."
  openocd -f interface/st-link.cfg -f target/stm32f1x.cfg -c "program build-stm32/stm32-midi.elf verify reset run" -c "shutdown"
elif command -v st-link >/dev/null; then
  echo "Используется st-flash..."
  st-flash write build-stm32/stm32-midi.bin 0x08000000
else
  echo "Ни st-link (st-flash) ни OpenOCD не установлены"
  echo "Выполните: sudo pacman -S openocd (рекомендуется)"
  exit 1
fi
```

## Интерфейсы OpenOCD подробнее

### st-link (встроенный в стандартную версию)

```bash
# ST-Link на GPIO (самая простая сборка)
openocd -f interface/st-link.cfg -f target/stm32f1x.cfg
```

### CMSIS-DAP (для систем на базе nRF52/х 32)

```bash
# CMSIS-DAP строится автоматически при использовании stm32-posix
openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg
```

### JTAG (изолированный серийный интерфейс)

```bash
# Для мостиков ARM JTAG
openocd -f interface/jtag.cfg -f target/stm32f1x.cfg
```

## Некоторые часто используемые утилиты OpenOCD

**Проверить идентификаторы устройств:**

```bash
openocd -f interface/st-link.cfg -f target/stm32f1x.cfg -c "targets" -c "shutdown"
```

**Считать флеш-память (для проверки):**

```bash
openocd -f interface/st-link.cfg -f target/stm32f1x.cfg \
  -c "init" \
  -c "halt" \
  -c "flash info" \
  -c "shutdown"
```

**Считать регистр (регистр идентификатора устройства):**

```bash
openocd -f interface/st-link.cfg -f target/stm32f1x.cfg \
  -c "init" \
  -c "reg read r0" \
  -c "shutdown"
```

## Обработка ошибок в режиме реального времени

**Сценарий с OpenOCD для CI:**

```bash
#!/bin/bash
cd "$(dirname "$0")"

set -e  # Критичные ошибки остановлены

echo "=== STM32 MIDI Прошивка через OpenOCD ==="

if [ ! -f "build-stm32/stm32-midi.elf" ]; then
  echo "❌ Файл ARM-elf не найден"
  echo "Запустите процесс сначала: cmake -S . -B build-stm32 -DCMAKE_TOOLCHAIN_FILE=cmake/stm32f103c8-toolchain.cmake && cmake --build build-stm32 --target stm32-midi"
  exit 1
fi

# Использовать OpenOCD, если он доступен
if command -v openocd >/dev/null; then
  echo "✅ OpenOCD обнаружен, использование OpenOCD..."
  # Выполнять графическую обработку ошибок (для локальной разработки, но не для CI)
  if command -v gdb >/dev/null; then
    openocd -f interface/st-link.cfg -f target/stm32f1x.cfg \
      -c "program build-stm32/stm32-midi.elf verify reset run" \
      -c "echo Процесс завершен" \
      -c "shutdown"
  else
    openocd -f interface/st-link.cfg -f target/stm32f1x.cfg \
      -c "program build-stm32/stm32-midi.elf verify reset run" \
      -c "shutdown"
  fi
else
  echo "⚠️  OpenOCD недоступен, использование st-flash..."
  if command -v st-flash >/dev/null; then
    st-flash write build-stm32/stm32-midi.bin 0x08000000
    echo "✅ st-flash завершен"
  else
    echo "❌ Ни OpenOCD, ни st-link не установлены"
    echo "Запустите: sudo pacman -S openocd st-link"
    exit 1
  fi
fi

echo "✅ Прошивка STM32 MIDI успешно завершена"
```

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