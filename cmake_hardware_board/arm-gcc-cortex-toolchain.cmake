set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(TARGET_TRIPLET "arm-none-eabi-")

## Avoids running the linker and is intended for use with cross-compiling toolchains
## that cannot link without custom flags or linker scripts.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build, options are: Debug Release." FORCE)
endif()

if (WIN32)
  set(TOOLCHAIN_EXT ".exe")
else()
  set(TOOLCHAIN_EXT "")
endif (WIN32)

find_program(COMPILER_ON_PATH "${TARGET_TRIPLET}gcc${TOOLCHAIN_EXT}")

if (DEFINED ENV{ARM_GCC_PATH})
  file(TO_CMAKE_PATH $ENV{ARM_GCC_PATH} ARM_TOOLCHAIN_PATH)
  message(STATUS "Using ENV variable ARM_GCC_PATH = ${ARM_TOOLCHAIN_PATH}")
elseif (COMPILER_ON_PATH)
  get_filename_component(ARM_TOOLCHAIN_PATH ${COMPILER_ON_PATH} DIRECTORY)
  message(STATUS "Using ARM GCC from path = ${ARM_TOOLCHAIN_PATH}")
else ()
  message(FATAL_ERROR "Unable to find ARM GCC. Either add to your PATH, or define ARM_GCC_PATH to the compiler location")
endif ()

set(CMAKE_C_COMPILER ${ARM_TOOLCHAIN_PATH}/${TARGET_TRIPLET}gcc${TOOLCHAIN_EXT})
set(CMAKE_CXX_COMPILER ${ARM_TOOLCHAIN_PATH}/${TARGET_TRIPLET}g++${TOOLCHAIN_EXT})
set(CMAKE_ASM_COMPILER ${ARM_TOOLCHAIN_PATH}/${TARGET_TRIPLET}gcc${TOOLCHAIN_EXT})
set(CMAKE_LINKER ${ARM_TOOLCHAIN_PATH}/${TARGET_TRIPLET}gcc${TOOLCHAIN_EXT})
set(CMAKE_SIZE_UTIL ${ARM_TOOLCHAIN_PATH}/${TARGET_TRIPLET}size${TOOLCHAIN_EXT})
set(CMAKE_OBJCOPY ${ARM_TOOLCHAIN_PATH}/${TARGET_TRIPLET}objcopy${TOOLCHAIN_EXT})
set(CMAKE_OBJDUMP ${ARM_TOOLCHAIN_PATH}/${TARGET_TRIPLET}objdump${TOOLCHAIN_EXT})
set(CMAKE_NM_UTIL ${ARM_TOOLCHAIN_PATH}/${TARGET_TRIPLET}gcc-nm${TOOLCHAIN_EXT})
set(CMAKE_AR ${ARM_TOOLCHAIN_PATH}/${TARGET_TRIPLET}gcc-ar${TOOLCHAIN_EXT})
set(CMAKE_RANLIB ${ARM_TOOLCHAIN_PATH}/${TARGET_TRIPLET}gcc-ranlib${TOOLCHAIN_EXT})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Compiler and linker flags
set(CMAKE_COMMON_FLAGS "-ffunction-sections -fdata-sections -Wall")
set(CMAKE_C_FLAGS "${MCPU_FLAGS} ${VFP_FLAGS} ${SPEC_FLAGS} ${CMAKE_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${MCPU_FLAGS} ${VFP_FLAGS} ${SPEC_FLAGS} ${CMAKE_COMMON_FLAGS} -Wconversion -Wold-style-cast -Woverloaded-virtual")
set(CMAKE_ASM_FLAGS "${MCPU_FLAGS} ${VFP_FLAGS} ${SPEC_FLAGS} ${CMAKE_COMMON_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS "${MCPU_FLAGS} ${VFP_FLAGS} ${LD_FLAGS} -Wl,--gc-sections -static -u _printf_float -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_ASM_FLAGS_DEBUG "-g")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")

set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_ASM_FLAGS_RELEASE "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")

