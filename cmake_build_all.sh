#!/usr/bin/bash

## $ shellcheck -Calways -o all -s bash -S style cmake_build_all.sh

set -e
set -u
set -o pipefail
## set -x
## set -v

# --- Colors ---------------------------------------------------------------
declare -r RED=$'\e[31m'
declare -r GREEN=$'\e[32m'
declare -r YELLOW=$'\e[33m'
declare -r BLUE=$'\e[34m'
declare -r MAGENTA=$'\e[35m'
declare -r CYAN=$'\e[36m'
declare -r RESET=$'\e[0m'

declare -r this_script=$(basename "$0")
echo Starting "${this_script}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)" >&2
## echo "$SCRIPT_DIR"

declare -r HOST_PLATFORM=$(uname -s)
declare -r HOST_NB_CPUS=$(/usr/bin/nproc)

if [[ ${HOST_PLATFORM} == CYGWIN* ]]; then
  declare -r CMAKE_BUILD_CMAKE="/proc/cygdrive/c/Program Files/CMake/bin"
  declare -r CMAKE_BUILD_NINJA="/proc/cygdrive/c/NINJA"
  declare -r CMAKE_BUILD_ARM_GCC_PATH="C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.3 rel1/bin"
  declare -r IS_CYGWIN="true"
  declare -r IS_LINUX="false"

elif [[ ${HOST_PLATFORM} == Linux* ]]; then
  declare -r CMAKE_BUILD_CMAKE="/usr/bin"
  declare -r CMAKE_BUILD_NINJA="/usr/bin"
  declare -r CMAKE_BUILD_ARM_GCC_PATH="/usr/lib64/ccache"
  declare -r IS_CYGWIN="false"
  declare -r IS_LINUX="true"

else
  exit
fi

usage() {
  echo "Usage: ${this_script} [-v]
  Action
    -v: For verbose
  " >&2
}

exit_abnormal() {
  usage
  exit 1
}

ARG_VERBOSE="no"


while getopts v option ; do
    case "${option}" in
    v)
      ARG_VERBOSE="yes"
    ;;
    :)
      exit_abnormal
    ;;
    *)
      exit_abnormal
    ;;
    esac
done


verbose_me() {
  if [[ "${ARG_VERBOSE}" = "yes" ]]; then
    printf "${CYAN}> %s${RESET}\n" "$*" >&2
  fi
}

debug_me_in() {
  if [[ "${ARG_VERBOSE}" = "yes" ]]; then
    printf "%s> %s%s\n" "${MAGENTA}" "$*" "${RESET}" >&2
  fi
}

debug_me_out() {
  if [[ "${ARG_VERBOSE}" = "yes" ]]; then
    printf "%s< %s%s\n\n" "${MAGENTA}" "$*" "${RESET}" >&2
  fi
}

# --- PATH setup ---------------------------------------------------------------
export PATH="${CMAKE_BUILD_CMAKE}:${PATH}"
export PATH="${CMAKE_BUILD_NINJA}:${PATH}"
## echo -e "\${PATH}        :\n${PATH//:/\\n}"

export ARM_GCC_PATH="${CMAKE_BUILD_ARM_GCC_PATH}"

export CCC_ANALYZER_ARGS="--target=arm-none-eabi -mcpu=cortex-m33 -mthumb"

ANALYSE_BUILD_OPTIONS=(
  --use-analyzer "${SCRIPT_DIR}/cmake_hardware_board/fake_clang.sh"
  --status-bugs
  --analyzer-config aggressive-binary-operation-simplification=true
  -enable-checker alpha
  -enable-checker security
  -enable-checker unix
  -enable-checker cplusplus
  -enable-checker deadcode
  -enable-checker nullability
  -enable-checker core
)


build_and_analyze() {
  local -r path_in="$1"
  debug_me_in "build_and_analyze()" "${path_in}"

  printf "%s==> Building: %s%s\n" "${BLUE}" "${path_in}" "${RESET}"
  pushd "${path_in}" > /dev/null

  rm -rf build
  cmake -Bbuild -G Ninja
  cmake --build build 2>&1

  printf "%s==> Running static analysis...%s\n" "${YELLOW}" "${RESET}"

  if ! output=$(analyze-build --cdb build/compile_commands.json "${ANALYSE_BUILD_OPTIONS[@]}" --output . \
      | grep -v "Run 'scan-view"); then
    printf "%sStatic analysis reported issues in %s%s\n" "${RED}" "${path_in}" "${RESET}"
  fi

  popd > /dev/null

  debug_me_out "build_and_analyze()"
}

# --- Run all builds -----------------------------------------------------------

declare -ri DATE_START=$(date +%s)

build_and_analyze "${SCRIPT_DIR}/cmake_emw_spi_no_os"
build_and_analyze "${SCRIPT_DIR}/cmake_emw_spi_freertos"
build_and_analyze "${SCRIPT_DIR}/cmake_emw_spi_lwip_freertos"

declare -ri DATE_END=$(date +%s)
declare -ri runtime=$((DATE_END - DATE_START))

printf "\n%s Execution time: %s seconds (%s / %s)%s\n" "${GREEN}" "${runtime}" "${HOST_PLATFORM}" "${HOST_NB_CPUS}" "${RESET}"