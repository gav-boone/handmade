#!/usr/bin/env bash
# Build main.cpp with clang targeting MinGW, producing CodeView + PDB
# debug info that raddbg reads cleanly (so &test / locals resolve).
#
# Requires: clang + lld (sudo apt install -y clang lld) and
#           gcc-mingw-w64-x86-64 (for headers/runtime libs)
# Usage:    ./build_linux.sh
# Note:     close the target in raddbg first, or the linker can't overwrite handmade.exe.

set -e

# Quiet Mesa/libEGL + Wine noise from processes started via this script.
export WINEDEBUG=-all
export LIBGL_DEBUG=quiet
export EGL_LOG_LEVEL=error
export MESA_LOG_LEVEL=error

CLANG="clang++"
SYSROOT="/usr/x86_64-w64-mingw32"
GCC_INST="$(dirname "$(x86_64-w64-mingw32-gcc -print-file-name=crtbegin.o)")"
CPP_INC="$GCC_INST/include/c++"
CPP_TGT_INC="$CPP_INC/$(x86_64-w64-mingw32-gcc -dumpmachine)"

mkdir -p bin

"$CLANG" \
  --target=x86_64-w64-windows-gnu \
  --sysroot="$SYSROOT" \
  -isystem "$CPP_INC" \
  -isystem "$CPP_TGT_INC" \
  -g -gcodeview -O0 \
  -mwindows \
  -static-libgcc -static-libstdc++ \
  -fdebug-compilation-dir="$PWD" \
  main.cpp \
  -o bin/handmade.exe \
  -fuse-ld=lld \
  -L "$GCC_INST" \
  -Wl,--pdb=bin/handmade.pdb \
  -lgdi32 -luser32

echo "Build OK: bin/handmade.exe + bin/handmade.pdb"