# STR-X

## About

A custom digital guitar amp, made in part to teach myself how to program plugins--but I'll also be updating it every now and then as new feature ideas come to mind.

## Installing

Pre-built binaries can be downloaded from the Releases page

## Building

Building from source requires CMake (post v1.2)

```bash
git clone --recurse-submodules https://github.com/ArborealAudio/STR-X
cd STR-X
git submodule update --init --recursive # just to make sure you get 'em all
cmake -Bbuild -DPRODUCTION_BUILD=1
cmake --build build --config Release --target <TARGET>
```
