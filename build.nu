#!/usr/bin/env nu

let clang    = 'C:\Users\gboone\Programs\clang+llvm-23.1.4-x86_64-pc-windows-msvc\bin\clang++.exe'
let sysroot  = 'C:\Users\GBOONE\Programs\mingw64'
let proj_dir = 'C:\Users\GBOONE\Documents\Code\handmade'
let radbin   = 'C:\Users\gboone\Programs\raddbg\radbin.exe'

mkdir bin

let args = [
    "--target=x86_64-w64-mingw32"
    $"--sysroot=($sysroot)"
    "-g"
    "-gcodeview"
    "-O0"
    $"-fdebug-compilation-dir=($proj_dir)"
    $"($proj_dir)/main.cpp"
    "-o"
    "bin/handmade.exe"
    "-fuse-ld=lld"
    "-Wl,--pdb=bin/handmade.pdb"
    "-lgdi32"
]
run-external $clang ...$args

# Convert PDB → RDI so raddbg has its native format available alongside the PDB
run-external $radbin "bin/handmade.pdb" "--out:bin/handmade.rdi"

print "Build OK: bin/handmade.exe + bin/handmade.pdb + bin/handmade.rdi"
