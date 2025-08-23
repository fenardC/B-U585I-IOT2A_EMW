# B-U585I-IOT2A_EMW
Wi-Fi examples and driver written with C/C++ languages

# Abstract

This project aims at demonstrating some Wi-Fi examples and driver written with C/C++ languages.

> [!IMPORTANT]
> Do not forget to set your Wi-Fi credentials in the `main.hpp` to connect correctly.

> [!IMPORTANT]
> In case a local echo server is running, the firewall TCP port used in this case MUST be allowed on the hosting machine.

_Cyril FENARD._

# Key words
CRTP, DMA, EMW, FreeRTOS, LwIP, RAII, SPI, STM32, TCP, UDP, Wi-Fi

## Topics

**Connectivity**

## Software

**C++** **STM32CubeIDE** **Cmake** **Ninja**
**Windows** **Cygwin** **Linux** **VirtualBox**


# Check development environment

## Tools for Windows and Cygwin as shell only

```shell
    $ uname -smov
    CYGWIN_NT-10.0-19045 2025-01-29 19:46 UTC x86_64 Cygwin

    $ export PATH=/cygdrive/c/Program\ Files/CMake/bin/:$PATH

    $ export ARM_GCC_PATH=c:/Program\ Files\ \(x86\)/Arm\ GNU\ Toolchain\ arm-none-eabi/14.2\ rel1/bin

    $ export PATH=/cygdrive/c/NINJA/:$PATH

    $ cat /cygdrive/c/ST/STM32CubeIDE_1_16_1/STM32CubeIDE/.eclipseproduct | grep version
    version=1.16.1

    $ /cygdrive/c/Program\ Files/Cppcheck/cppcheck --version
    Cppcheck 2.16.0

    $ uname -srvimo
    Linux 6.15.10-100.fc41.x86_64 #1 SMP PREEMPT_DYNAMIC Fri Aug 15 14:55:12 UTC 2025 x86_64 unknown GNU/Linux

    $ cat /opt/st/stm32cubeide_1.18.0/.eclipseproduct | grep version
    version=1.18.0

    $ cppcheck --version
    Cppcheck 2.17.1

    # cd /usr/lib64/ccache && ls -lG1 arm-none*
    lrwxrwxrwx. 1 root 16 13 avril 19:56 arm-none-eabi-g++ -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 12 avril 11:23 arm-none-eabi-gcc -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 14 avril 22:12 arm-none-eabi-gcc-ar -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 14 avril 22:13 arm-none-eabi-gcc-ranlib -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 15 avril 21:02 arm-none-eabi-objcopy -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 15 avril 20:53 arm-none-eabi-size -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 15 avril 20:53 arm-none-eabi-size -> ../../bin/ccache

    $ arm-none-eabi-gcc --version
    arm-none-eabi-gcc (Fedora 15.1.0-1.fc41) 15.1.0

    $ cmake --version
    cmake version 3.30.5

    $ ninja --version
    1.12.1

    $ iperf -v
    iperf version 2.2.1 (4 Nov 2024) pthreads
```

# Some technical reminders

  * fast quality checks
```shell
    $ cppcheck --force --enable=all --std=c++11 \
    -DCOMPILATION_WITH_SPI -DCOMPILATION_WITH_FREERTOS -DCOMPILATION_WITH_EMW \
    -DEMW_API_DEBUG -DEMW_IPC_DEBUG -DEMW_HCI_DEBUG -DEMW_IO_DEBUG -UEMW_OS_DEBUG_LOG \
    *.cpp
```
```shell
    $ cd cmake_emw_spi_no_os && cmake -Bbuild -G "Unix Makefiles" && cd build && scan-build make
```

  * building the applications
```shell
    $ cd cmake_emw_spi_no_os && cmake -Bbuild -G "Ninja" && cd build && cmake --build .
    $ cd cmake_emw_spi_freertos && cmake -Bbuild -G "Ninja" && cd build && cmake --build .
    $ cd cmake_emw_spi_lwip_freertos && cmake -Bbuild -G "Ninja" && cd build && cmake --build .
```

  * running the applications
