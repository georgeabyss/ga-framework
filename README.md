# GA framework

Shared building blocks for GA audio plug-ins (JUCE), used as a git submodule at `framework/`.

Being extracted from [Hyper! Hyper! Dimension](https://github.com/georgeabyss/GA-hyper-hyper-dimension)
step by step, each step proven by a bit-identical null test and unchanged screenshots.

## Build (cmake/)

```cmake
cmake_minimum_required(VERSION 3.22)
include(framework/cmake/GaDefaults.cmake)   # universal, macOS 11; before project()
project(MyPlugin VERSION 1.0.0)
include(framework/cmake/GaPlugin.cmake)     # JUCE (../JUCE or -DJUCE_DIR=...) and the functions below

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

JUCE is pinned: every GA build uses the version in `cmake/juce-version.txt`, locally and in CI, and
configuring fails with any other. It is found next to the plug-in repo (`../JUCE`), otherwise at
`-DJUCE_DIR=...`:

```sh
git clone --depth 1 --branch "$(cat framework/cmake/juce-version.txt)" https://github.com/juce-framework/JUCE.git ../JUCE
```

## Checks (.github/workflows/)

Every framework commit builds Hyper! Hyper! Dimension with it (branch `framework-next`, or the
repository variable `HHD_REF`) and runs its null test and screenshot check, as strict as in its own
repo, plus a Windows build. A framework change that alters Hyper! Hyper! Dimension's sound or a single
pixel fails.

## Interface (ui/)

Compiled into every plug-in by `ga_add_plugin()`; include as `#include "ui/Controls.h"`.

| File | |
| --- | --- |
| `ui/Style.h` | `ga::ui`: colours, fonts, drawing (recess, inset panel, knob faces, LED, padlock, screw), `drawTitle()`, `drawLogo()`; `ga::LookAndFeel` for menus, tooltips and dialogs |
| `ui/Controls.h` | `ga::Knob` (regular and mini, lockable), `StepBox`, `LampButton`, `SquareToggle`, `SectionLed`, `LedToggle`, `MenuBox` |
| `ui/Meters.h` | `ga::CorrelationMeter`, `ga::Goniometer` |
| `ui/BottomStrip.h` | `ga::BottomStrip`: Bypass, Dry/Wet, Bass Mono \| oversampling, filter, window size \| correlation, Phase Protect, goniometer button. Asks the plug-in through a small `Host` struct of callbacks |
| `ui/EditorShell.h` | `ga::EditorShell`: the editor itself. Faceplate, rack screws, bottom strip, goniometer side panel, scaling (75–200 %, corner drag), scale and goniometer saved with the session. The plug-in derives from it and hands over its panel and size |

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

## Presets (presets/)

| File | |
| --- | --- |
| `presets/PresetManager.h` | `ga::PresetManager`: factory and user presets as one list, save / rename / delete, locks (a locked value stays when a preset loads), the modified flag. Values missing from a preset are the defaults |
| `presets/PresetBar.h` | `ga::PresetBar`: the dot-matrix preset browser with previous / next, the list, Save / Rename / Delete and the safety-mute warning |

Each plug-in describes its presets in a `ga::PresetManager::Config`: the folder, file extension and
file tag (never change these after a release), the parameters a preset stores, the lockable parameters,
the factory presets (the first one is the default) and an optional tag shown in the list.

## DSP (dsp/)

| File | |
| --- | --- |
| `dsp/DspBlocks.h` | `ga::dsp`, header only, no JUCE: `DelayLine` (4-point Hermite), `Biquad` (low / high pass, K-weighting stages), `Crossover` (Linkwitz-Riley 4th order), `Allpass1`, `Allpass2`, `Hilbert` (90° pair) |
| `dsp/OutputStage.h` | `ga::dsp::OutputStage`, no JUCE: the end of every GA effect, per sample from the dry input and the effect's output: dry/wet, Bass Mono, Phase Protect (correlation kept above +0.2; `protectActivity` for the LED), loudness match (K-weighted, ITU-R BS.1770) |

## Processor (plugin/)

| File | |
| --- | --- |
| `plugin/ProcessorBase.h` | `ga::ProcessorBase`, the `AudioProcessor` every GA effect derives from: stereo or mono, oversampling (Off / 2x / 4x / 8x, linear phase or low latency, latency reported), click-free bypass at the effect's loudness, safety mute, the scope buffer for the meters, state save / load. The plug-in implements `prepareWet`, `updateWetParams`, `processWet`, `resetWet`, and optionally `splitAtNoteOns` / `noteOn` (sample-accurate note-ons) and `migrateState` (old sessions) |
| `plugin/SharedParams.h` | `ga::addSharedParameters()`: Bass Mono, Dry/Wet, Bypass, Oversampling, Anti-Alias Filter, Phase Protect, appended after the plug-in's own parameters |
| `plugin/SharedIds.h` | `ga::pid` and `ga::prop`: the shared parameter IDs (`mix`, `bassmono`, `bypass`, `os`, `osfilter`, `phaseguard`) and state properties (`scopeOpen`, `uiScale`, `preset`, `presetUser`, `lockDryWet`, `lockBassMono`); never change them |

```cpp
class MyProcessor : public ga::ProcessorBase
{
public:
    MyProcessor() : ga::ProcessorBase (createParameterLayout()) {}   // own parameters, then ga::addSharedParameters()
private:
    void prepareWet (double sampleRate, int channels, int oversampling) override;
    void updateWetParams() override;
    void processWet (float* const* io, int numSamples) override;      // at the oversampled rate, in place
    void resetWet() override;
};
```

Everything from Hyper! Hyper! Dimension that is not its own sound or its own sections now lives here.
