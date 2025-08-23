cmake_minimum_required(VERSION 3.28)

set(PROJECT_NAME emw_spi_lwip_freertos)

# Customize the toolchain
set(MCPU_FLAGS "-mcpu=cortex-m33 -mthumb")
set(VFP_FLAGS "-mfloat-abi=hard -mfpu=fpv5-sp-d16")
set(LD_FLAGS "-T${CMAKE_CURRENT_SOURCE_DIR}/STM32U585AIIXQ_FLASH.ld -Wl,-Map=${PROJECT_NAME}.map")
set(SPEC_FLAGS "--specs=nosys.specs") ## "--specs=nano_c_standard_cpp.specs"

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../cmake_hardware_board/arm-gcc-cortex-toolchain.cmake")


project(${PROJECT_NAME} C CXX ASM)

add_definitions(-DCOMPILATION_WITH_LWIP -DCOMPILATION_WITH_FREERTOS -DCOMPILATION_WITH_SPI -DSTM32U585xx -DUSE_HAL_DRIVER -DSTM32_THREAD_SAFE_STRATEGY=4)
add_compile_options("-Wextra" "-pedantic" "-Wmissing-declarations" "-Wredundant-decls")
#add_compile_options("-v")

set(APPLICATION_CORE_INC_PATH "${CMAKE_SOURCE_DIR}/../core/inc")
set(APPLICATION_CORE_SRC_PATH "${CMAKE_SOURCE_DIR}/../core/src")
set(APPLICATION_LWIP_INC_PATH "${CMAKE_SOURCE_DIR}/../applications/lwip")
set(APPLICATION_LWIP_SRC_PATH "${CMAKE_SOURCE_DIR}/../applications/lwip")
set(DRIVER_CMSIS_INC_PATH "${CMAKE_SOURCE_DIR}/../drivers/cmsis/include")
set(DRIVER_CMSIS_DEVICE_STM32U5_INC_PATH "${CMAKE_SOURCE_DIR}/../drivers/cmsis/device/st/stm32u5xx/include")
set(DRIVER_EMW_INC_PATH "${CMAKE_SOURCE_DIR}/../drivers/emw")
set(DRIVER_EMW_SRC_PATH "${CMAKE_SOURCE_DIR}/../drivers/emw")
set(DRIVER_HAL_STM32U5_INC_PATH "${CMAKE_SOURCE_DIR}/../drivers/stm32u5xx_hal_driver/inc")
set(DRIVER_HAL_STM32U5_SRC_PATH "${CMAKE_SOURCE_DIR}/../drivers/stm32u5xx_hal_driver/src")
set(DRIVER_LIB_INC_PATH "${CMAKE_SOURCE_DIR}/../lib")
set(DRIVER_LIB_SRC_PATH "${CMAKE_SOURCE_DIR}/../lib")
set(FREERTOS_KERNEL_INC_PATH "${CMAKE_SOURCE_DIR}/../middlewares/freertos/source/include")
set(FREERTOS_KERNEL_SRC_PATH "${CMAKE_SOURCE_DIR}/../middlewares/freertos/source")
set(FREERTOS_KERNEL_PORT_INC_PATH "${CMAKE_SOURCE_DIR}/../middlewares/freertos/source/portable/GCC/ARM_CM33_NTZ/non_secure")
set(FREERTOS_KERNEL_PORT_SRC_PATH "${CMAKE_SOURCE_DIR}/../middlewares/freertos/source/portable/GCC/ARM_CM33_NTZ/non_secure")
set(FREERTOS_KERNEL_MEM_MAN_SRC_PATH "${CMAKE_SOURCE_DIR}/../middlewares/freertos/source/portable/MemMang")
set(LWIP_INC_PATH "${CMAKE_SOURCE_DIR}/../middlewares/lwip/src/include")
set(LWIP_SRC_PATH "${CMAKE_SOURCE_DIR}/../middlewares/lwip/src")
set(LWIP_CONTRIB_PORT_INC_PATH "${CMAKE_SOURCE_DIR}/../middlewares/lwip/contrib/ports/freertos/include")
set(LWIP_CONTRIB_PORT_SRC_PATH "${CMAKE_SOURCE_DIR}/../middlewares/lwip/contrib/ports/freertos")