```shell
   $ cd cmake_emw_spi_no_os/build && cp emw_spi_no_os.bin /var/run/media/fenard1/DIS_U585AI/
   $ cd cmake_emw_spi_freertos/build && cp emw_spi_freertos.bin /var/run/media/fenard1/DIS_U585AI/
   $ cd cmake_emw_spi_lwip_freertos/build && cp emw_spi_lwip_freertos.bin /var/run/media/fenard1/DIS_U585AI/
```

```shell
Welcome

[   135] main(): Aug 23 2025 20:38:05  ( __GNUC__)   : 131 ms for 1600000 loops

std::rand() seeded by 1132435944 returned 45644357

BUFSIZ 1024
---------------------------------------------------------
CPUID: 410FD214, DEVID: 482, REVID: 2001
Cortex M33 r0p4
---------------------------------------------------------
---------------------------------------------------------
CheckExceptionHandling(): **Checking C++ exception**
---------------------------------------------------------

main(): sp : 0x200bff68
main(): end: 0x20044a60
main(): clk:  160000000
main(): StackType_t with: 4

[187] StartRootTask(): STM32_THREAD_SAFE_STRATEGY (4) (4 - 1)
SPI+DMA, FreeRTOS(V11.2.0), Network on STM32
SPI+DMA, FreeRTOS(V11.2.0), Network on STM32
LWIP_IPV4, LWIP_IPV6
NETWORK_BUFFER_SIZE:   1542
MEM_SIZE           :  61440
PBUF_POOL_BUFSIZE  :   1544

Wi-Fi network interface initialization (SOFTAP)
[  1546] Wi-Fi driver ready (SOFTAP):
          - Device Name    : EMW3080B.
          - Device ID      : MXCHIP-WIFI.
          - Device Version : V2.3.4.

Wi-Fi network interface initialization (STATION)
[  1576] Wi-Fi driver ready (STATION):
          - Device Name    : EMW3080B.
          - Device ID      : MXCHIP-WIFI.
          - Device Version : V2.3.4.

AppWiFiLwip::checkIoSpeed()> (130 x (1542 + 1542)) ...
transfered: 400920 bytes, time: 310 ms, Speed: 10346 Kbps

Start Software enabled Access Point with "MyHotSpot"
[  2263] Wi-Fi interface ready (SOFTAP):
          - name        : "MA".
          - hostname    : "lwip-softap".
          - mtu         : 1500.
          - MAC         : 84.9D.C2.96.C8.E1

[  2280] Network interface ready (SOFTAP):
          - IP address      : 10.10.10.1
          - Netmask         : 255.255.255.0
          - GW address      : 10.10.10.1

Wi-Fi scan
######### Scan 10 BSS ##########
######### End of Scan ##########

Wi-Fi connection

AppWiFiLwip::connectAp()> joining "XXXXXXX" with "ZZZZZZZZZZ" ...
ccccccccccccccccccccccccccccc
[  6802] Wi-Fi interface ready (STATION):
          - name        : "MS".
          - hostname    : "lwip-sta".
          - mtu         : 1500.
          - MAC         : 84.9D.C2.96.C8.E0
[  6819] Setting IPv6 link-local address
[  6823] Calling dhcp_start()
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
[ 13426] Network Interface connected (STATION):
          - IP address      : 192.168.1.113
          - Netmask         : 255.255.255.0
          - GW address      : 192.168.1.254
          - IP6 address (0) : FE80::AAAA:BBBB:FE96:C8E0 [48]
          - IP6 address (1) : 2001:CCC:DDDD:EEEE:FFFF:GGGG:FE96:C8E0 [48]
          - IP6 address (2) : :: [0]
          - DNS_0 address   : 2001:HHH:IIII:JJJJ:KKKK:LLLL:MMMM:NNNN
          - DNS_1 address   : 0.0.0.0
          - DNS_2 address   : 2001:4860:4860::8888

##### Please enter one of the following command:

echo        echo [-cCount] [-6] <ip>
iperf       iperf [-s | -c <host>]
ping        ping [-6] <hostname> (default is google.fr)
scan        Wi-Fi scan
stats       Get LwIP statistics
app> iperf -s
iperf: Started a TCP server on the default TCP port (5001)
app>
LWIPERF_TCP_DONE_SERVER
local address    : 192.168.1.113
local port       : 5001
remote address   : 192.168.1.19
remote port      : 65403
bytes transferred: 8387172
duration         : 10452 ms
bandwidth        : 6416 kBits/s

```
