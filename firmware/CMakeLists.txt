cmake_minimum_required(VERSION 3.20)

project(stm32_midi VERSION 0.1.0 LANGUAGES C)

option(STM32_MIDI_BUILD_HOST_TESTS
       "Build host tests when CMake is configured without a cross toolchain"
       ON)

set(OPENCM3_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../libopencm3"
    CACHE PATH "Path to the libopencm3 checkout")

set(FIRMWARE_SOURCES
    src/main.c
    src/keyboard.c
    src/midi_packet.c
    src/usb_midi.c)

set(MIDI_PACKET_SOURCES
    src/midi_packet.c)

enable_testing()

if(NOT CMAKE_CROSSCOMPILING AND STM32_MIDI_BUILD_HOST_TESTS)
  add_executable(midi_packets_test
                 tests/test_midi_packets.c
                 ${MIDI_PACKET_SOURCES})

  target_include_directories(midi_packets_test PRIVATE include)
  target_compile_features(midi_packets_test PRIVATE c_std_11)
  target_compile_options(midi_packets_test PRIVATE -Wall -Wextra -Werror)

  add_test(NAME midi_packets_test COMMAND midi_packets_test)
endif()

if(CMAKE_CROSSCOMPILING)
  if(NOT EXISTS "${OPENCM3_DIR}/include/libopencm3/stm32/rcc.h")
    message(FATAL_ERROR "libopencm3 not found at ${OPENCM3_DIR}")
  endif()

  if(NOT EXISTS "${OPENCM3_DIR}/ld/linker.ld.S")
    message(FATAL_ERROR "libopencm3 linker script template not found at ${OPENCM3_DIR}")
  endif()

  add_executable(stm32-midi ${FIRMWARE_SOURCES})

  set_target_properties(stm32-midi
                        PROPERTIES
                        OUTPUT_NAME "stm32-midi"
                        SUFFIX ".elf")

  target_include_directories(stm32-midi
                             PRIVATE
                             include
                             "${OPENCM3_DIR}/include")

  target_compile_features(stm32-midi PRIVATE c_std_11)
  target_compile_definitions(stm32-midi PRIVATE STM32F1 STM32F103C8)
  target_compile_options(stm32-midi
                         PRIVATE
                         -mcpu=cortex-m3
                         -mthumb
                         -msoft-float
                         -ffreestanding
                         -Os
                         -Wall
                         -Wextra
                         -Werror
                         -ffunction-sections
                         -fdata-sections)

  set(OPENCM3_STM32F1_LIBRARY
      "${OPENCM3_DIR}/lib/libopencm3_stm32f1.a")

  if(NOT EXISTS "${OPENCM3_STM32F1_LIBRARY}")
    include(ExternalProject)

    ExternalProject_Add(libopencm3_stm32f1
                        SOURCE_DIR "${OPENCM3_DIR}"
                        CONFIGURE_COMMAND ""
                        BUILD_COMMAND "${CMAKE_MAKE_PROGRAM}" -C "${OPENCM3_DIR}" lib/stm32/f1
                        BUILD_BYPRODUCTS "${OPENCM3_STM32F1_LIBRARY}"
                        INSTALL_COMMAND ""
                        EXCLUDE_FROM_ALL TRUE)
  endif()

  set(GENERATED_LINKER_SCRIPT
      "${CMAKE_CURRENT_BINARY_DIR}/generated.stm32f103c8.ld")

  add_custom_command(OUTPUT "${GENERATED_LINKER_SCRIPT}"
                     COMMAND "${CMAKE_C_COMPILER}"
                             -E
                             -mcpu=cortex-m3
                             -mthumb
                             -msoft-float
                             -DSTM32F1
                             -DSTM32F103C8
                             -D_ROM=64K
                             -D_RAM=20K
                             -D_ROM_OFF=0x08000000
                             -D_RAM_OFF=0x20000000
                             -P
                             -E "${OPENCM3_DIR}/ld/linker.ld.S"
                             -o "${GENERATED_LINKER_SCRIPT}"
                     DEPENDS "${OPENCM3_DIR}/ld/linker.ld.S"
                     VERBATIM)

  add_custom_target(generated-linker-script
                    DEPENDS "${GENERATED_LINKER_SCRIPT}")

  add_dependencies(stm32-midi generated-linker-script)

  if(NOT EXISTS "${OPENCM3_STM32F1_LIBRARY}")
    add_dependencies(stm32-midi libopencm3_stm32f1)
  endif()

  target_link_directories(stm32-midi PRIVATE "${OPENCM3_DIR}/lib")
  target_link_libraries(stm32-midi PRIVATE "${OPENCM3_STM32F1_LIBRARY}")
  target_link_options(stm32-midi
                      PRIVATE
                      -mcpu=cortex-m3
                      -mthumb
                      -msoft-float
                      -T "${GENERATED_LINKER_SCRIPT}"
                      -Wl,--gc-sections)

  add_custom_command(TARGET stm32-midi
                     POST_BUILD
                     COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:stm32-midi>"
                             "$<TARGET_FILE_DIR:stm32-midi>/stm32-midi.bin"
                     COMMAND "${CMAKE_OBJCOPY}" -O ihex "$<TARGET_FILE:stm32-midi>"
                             "$<TARGET_FILE_DIR:stm32-midi>/stm32-midi.hex"
                     VERBATIM)

  add_custom_target(flash
                    COMMAND st-flash write "$<TARGET_FILE_DIR:stm32-midi>/stm32-midi.bin" 0x08000000
                    DEPENDS stm32-midi)
endif()
