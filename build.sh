#!/usr/bin/env bash
# Build hello.cpp with clang targeting MinGW, producing CodeView + PDB
# debug info that raddbg reads cleanly (so &test / locals resolve).
#
# Requires: LLVM at /c/Users/GBOONE/tools/LLVM and MinGW at /c/Users/GBOONE/mingw64
# Usage:    ./build.sh      (or: bash build.sh)
# Note:     close the target in raddbg first, or the linker can't overwrite hello.exe.

set -e

CLANG="/c/Users/GBOONE/tools/LLVM/bin/clang++.exe"
SYSROOT="C:/Users/GBOONE/mingw64"

"$CLANG" \
  --target=x86_64-w64-mingw32 \
  --sysroot="$SYSROOT" \
  -g -gcodeview -O0 \
  hello.cpp \
  -o hello.exe \
  -fuse-ld=lld \
  -Wl,--pdb=hello.pdb

echo "Build OK: hello.exe + hello.pdb"
