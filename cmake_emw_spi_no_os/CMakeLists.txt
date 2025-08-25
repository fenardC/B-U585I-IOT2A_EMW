cmake_minimum_required(VERSION 3.28)

set(PROJECT_NAME emw_spi_no_os)

# Customize the toolchain
set(MCPU_FLAGS "-mcpu=cortex-m33 -mthumb")
set(VFP_FLAGS "-mfloat-abi=hard -mfpu=fpv5-sp-d16")
set(LD_FLAGS "-T${CMAKE_CURRENT_SOURCE_DIR}/STM32U585AIIXQ_FLASH.ld -Wl,-Map=${PROJECT_NAME}.map")
set(SPEC_FLAGS "--specs=nosys.specs") ## "--specs=nano_c_standard_cpp.specs"

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../cmake_hardware_board/arm-gcc-cortex-toolchain.cmake")


project(${PROJECT_NAME} C CXX ASM)

add_definitions(-DCOMPILATION_WITH_EMW -DCOMPILATION_WITH_NO_OS -DCOMPILATION_WITH_SPI -DSTM32U585xx -DUSE_HAL_DRIVER )
add_compile_options("-Wextra" "-pedantic" "-Wmissing-declarations" "-Wredundant-decls")
#add_compile_options("-v")

set(APPLICATION_CORE_INC_PATH "${CMAKE_SOURCE_DIR}/../core/inc")
set(APPLICATION_CORE_SRC_PATH "${CMAKE_SOURCE_DIR}/../core/src")
set(APPLICATION_EMW_INC_PATH "${CMAKE_SOURCE_DIR}/../applications/emw")
set(APPLICATION_EMW_SRC_PATH "${CMAKE_SOURCE_DIR}/../applications/emw")
set(DRIVER_CMSIS_INC_PATH "${CMAKE_SOURCE_DIR}/../drivers/cmsis/include")
set(DRIVER_CMSIS_DEVICE_STM32U5_INC_PATH "${CMAKE_SOURCE_DIR}/../drivers/cmsis/device/st/stm32u5xx/include")
set(DRIVER_EMW_INC_PATH "${CMAKE_SOURCE_DIR}/../drivers/emw")
set(DRIVER_EMW_SRC_PATH "${CMAKE_SOURCE_DIR}/../drivers/emw")
set(DRIVER_HAL_STM32U5_INC_PATH "${CMAKE_SOURCE_DIR}/../drivers/stm32u5xx_hal_driver/inc")
set(DRIVER_HAL_STM32U5_SRC_PATH "${CMAKE_SOURCE_DIR}/../drivers/stm32u5xx_hal_driver/src")
set(DRIVER_LIB_INC_PATH "${CMAKE_SOURCE_DIR}/../lib")
set(DRIVER_LIB_SRC_PATH "${CMAKE_SOURCE_DIR}/../lib")

add_executable(${PROJECT_NAME}
  ${APPLICATION_CORE_SRC_PATH}/Console.cpp
  ${APPLICATION_CORE_SRC_PATH}/dcache.c
  ${APPLICATION_CORE_SRC_PATH}/gpdma.c
  ${APPLICATION_CORE_SRC_PATH}/gpio.c
  ${APPLICATION_CORE_SRC_PATH}/icache.c
  ${APPLICATION_CORE_SRC_PATH}/main.cpp
  ${APPLICATION_CORE_SRC_PATH}/rng.c
  ${APPLICATION_CORE_SRC_PATH}/spi.c
  ${APPLICATION_CORE_SRC_PATH}/startup_stm32u585aiixq.s
  ${APPLICATION_CORE_SRC_PATH}/stm32u5xx_hal_msp.c
  ${APPLICATION_CORE_SRC_PATH}/stm32u5xx_hal_timebase_tim.c
  ${APPLICATION_CORE_SRC_PATH}/stm32u5xx_it.c
  ${APPLICATION_CORE_SRC_PATH}/system_stm32u5xx.c
  ${APPLICATION_CORE_SRC_PATH}/tim.c
  ${APPLICATION_CORE_SRC_PATH}/usart.c
  ${APPLICATION_EMW_SRC_PATH}/AppConsoleDownload.cpp
  ${APPLICATION_EMW_SRC_PATH}/AppConsoleEcho.cpp
  ${APPLICATION_EMW_SRC_PATH}/AppConsolePing.cpp
  ${APPLICATION_EMW_SRC_PATH}/AppConsoleScan.cpp
  ${APPLICATION_EMW_SRC_PATH}/AppConsoleStats.cpp
  ${APPLICATION_EMW_SRC_PATH}/AppWiFiEmw.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwAddress.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwApiCore.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwApiEmw.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwCoreHci.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwCoreIpc.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwIoHardware.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwIoSpi.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwNetworkEmwImplementation.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwOsNoOSImplementation.cpp
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_cortex.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_dcache.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_dma.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_dma_ex.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_exti.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_flash.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_flash_ex.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_gpio.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_icache.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_pwr.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_pwr_ex.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_rcc.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_rcc_ex.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_rng.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_rng_ex.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_spi.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_spi_ex.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_tim.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_tim_ex.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_uart.c
  ${DRIVER_HAL_STM32U5_SRC_PATH}/stm32u5xx_hal_uart_ex.c
  ${DRIVER_LIB_SRC_PATH}/stdin_uart.c
  ${DRIVER_LIB_SRC_PATH}/stdout_uart.c
  ${DRIVER_LIB_SRC_PATH}/syscalls.c
  ${DRIVER_LIB_SRC_PATH}/sysmem.c
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  ${APPLICATION_CORE_INC_PATH}
  ${APPLICATION_EMW_INC_PATH}
  ${DRIVER_CMSIS_INC_PATH}
  ${DRIVER_CMSIS_DEVICE_STM32U5_INC_PATH}
  ${DRIVER_EMW_INC_PATH}
  ${DRIVER_HAL_STM32U5_INC_PATH}
  ${DRIVER_LIB_INC_PATH}
)

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX .elf)

add_custom_command(TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_SIZE_UTIL} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}>
)

add_custom_command(TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
)

