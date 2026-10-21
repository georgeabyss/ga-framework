# GA framework

Shared building blocks for GA audio plug-ins (JUCE), used as a git submodule at `framework/`.

Being extracted from [Hyper! Hyper! Dimension](https://github.com/georgeabyss/GA-hyper-hyper-dimension)
step by step, each step proven by a bit-identical null test and unchanged screenshots.

## Build (cmake/)

```cmake
cmake_minimum_required(VERSION 3.22)
include(framework/cmake/GaDefaults.cmake)   # universal, macOS 11; before project()
project(MyPlugin VERSION 1.0.0)
include(framework/cmake/GaPlugin.cmake)     # JUCE (-DJUCE_DIR=...) and the functions below

ga_add_plugin(MyPlugin
    PRODUCT_NAME "My Plugin"
    PLUGIN_CODE Abc1                        # never change after a release
    BUNDLE_ID com.ga.myplugin
    SOURCES ${SOURCES})

ga_add_tool(MyPlugin MyPluginTests OPTION MYPLUGIN_TESTS SOURCES tools/Tests.cpp MAIN tools/TestsMain.cpp)
```

`ga_add_plugin()` sets what every GA plug-in shares: company GA, manufacturer code `Gaau`, VST3 and
Audio Unit, link-time optimisation and dead-code stripping. `ga_add_tool()` builds a command-line tool
that runs the plug-in's own code, only when its option is on.

Coming next: UI building blocks, presets, DSP blocks, the output stage and a processor base.
