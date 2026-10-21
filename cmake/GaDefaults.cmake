# GA plug-in defaults that must be set before project():
#   include(framework/cmake/GaDefaults.cmake)
#   project(MyPlugin VERSION 1.0.0)
#
# Universal build: one binary for Apple Silicon and Intel Macs, macOS 11 (Big Sur) or newer.
# (Before project(), so it also applies to the JUCE helper builds.)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")   # the oldest macOS the current C++ library supports
