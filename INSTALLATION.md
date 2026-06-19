# Инструкция по установке STM32 MIDI

## Содержание

1. [Быстрый старт](#быстрый-старт)
2. [Инструменты](#инструменты)
3. [Настройка среды](#настройка-среды)
4. [Проверка хост-тестов](#проверка-хост-тестов)
5. [Аппаратное тестирование](#аппаратное-тестирование)
6. [Обновление и участие](#обновление-и-участие)

## Быстрый старт

```bash
cd stm32-midi/firmware
# 1. Инициализировать субмодули
git submodule update --init --recursive

# 2. Проверить хост-тесты
mkdir -p build-test
cmake -S . -B build-test
cmake --build build-test --target midi_packets_test
ctest --test-dir build-test --output-on-failure

# 3. Собрать ARM-версию (необязательно)
mkdir -p build-stm32
cmake -S . -B build-stm32 -DCMAKE_TOOLCHAIN_FILE=cmake/stm32f103c8-toolchain.cmake
cmake --build build-stm32 --target stm32-midi
```

## Инструменты

| Инструмент | Назначение | Установка (Arch Linux) |
|------------|----------|----------------------|
| CMake | Системы сборки | `sudo pacman -S cmake` |
| arm-none-eabi-gcc | ARM компилятор | `sudo pacman -S gcc-arm-none-eabi` |
| OpenOCD | Отладка и прошивка | `sudo pacman -S openocd stlink` |
| st-flash | Альтернативная прошивка | `sudo pacman -S st-link` |
| clang | Хост-тесты | `sudo pacman -S clang` |

**Предпочтительно:** OpenOCD для общего доступа к отладке, st-flash для быстрой проверки.

## Настройка среды

### Export переменных среды (необязательно)

```bash
export PATH="/opt/arm-none-eabi/bin:$PATH"

# Для OpenOCD (часто требуется для систем на базе π0)
sudo usermod -aG wheel $USER
# Затем войдите снова, чтобы получить Wheel-group permissions
```

### Системы на базе Arch Linux (без AUR)

```bash
# Полноценный стек инструментов
 paru -S cmake gcc-arm-none-eabi openocd stlink clang
```

**Советы по установке:**

```bash
# Проверьте доступные инструменты
command -v openocd && echo "OpenOCD: доступен"
command -v st-flash && echo "st-flash: доступен"
command -v arm-none-eabi-gcc && echo "ARM GCC: доступен"
```

## Проверка хост-тестов

Хост-тесты работают в любой C-среде без ARM-процессора, что делает их идеальными для CI, предварительной проверки и обучения.

```bash
cd firmware
mkdir -p build-test
cmake -S . -B build-test
cmake --build build-test --target midi_packets_test
ctest --test-dir build-test --output-on-failure
```

**Результат:**
- ✅ `midi_packets_test` должен пройти с сообщением "passed"
- ✅ Проверка MIDI packet builder и обработки данных

## Аппаратное тестирование

### Предварительное условие: Соберите целевой ARM-бинарный файл

```bash
cd firmware
mkdir -p build-stm32
cmake -S . -B build-stm32 -DCMAKE_TOOLCHAIN_FILE=cmake/stm32f103c8-toolchain.cmake
cmake --build build-stm32 --target stm32-midi
```

**Целевые файлы:**

```
build-stm32/stm32-midi.elf      # Прошивка ARM образ
build-stm32/stm32-midi.bin      # Бинарный образ
build-stm32/stm32-midi.hex      # HEX образ
```

### Прошивка с использованием OpenOCD (рекомендуется)

```bash
cd firmware
openocd -f interface/st-link.cfg -f target/stm32f1x.cfg \
  -c "program build-stm32/stm32-midi.elf verify reset run" \
  -c "shutdown"
```

**Скрипт прошивки (повторно используемый):**

```bash
cd stm32-midi/firmware
if [ ! -f "build-stm32/stm32-midi.elf" ]; then
    echo "❌ Файл сборки не найден!"
    exit 1
fi

echo "👉 Использование OpenOCD..."
openocd -f interface/st-link.cfg -f target/stm32f1x.cfg \
    -c "program build-stm32/stm32-midi.elf verify reset run" \
    -c "echo Прошивка завершена" \
    -c "shutdown"

echo "✅ Прошивка STM32 MIDI успешно завершена"
```

### Прошивка с использованием st-flash (альтернативный метод)

```bash
cd firmware
st-flash write build-stm32/stm32-midi.elf 0x08000000
```

## Проблемы и устранение неполадок

### Q1: Что, если OpenOCD не работает?

```bash
# Установите OpenOCD правильно
sudo pacman -S openocd stlink

# Проверьте его работу
openocd -f interface/st-link.cfg -f target/stm32f1x.cfg \
  -c "init" -c "halt" -c "shutdown"
```

### Q2: Что, если CMake не может найти libopencm3?

```bash
cd stm32-midi/firmware
git submodule update --init --recursive
# Перейдите в build-test, повторите команду
cmake -S . -B build-test
```

### Q3: Что делать, если сборка ARM-файла останавливается?

```bash
# Экспортируйте ARM-путь, если необходимо
export PATH="/opt/arm-none-eabi/bin:$PATH"

# Проверьте, что требуется
cd stm32-midi/firmware
mkdir -p build-stm32
cmake -S . -B build-stm32 -DCMAKE_TOOLCHAIN_FILE=cmake/stm32f103c8-toolchain.cmake
```

### Q4: Что, если st-flash недоступен?

```bash
# Для Arch Linux (без AUR)
sudo pacman -S st-link
```

## Обновление и участие

### Обновление в CI

Добавьте `firmware/openocd-flash.sh` в ваш репозиторий CI:

```bash
#!/bin/bash
cd "$(dirname "$0")"
sudo pacman -S openocd stlink
./openocd-flash.sh
```

### Вклад в документацию

Вы можете улучшить это руководство, добавив:

- Дополнительные примеры с использованием `gdb` (с OpenOCD)
- Инструкции по отладке для аппаратных ошибок
- Пробные версии исследований FPGA/платы

## Получить помощь

1. **Хост-тесты:** Разберитесь с `midi_packets_test` напрямую
2. **OpenOCD:** Посетите форум OpenOCD, задайте вопрос `gdb` в вашей среде разработки
3. **stm32-midi:** Создайте экземпляр для конкретной платы для более быстрой отладки

## Версия

Автор: Человеческая обработка на основе редакции Gemini
Обновлено: $(date -u +%Y-%m-%dT%H:%M:%SZ)

## Смотрите также

- `README.md` — быстрый обзор и информация о стейкхолдерах
- `TESTING_INSTRUCTIONS.md` — подробные проверки и порты
- `docs/hardware.md` — полное аппаратное обеспечение
- `docs/Schematic.pdf` — схемы и распределение
- `docs/BOM.pdf` — перечень элементов платы