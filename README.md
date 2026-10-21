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
| `ui/BottomStrip.h` | `ga::BottomStrip`: Bypass, Dry/Wet, Bass Mono \| oversampling, filter, window size \| correlation, Phase Protect, goniometer button. Asks the plug-in through a small `Host` struct of callbacks |
| `ui/EditorShell.h` | `ga::EditorShell`: the editor itself. Faceplate, rack screws, bottom strip, goniometer side panel, scaling (75–200 %, corner drag), scale and goniometer saved with the session. The plug-in derives from it and hands over its panel and size |
| `plugin/SharedIds.h` | `ga::pid` and `ga::prop`: the parameter and property names the strip and shell use (`mix`, `bassmono`, `bypass`, `os`, `osfilter`, `phaseguard`, `scopeOpen`, `uiScale`); never change them |

```cpp
class MyEditor : public ga::EditorShell
{
public:
    MyEditor (MyProcessor& p) : ga::EditorShell (p, p.apvts, stripHost (p)), panel (p) { setPanel (panel, 802, 192); }
private:
    MyPanel panel;   // transparent: title and the plug-in's own sections
};
```

The window size stays in each plug-in.

Coming next: presets, DSP blocks, the output stage and a processor base.
