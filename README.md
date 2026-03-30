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

**C++** **STM32CubeIDE** **Cmake** **Ninja** **Visual Studio Code**
**Windows** **Cygwin** **Linux** **VirtualBox**


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
    Cppcheck 2.16.0

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

  * building the applications
    $

  * running the applications
    $
