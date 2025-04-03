# B-U585I-IOT2A_EMW
Wi-Fi examples and driver written with C/C++ languages

# Abstract

This project aims at demonstrating some Wi-Fi examples and driver written with C/C++ languages.

> [!IMPORTANT]
> Do not forget to set your Wi-Fi credentials in the `emw_conf.hpp` to connect correctly.

> [!IMPORTANT]
> In case a local echo server is running, the firewall TCP port used in this case MUST be allowed on the hosting machine.

_Cyril FENARD._

# Key words
DMA, EMW, FreeRTOS, LwIP, SPI, STM32, Wi-Fi

## Topics

**Connectivity**

## Software

**C++** **STM32CubeIDE** **Cmake**
**Windows** **Cygwin** **Linux** **VirtualBox**


# Check development environment
```shell
    $ uname -smov
    CYGWIN_NT-10.0-19045 2025-01-29 19:46 UTC x86_64 Cygwin

    $ cat /cygdrive/c/ST/STM32CubeIDE_1_16_1/STM32CubeIDE/.eclipseproduct | grep version
    version=1.16.1

    $ /cygdrive/c/Program\ Files/Cppcheck/cppcheck --version
    Cppcheck 2.16.0

    $ uname -srvimo
    Linux 6.11.9-100.fc39.x86_64 #1 SMP PREEMPT_DYNAMIC Sun Nov 17 18:52:19 UTC 2024 x86_64 unknown GNU/Linux

    $ cat /opt/st/stm32cubeide_1.18.0/.eclipseproduct | grep version
    version=1.18.0

    $ cppcheck --version
    Cppcheck 2.14.2

    # cd /usr/lib64/ccache && ls -lG1 arm-none*
    lrwxrwxrwx. 1 root 16 13 avril 19:56 arm-none-eabi-g++ -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 12 avril 11:23 arm-none-eabi-gcc -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 14 avril 22:12 arm-none-eabi-gcc-ar -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 14 avril 22:13 arm-none-eabi-gcc-ranlib -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 15 avril 21:02 arm-none-eabi-objcopy -> ../../bin/ccache
    lrwxrwxrwx. 1 root 16 15 avril 20:53 arm-none-eabi-size -> ../../bin/ccache

    $ arm-none-eabi-gcc --version
    arm-none-eabi-gcc (Fedora 14.1.0-1.fc39) 14.1.0

    $ cmake --version
    cmake version 3.30.5

    $ ninja --version
    1.11.1

    $ iperf -v
    iperf version 2.2.0 (10 April 2024) pthreads
```

# Some technical reminders

  * fast quality checks
```shell
    $ cppcheck --force --enable=all  --std=c++11 \
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

[133] main(): Apr 21 2025 12:33:41  ( __GNUC__)   : 130 ms for 1600000 loops

rand() seeded by -1073904781 returned 967579691

---------------------------------------------------------
CPUID : 410FD214 , DEVID : 482 , REVID : 2001
Cortex M33 r0p4
---------------------------------------------------------
---------------------------------------------------------
CheckExceptionHandling(): **Checking C++ exception**
---------------------------------------------------------

main(): sp : 0x200bff78
main(): end: 0x2002da50
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
WiFiNetwork::netifExtCallbackFunction() -> (0001) LWIP_NSC_NETIF_ADDED
[  1553] Wi-Fi driver ready:
          - Device Name    : EMW3080B.
          - Device ID      : MXCHIP-WIFI.
          - Device Version : V2.3.4.

Wi-Fi network interface initialization (STATION)
WiFiNetwork::netifExtCallbackFunction() -> (0001) LWIP_NSC_NETIF_ADDED
[  1588] Wi-Fi driver ready:
          - Device Name    : EMW3080B.
          - Device ID      : MXCHIP-WIFI.
          - Device Version : V2.3.4.

AppWiFiLwip::checkIoSpeed()> (130 x (1542 + 1542)) ...
transfered: 400920 bytes, time: 307 ms, Speed: 10447 Kbps

Start Software enabled Access Point with "MyHotSpot"
WiFiNetwork::informOfDriverStatus() -> EmwApiBase::eWIFI_EVENT_AP_UP
[  2271] Wi-Fi interface ready:
          - name        : "MA".
          - hostname    : "lwip-softap".
          - mtu         : 1500.
SoftAP MAC address 84.9D.C2.96.C8.E1
WiFiNetwork::netifExtCallbackFunction() -> (0004) LWIP_NSC_LINK_CHANGED
WiFiNetwork::netifExtCallbackFunction() -> (0008) LWIP_NSC_STATUS_CHANGED
WiFiNetwork::netifExtCallbackFunction() -> (04f0) LWIP_NSC_IPV4_ADDR_VALID | LWIP_NSC_IPV4_SETTINGS_CHANGED
| LWIP_NSC_IPV4_NETMASK_CHANGED | LWIP_NSC_IPV4_GATEWAY_CHANGED | LWIP_NSC_IPV4_ADDRESS_CHANGED

[  2318] Network Interface connected:
          - IP address      : 10.10.10.1
          - Netmask         : 255.255.255.0
          - GW address      : 10.10.10.1

Wi-Fi scan
######### Scan 10 BSS ##########
######### End of Scan ##########

Wi-Fi connection

AppWiFiLwip::joinAccessPoint()> joining "XXXXXXX" with "ZZZZZZZZZZ" ...
WiFiNetwork::informOfDriverStatus() -> EmwApiBase::eWIFI_EVENT_STA_UP
[  7711] Wi-Fi interface ready:
          - name        : "MS".
          - hostname    : "lwip-sta".
          - mtu         : 1500.
          - MAC         : 84.9D.C2.96.C8.E0
WiFiNetwork::netifExtCallbackFunction() -> (0004) LWIP_NSC_LINK_CHANGED
WiFiNetwork::netifExtCallbackFunction() -> (0008) LWIP_NSC_STATUS_CHANGED
[  7741] Setting IPv6 link-local address
WiFiNetwork::netifExtCallbackFunction() -> (0200) LWIP_NSC_IPV6_ADDR_STATE_CHANGED
[  7752] Calling dhcp_start()
WiFiNetwork::netifExtCallbackFunction() -> (0200) LWIP_NSC_IPV6_ADDR_STATE_CHANGED
WiFiNetwork::netifExtCallbackFunction() -> (0200) LWIP_NSC_IPV6_ADDR_STATE_CHANGED
WiFiNetwork::netifExtCallbackFunction() -> (0200) LWIP_NSC_IPV6_ADDR_STATE_CHANGED
WiFiNetwork::netifExtCallbackFunction() -> (0200) LWIP_NSC_IPV6_ADDR_STATE_CHANGED
WiFiNetwork::netifExtCallbackFunction() -> (0200) LWIP_NSC_IPV6_ADDR_STATE_CHANGED
WiFiNetwork::netifExtCallbackFunction() -> (04f0) LWIP_NSC_IPV4_ADDR_VALID | LWIP_NSC_IPV4_SETTINGS_CHANGED
| LWIP_NSC_IPV4_NETMASK_CHANGED | LWIP_NSC_IPV4_GATEWAY_CHANGED | LWIP_NSC_IPV4_ADDRESS_CHANGED

[ 14843] Network Interface connected:
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

echo        echo [-cCount] [-6] <host>
iperf       iperf [-s | -c <host>]
ping        ping [-6] <host> (default is google.fr)
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
