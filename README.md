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

## Interface (ui/)

Compiled into every plug-in by `ga_add_plugin()`; include as `#include "ui/Controls.h"`.

| File | |
| --- | --- |
| `ui/Style.h` | `ga::ui`: colours, fonts, drawing (recess, inset panel, knob faces, LED, padlock, screw), `drawTitle()`, `drawLogo()`; `ga::LookAndFeel` for menus, tooltips and dialogs |
| `ui/Controls.h` | `ga::Knob` (regular and mini, lockable), `StepBox`, `LampButton`, `SquareToggle`, `SectionLed`, `LedToggle`, `MenuBox` |
| `ui/Meters.h` | `ga::CorrelationMeter`, `ga::Goniometer` |

The window size stays in each plug-in.

Coming next: bottom strip and editor shell, presets, DSP blocks, the output stage and a processor base.
