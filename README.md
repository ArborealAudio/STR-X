# STR-X

## About

A custom digital guitar amp, made in part to teach myself how to program plugins--but I'll also be updating it every now and then as new feature ideas come to mind.

## Installing

Pre-built binaries can be downloaded from the Releases page

## Building

Building from source requires CMake (post v1.2)

```
git clone https://github.com/ArborealAudio/STR-X
cd STR-X
cmake -Bbuild
cmake --build build --config Release --target <TARGET>
```