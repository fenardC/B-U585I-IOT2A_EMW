# B-U585I-IOT2A_EMW
Wi-Fi examples and driver written with C/C++ languages

# Abstract

This project aims at demonstrating some Wi-Fi examples and driver written with C/C++ languages.

> [!IMPORTANT]
> Do not forget to set your Wi-Fi credentials in the `wifi_emw.hpp` to connect correctly.

> [!IMPORTANT]
> In case a local echo server is running, the firewall TCP port used in this case MUST be allowed on the hosting machine.
> The same is also to be done for iperf tests.

_Cyril FENARD._

# Key words

CRTP, DMA, EMW, FreeRTOS, LwIP, RAII, SSE, SPI, STM32, TCP, UDP, Wi-Fi

## Topics

**Connectivity**

## Software

**C++**, **STM32CubeIDE**, **Cmake**, **Ninja**, **Visual Studio Code**

**Windows**, **Cygwin**, **Linux**, **VirtualBox**


# Check development environment

## Tools for Windows and Cygwin as shell only

```shell
    $ uname -smov
    CYGWIN_NT-10.0-26200 2026-03-02 20:13 UTC x86_64 Cygwin

    $ export PATH=/cygdrive/c/Program\ Files/CMake/bin/:"${PATH}"

    $ export ARM_GCC_PATH=c:/Program\ Files\ \(x86\)/Arm\ GNU\ Toolchain\ arm-none-eabi/14.3\ rel1/bin

    $ export PATH=/cygdrive/c/NINJA/:"${PATH}"

    $ cat /cygdrive/c/ST/STM32CubeIDE_1.19.0/STM32CubeIDE/.eclipseproduct | grep version
    version=1.19.0

    $ /cygdrive/c/Program\ Files/Cppcheck/cppcheck --version
    Cppcheck 2.20.0

    $ iperf -v
    iperf version 2.0.13 (21 Jan 2019) pthreads
```

## Tools with Linux

```shell
    $ uname -srvimo
    Linux 7.1.9-200.fc44.x86_64 #1 SMP PREEMPT_DYNAMIC Wed Aug 19 17:41:40 UTC 2026 x86_64 unknown GNU/Linux

    $ /usr/share/code/bin/code --help
    Visual Studio Code 1.134.0

    $ cat /opt/st/stm32cubeide_2.0.0/.eclipseproduct | grep version
    version=2.0.0

    $ grep GROUP /etc/udev/rules.d/*stlink*

    $ cppcheck --version
    Cppcheck 2.21.1

    # cd /usr/lib64/ccache && ls -lG1 arm-none*
    lrwxrwxrwx. 1 root 16 13 avril 19:56 arm-none-eabi-g++ -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 12 avril 11:23 arm-none-eabi-gcc -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 14 avril 22:12 arm-none-eabi-gcc-ar -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 14 avril 22:13 arm-none-eabi-gcc-ranlib -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 15 avril 21:02 arm-none-eabi-objcopy -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 15 avril 20:53 arm-none-eabi-size -> ../../bin/ccache

    $ arm-none-eabi-gcc --version
    arm-none-eabi-gcc (Fedora 15.2.0-4.fc44) 15.2.0

    $ cmake --version
    cmake version 3.31.11

    $ ninja --version
    1.13.2

    $ clang --version
    clang version 22.1.8 (Fedora 22.1.8-4.fc44)
    Target: x86_64-redhat-linux-gnu
    Thread model: posix
    InstalledDir: /usr/bin
    Configuration file: /etc/clang/x86_64-redhat-linux-gnu-clang.cfg

    $ which analyze-build
    /usr/bin/analyze-build

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
    $ cd cmake_emw_spi_no_os && cmake -Bbuild -G "Ninja" && cd build && \
    analyze-build --cdb compile_commands.json \
    --use-analyzer ../../cmake_hardware_board/fake_clang.sh --status-bugs \
    --analyzer-config aggressive-binary-operation-simplification=true \
    --analyzer-config aggressive=true -enable-checker alpha -enable-checker security \
    -enable-checker unix -enable-checker cplusplus -enable-checker deadcode -enable-checker nullability \
    -enable-checker core \
    --output ..
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

[   135] main(): Jun  7 2026 17:32:57  ( __GNUC__)   : 130 ms for 1600000 loops

 std::rand() seeded by -749746119 returned 1629264863

BUFSIZ 1024
 ---------------------------------------------------------
 CPUID: 410FD214, DEVID: 482, REVID: 2001
 Cortex M33 r0p4
 ---------------------------------------------------------
 ---------------------------------------------------------
 CheckExceptionHandling(): **Checking C++ exception**
 ---------------------------------------------------------

 main(): sp : 0x200bff68
 main(): end: 0x200449b8
 main(): clk:  160000000
 main(): StackType_t with: 4

 [191] FreeRtosMainTask(): STM32_THREAD_SAFE_STRATEGY (4) (4 - 1)
MEM_SIZE           :  61440
PBUF_POOL_BUFSIZE  :   1544

LWIP_IPV4, LWIP_IPV6
SPI+DMA, FreeRTOS(V11.2.0), Network on STM32
NETWORK_BUFFER_SIZE:   1542

[   217] InitializeEmw(): REBOOT(HW) ...
 - Device Name    : MXCHIP-WIFI.
 - Device ID      : EMW3080B.
 - Device Version : V2.3.4.
 - MAC address    : 84.9D.C2.96.C8.E0

[  1551] Checking Emw Io Speed (130 x (1542 + 1542)) ...
[  1866] ... transferred: 400920 bytes, time: 310 ms, Speed: 10346 Kbps

Wi-Fi network interface initialization (SOFTAP)
[  1883] Wi-Fi driver ready (SOFTAP):

Wi-Fi network interface initialization (STATION)
[  1891] Wi-Fi driver ready (STATION):

Start Software enabled Access Point with "MyHotSpot"

[  2246] Wi-Fi interface ready (SOFTAP):
          - name        : "MA".
          - hostname    : "lwip-softap".
          - mtu         : 1500.
          - MAC         : 84.9D.C2.96.C8.E1

[  2264] Network interface ready (SOFTAP):
          - IP address      : 10.10.10.1
          - Netmask         : 255.255.255.0
          - GW address      : 10.10.10.1
Starting the DHCP server ...

 Wi-Fi scan
 ######### Scan 10 BSS ##########
 ######### End of Scan ##########

SSE Web server started (SOFTAP)

 Wi-Fi connection

AppWiFiLwip::connectToAp()> joining "XXXXXXX" with "ZZZZZZZZZZ" ...
cccccccccccccccccccccccccccc
[  6688] Wi-Fi interface ready (STATION):
          - name        : "MS".
          - hostname    : "lwip-sta".
          - mtu         : 1500.
          - MAC         : 84.9D.C2.96.C8.E0
[  6706] Setting IPv6 link-local address
[  6709] Calling dhcp_start()
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
[ 13412] Network interface connected (STATION):
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

echo         echo [-cCount] [-6] <ip>
iperf        iperf [-s | -c <ip>]
ping         ping [-6] <hostname> (default is google.fr)
scan         Wi-Fi scan
stats        Get LwIP statistics
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
