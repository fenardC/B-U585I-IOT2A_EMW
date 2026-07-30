#!/usr/bin/bash

# Wrapper that removes ARM-only and unsupported flags so analyze-build can run.

cleaned_args=""

for arg in "$@"; do
  case "${arg}" in
    -mcpu=*|-mthumb|-mfloat-abi=*|-mfpu=*|--specs=*|-fmodules-ts|-fmodule-mapper=*|-fdeps-format=*)
      # Skip ARM and module flags
      ;;
    *)
      cleaned_args="${cleaned_args} \"${arg}\""
      ;;
  esac
done

# Call host clang with cleaned arguments
eval clang "${cleaned_args}"