add_executable(${PROJECT_NAME}
  ${APPLICATION_CORE_SRC_PATH}/app_freertos.c
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
  ${APPLICATION_LWIP_SRC_PATH}/AppConsoleEcho.cpp
  ${APPLICATION_LWIP_SRC_PATH}/AppConsolePing.cpp
  ${APPLICATION_LWIP_SRC_PATH}/AppConsoleIperf.cpp
  ${APPLICATION_LWIP_SRC_PATH}/AppConsoleScan.cpp
  ${APPLICATION_LWIP_SRC_PATH}/AppConsoleStats.cpp
  ${APPLICATION_LWIP_SRC_PATH}/AppLwipServices.cpp
  ${APPLICATION_LWIP_SRC_PATH}/AppWiFiLwip.cpp
  ${APPLICATION_LWIP_SRC_PATH}/lwip_wifi/emw/WiFiNetwork.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwAddress.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwApiCore.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwApiEmwBypass.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwCoreHci.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwCoreIpc.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwIoHardware.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwIoSpi.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwNetworkLwipImplementation.cpp
  ${DRIVER_EMW_SRC_PATH}/EmwOsFreeRTOSImplementation.cpp
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
  ${DRIVER_LIB_SRC_PATH}/newlib_lock_glue.c
  ${DRIVER_LIB_SRC_PATH}/stdin_uart.c
  ${DRIVER_LIB_SRC_PATH}/stdout_uart.c
  ${DRIVER_LIB_SRC_PATH}/syscalls.c
  ${DRIVER_LIB_SRC_PATH}/sysmem.c
  ${FREERTOS_KERNEL_SRC_PATH}/croutine.c
  ${FREERTOS_KERNEL_SRC_PATH}/event_groups.c
  ${FREERTOS_KERNEL_SRC_PATH}/list.c
  ${FREERTOS_KERNEL_SRC_PATH}/queue.c
  ${FREERTOS_KERNEL_SRC_PATH}/stream_buffer.c
  ${FREERTOS_KERNEL_SRC_PATH}/tasks.c
  ${FREERTOS_KERNEL_SRC_PATH}/timers.c
  ${FREERTOS_KERNEL_PORT_SRC_PATH}/port.c
  ${FREERTOS_KERNEL_PORT_SRC_PATH}/portasm.c
  ${FREERTOS_KERNEL_MEM_MAN_SRC_PATH}/heap_4.c
  ${LWIP_SRC_PATH}/api/api_lib.c
  ${LWIP_SRC_PATH}/api/api_msg.c
  ${LWIP_SRC_PATH}/api/err.c
  ${LWIP_SRC_PATH}/api/if_api.c
  ${LWIP_SRC_PATH}/api/netbuf.c
  ${LWIP_SRC_PATH}/api/netdb.c
  ${LWIP_SRC_PATH}/api/netifapi.c
  ${LWIP_SRC_PATH}/api/sockets.c
  ${LWIP_SRC_PATH}/api/tcpip.c
  ${LWIP_SRC_PATH}/core/altcp.c
  ${LWIP_SRC_PATH}/core/altcp_alloc.c
  ${LWIP_SRC_PATH}/core/altcp_tcp.c
  ${LWIP_SRC_PATH}/core/def.c
  ${LWIP_SRC_PATH}/core/dns.c
  ${LWIP_SRC_PATH}/core/inet_chksum.c
  ${LWIP_SRC_PATH}/core/init.c
  ${LWIP_SRC_PATH}/core/ip.c
  ${LWIP_SRC_PATH}/core/mem.c
  ${LWIP_SRC_PATH}/core/memp.c
  ${LWIP_SRC_PATH}/core/netif.c
  ${LWIP_SRC_PATH}/core/pbuf.c
  ${LWIP_SRC_PATH}/core/raw.c
  ${LWIP_SRC_PATH}/core/stats.c
  ${LWIP_SRC_PATH}/core/sys.c
  ${LWIP_SRC_PATH}/core/tcp.c
  ${LWIP_SRC_PATH}/core/tcp_in.c
  ${LWIP_SRC_PATH}/core/tcp_out.c
  ${LWIP_SRC_PATH}/core/timeouts.c
  ${LWIP_SRC_PATH}/core/udp.c
  ${LWIP_SRC_PATH}/netif/bridgeif.c
  ${LWIP_SRC_PATH}/netif/bridgeif_fdb.c
  ${LWIP_SRC_PATH}/netif/ethernet.c
  ${LWIP_SRC_PATH}/netif/lowpan6.c
  ${LWIP_SRC_PATH}/netif/lowpan6_ble.c
  ${LWIP_SRC_PATH}/netif/lowpan6_common.c
  ${LWIP_SRC_PATH}/netif/slipif.c
  ${LWIP_SRC_PATH}/netif/zepif.c
  ${LWIP_SRC_PATH}/apps/lwiperf/lwiperf.c
  ${LWIP_SRC_PATH}/core/ipv4/acd.c
  ${LWIP_SRC_PATH}/core/ipv4/autoip.c
  ${LWIP_SRC_PATH}/core/ipv4/dhcp.c
  ${LWIP_SRC_PATH}/core/ipv4/etharp.c
  ${LWIP_SRC_PATH}/core/ipv4/icmp.c
  ${LWIP_SRC_PATH}/core/ipv4/igmp.c
  ${LWIP_SRC_PATH}/core/ipv4/ip4.c
  ${LWIP_SRC_PATH}/core/ipv4/ip4_addr.c
  ${LWIP_SRC_PATH}/core/ipv4/ip4_frag.c
  ${LWIP_SRC_PATH}/core/ipv6/dhcp6.c
  ${LWIP_SRC_PATH}/core/ipv6/ethip6.c
  ${LWIP_SRC_PATH}/core/ipv6/icmp6.c
  ${LWIP_SRC_PATH}/core/ipv6/inet6.c
  ${LWIP_SRC_PATH}/core/ipv6/ip6.c
  ${LWIP_SRC_PATH}/core/ipv6/ip6_addr.c
  ${LWIP_SRC_PATH}/core/ipv6/ip6_frag.c
  ${LWIP_SRC_PATH}/core/ipv6/mld6.c
  ${LWIP_SRC_PATH}/core/ipv6/nd6.c
  ${LWIP_SRC_PATH}/netif/ppp/auth.c
  ${LWIP_SRC_PATH}/netif/ppp/ccp.c
  ${LWIP_SRC_PATH}/netif/ppp/chap-md5.c
  ${LWIP_SRC_PATH}/netif/ppp/chap-new.c
  ${LWIP_SRC_PATH}/netif/ppp/chap_ms.c
  ${LWIP_SRC_PATH}/netif/ppp/demand.c
  ${LWIP_SRC_PATH}/netif/ppp/eap.c
  ${LWIP_SRC_PATH}/netif/ppp/ecp.c
  ${LWIP_SRC_PATH}/netif/ppp/eui64.c
  ${LWIP_SRC_PATH}/netif/ppp/fsm.c
  ${LWIP_SRC_PATH}/netif/ppp/ipcp.c
  ${LWIP_SRC_PATH}/netif/ppp/ipv6cp.c
  ${LWIP_SRC_PATH}/netif/ppp/lcp.c
  ${LWIP_SRC_PATH}/netif/ppp/magic.c
  ${LWIP_SRC_PATH}/netif/ppp/mppe.c
  ${LWIP_SRC_PATH}/netif/ppp/multilink.c
  ${LWIP_SRC_PATH}/netif/ppp/ppp.c
  ${LWIP_SRC_PATH}/netif/ppp/pppapi.c
  ${LWIP_SRC_PATH}/netif/ppp/pppcrypt.c
  ${LWIP_SRC_PATH}/netif/ppp/pppoe.c
  ${LWIP_SRC_PATH}/netif/ppp/pppol2tp.c
  ${LWIP_SRC_PATH}/netif/ppp/pppos.c
  ${LWIP_SRC_PATH}/netif/ppp/upap.c
  ${LWIP_SRC_PATH}/netif/ppp/utils.c
  ${LWIP_SRC_PATH}/netif/ppp/vj.c
  ${LWIP_SRC_PATH}/netif/ppp/polarssl/arc4.c
  ${LWIP_SRC_PATH}/netif/ppp/polarssl/des.c
  ${LWIP_SRC_PATH}/netif/ppp/polarssl/md4.c
  ${LWIP_SRC_PATH}/netif/ppp/polarssl/md5.c
  ${LWIP_SRC_PATH}/netif/ppp/polarssl/sha1.c
  ${LWIP_CONTRIB_PORT_SRC_PATH}/sys_arch.c
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  ${APPLICATION_CORE_INC_PATH}
  ${APPLICATION_LWIP_INC_PATH}
  ${APPLICATION_LWIP_INC_PATH}/lwip_wifi/emw/
  ${DRIVER_CMSIS_INC_PATH}
  ${DRIVER_CMSIS_DEVICE_STM32U5_INC_PATH}
  ${DRIVER_EMW_INC_PATH}
  ${DRIVER_HAL_STM32U5_INC_PATH}
  ${DRIVER_LIB_INC_PATH}
  ${FREERTOS_KERNEL_INC_PATH}
  ${FREERTOS_KERNEL_PORT_INC_PATH}
  ${LWIP_INC_PATH}
  ${LWIP_CONTRIB_PORT_INC_PATH}
)

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX .elf)

add_custom_command(TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_SIZE_UTIL} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}>
)

add_custom_command(TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_OBJDUMP} -h -S $<TARGET_FILE:${PROJECT_NAME}> > ${PROJECT_NAME}.list
)

add_custom_command(TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
)

