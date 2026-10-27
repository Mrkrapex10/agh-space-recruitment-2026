cmake_minimum_required(VERSION 3.21)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Works both with "cmake --preset Debug" and with a plain "cmake -B build -G Ninja"
if(NOT CMAKE_TOOLCHAIN_FILE)
    include("${CMAKE_CURRENT_LIST_DIR}/cmake/gcc-arm-none-eabi.cmake")
endif()
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

project(agh_firmware C ASM)

add_executable(${PROJECT_NAME})

# ---- MCU settings ----
set(MCU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)

target_compile_definitions(${PROJECT_NAME} PRIVATE USE_HAL_DRIVER STM32G474xx
    $<$<CONFIG:Debug>:DEBUG>)

target_compile_options(${PROJECT_NAME} PRIVATE
    ${MCU_FLAGS}
    -Wall -Wextra
    -ffunction-sections -fdata-sections -fno-common
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-Os>)

target_link_options(${PROJECT_NAME} PRIVATE
    ${MCU_FLAGS}
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32G474RETx_FLASH.ld
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    --specs=nano.specs
    --specs=nosys.specs)

target_include_directories(${PROJECT_NAME} PRIVATE
    Core/Inc
    App/Inc
    lib/tlv
    Drivers/STM32G4xx_HAL_Driver/Inc
    Drivers/CMSIS/Device/ST/STM32G4xx/Include
    Drivers/CMSIS/Include)

# ---- sources ----
set(HAL_SRC_DIR Drivers/STM32G4xx_HAL_Driver/Src)
target_sources(${PROJECT_NAME} PRIVATE
    # startup + system
    startup/startup_stm32g474xx.s
    Core/Src/system_stm32g4xx.c
    # CubeMX-style peripheral init
    Core/Src/main.c
    Core/Src/gpio.c
    Core/Src/dma.c
    Core/Src/tim.c
    Core/Src/usart.c
    Core/Src/stm32g4xx_it.c
    Core/Src/stm32g4xx_hal_msp.c
    # application (tasks 2.1 and 2.2)
    App/Src/app.c
    App/Src/led_effects.c
    App/Src/msg_handler.c
    App/Src/uart_link.c
    App/Src/ws2812.c
    lib/tlv/tlv.c
    # HAL
    ${HAL_SRC_DIR}/stm32g4xx_hal.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_cortex.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_dma.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_dma_ex.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_exti.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_flash.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_flash_ex.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_flash_ramfunc.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_gpio.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_pwr.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_pwr_ex.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_rcc.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_rcc_ex.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_tim.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_tim_ex.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_uart.c
    ${HAL_SRC_DIR}/stm32g4xx_hal_uart_ex.c)

# ---- post-build: .hex / .bin + size ----
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>)
