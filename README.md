# B-U585I-IOT2A_EMW
Wi-Fi examples and driver written with C/C++ languages

# Abstract

This project aims at demonstrating some Wi-Fi examples and driver written with C/C++ languages.

Do not forget to set your Wi-Fi credentials in the `emw_conf.hpp` to connect correctly.

_Cyril FENARD._

# Key words
DMA, EMW, FreeRTOS, LwIP, SPI, STM32, Wi-Fi

## Topics

**Connectivity**

## Software

**C++** **STM32CubeIDE**
**Windows** **Cygwin** **Linux**


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
```

# Some technical reminders

  * fast quality check
```shell
    $ cppcheck --force --enable=all  --std=c++11\
      -DCOMPILATION_WITH_SPI -DCOMPILATION_WITH_FREERTOS -DCOMPILATION_WITH_EMW \
      -DEMW_API_DEBUG -DEMW_IPC_DEBUG -DEMW_HCI_DEBUG -DEMW_IO_DEBUG -UEMW_OS_DEBUG_LOG\
      *.cpp
```

  * building the application

    $

  * running the applications

```shell
SPI+DMA, FreeRTOS(V11.2.0), Network on STM32
NETWORK_BUFFER_SIZE:   1542
MEM_SIZE           :  61440
PBUF_POOL_BUFSIZE  :   1544


Wi-Fi network interface initialization (SOFTAP)

Wi-Fi network interface initialization (STATION)

transfered: 400920 bytes, time: 297 ms, Speed: 10799 Kbps

Start Software enabled Access Point with "MyHotSpot"

Wi-Fi scan
######### Scan 10 BSS ##########

######### End of Scan ##########

Wi-Fi connection

[ 13424] Network Interface connected:
          - IP address      : 192.168.1.113
          - GW address      : 192.168.1.254

          - DNS_0 address   : 192.168.1.254
          - DNS_1 address   : 0.0.0.0
          - DNS_2 address   : 1.1.1.1

##### Please enter one of the following command:

echo        echo [-cCount] <host>
iperf       iperf [-s | -c <host>]
ping        ping <host> (default is google.fr)
scan        Wi-Fi scan
stats       Get LwIP statistics
app>
app> iperf -s
iperf: Started a TCP server on the default TCP port (5001)
app>
LWIPERF_TCP_DONE_SERVER
local address    : 192.168.1.113
local port       : 5001
remote address   : 192.168.1.139
remote port      : 65403
bytes transferred: 8387172
duration         : 10452 ms
bandwidth        : 6416 kBits/s

```
