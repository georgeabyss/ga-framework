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

`ga_add_plugin()` sets what every GA plug-in shares: VST3 and Audio Unit, link-time optimisation and
dead-code stripping. Company and manufacturer code default to GA and `Gaau` (`COMPANY`, `MANUFACTURER_CODE`). `ga_add_tool()` builds a command-line tool
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
| `ui/Theme.h` | `ga::Theme`: every colour, text style and size the framework draws with; the defaults are the GA look. `Theme::withAccent()` derives the highlight and dark shades from one colour; override any of them afterwards. `ga::themeOf (component)` finds the editor's theme |
| `ui/Looks.h` | The swappable looks, each an interface with the GA look built in: `TextEffect` (title; `PlainText`, `EchoSmear` with its echoes as settings), `FaceplateStyle` (`GradientFaceplate`), `PresetDisplayStyle` (`LedMatrixDisplay`), `KnobFace` (`GaKnobFace`). A plug-in sets them in `Theme::looks`, or writes its own class; a new look never touches the others |
| `ui/Style.h` | `ga::ui`: drawing in a theme's colours (recess, inset panel, knob faces, LED, padlock, screw), `drawTitle()`, `drawLogo()`; `ga::LookAndFeel` holds the theme and draws menus, tooltips and dialogs in it |
| `ui/Controls.h` | `ga::Knob` (regular and mini, lockable), `StepBox`, `LampButton`, `SquareToggle`, `SectionLed`, `LedToggle`, `SoloButton`, `DiceButton`, `MenuBox`; each has its native size from the start. `anchorOf()`: the point a control is lined up by (a knob's dial) |
| `ui/Modulation.h` | `ga::ModSource` (the grip: drag onto a knob that takes modulation; the editor shell runs the drag and outlines the knobs), `ga::ModDonut` (depth ring: drag up/down, double-click to remove, hover to highlight its knob). `Knob::setModulation()` draws the arc from the value to value + depth; `Knob::onModDrop` makes a knob a drop target |
| `ui/Section.h` | `ga::Section`: header line (title left, the plug-in's controls right-aligned, rule underneath) and body (one control per column, centred on the section's centre line, a knob by its dial; names for controls that do not draw their own). `ga::ColumnGrid`: N equal columns for a row of sections, a gap between sections, each section spanning some columns |
| `ui/Meters.h` | `ga::CorrelationMeter`, `ga::Goniometer` |
| `ui/BottomStrip.h` | `ga::BottomStrip`: Bypass, Dry/Wet, Bass Mono \| oversampling, filter, window size \| correlation, Phase Protect, goniometer button. Asks the plug-in through a small `Host` struct of callbacks. Native size 750 × 28, scaled as one piece: `setArea()` fills the area as far as its proportions allow, centred |
| `ui/Header.h` | `ga::Header`: the title row. Title and subtitle on the row's left edge, the gothic GA on its right edge, the preset bar centred between them, with an optional nudge (`setPresetBar (height, offsetX)`) |
| `ui/EditorShell.h` | `ga::EditorShell`: the editor itself. Faceplate, rack screws, bottom strip, goniometer side panel, scaling (75–200 %, corner drag), scale and goniometer saved with the session. The plug-in derives from it and hands over its panel (a `ga::Panel`), its size and its margin |

```cpp
class MyEditor : public ga::EditorShell
{
public:
    MyEditor (MyProcessor& p) : ga::EditorShell (p, p.apvts, stripHost (p), myTheme()), panel (p) { setPanel (panel, 802, 192, 26, 8); }
private:
    MyPanel panel;   // a ga::Panel, transparent: the header, the plug-in's own sections and the bottom strip
};
```

Framework components never know the window size or a plug-in's margins: they fill or centre
themselves in the area they are given. Each plug-in defines its layout once, as a few named values,
and places everything in `ga::Panel::layout()`:

```cpp
void MyPanel::layout (juce::Rectangle<int> content, ga::BottomStrip& strip)   // the window minus the margin
{
    header.setArea (content.removeFromTop (kHeaderHeight));
    strip.setArea (content.removeFromBottom (ga::BottomStrip::heightForWidth (content.getWidth())));
}
```

## Presets (presets/)

| File | |
| --- | --- |
| `presets/PresetManager.h` | `ga::PresetManager`: factory and user presets as one list, save / rename / delete, locks (a locked value stays when a preset loads), the modified flag. Values missing from a preset are the defaults |
| `presets/PresetBar.h` | `ga::PresetBar`: the dot-matrix preset browser with previous / next, the list, Save / Rename / Delete and the safety-mute warning. Native size 238 × 24, scaled to fit its bounds, centred |

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
| `plugin/ProcessorConfig.h` | `ga::ProcessorConfig`: the features a plug-in uses (oversampling, Bass Mono, Phase Protect, loudness match, scope, Dry/Wet locks, MIDI input) and the limits (safety level, oversampling factors, window sizes, corner-drag scale). `defaultScale` is the size of 100 % (screen pixels per layout pixel); the size menu and the saved window size are relative to it. A feature that is off has no parameter and is not in the bottom strip. The defaults are Hyper! Hyper! Dimension; pass the same config to `ProcessorBase` and `addSharedParameters()` |
| `plugin/SharedParams.h` | `ga::addSharedParameters()`: Bass Mono, Dry/Wet, Bypass, Oversampling, Anti-Alias Filter, Phase Protect, appended after the plug-in's own parameters |
| `plugin/SharedIds.h` | `ga::pid` and `ga::prop`: the shared parameter IDs (`mix`, `bassmono`, `bypass`, `os`, `osfilter`, `phaseguard`) and state properties (`scopeOpen`, `uiScale`, `preset`, `presetUser`, `lockDryWet`, `lockBassMono`); never change them |

```cpp
class MyProcessor : public ga::ProcessorBase
{
public:
    MyProcessor() : ga::ProcessorBase (createParameterLayout(), config()) {}   // own parameters, then ga::addSharedParameters (p, config())
private:
    void prepareWet (double sampleRate, int channels, int oversampling) override;
    void updateWetParams() override;
    void processWet (float* const* io, int numSamples) override;      // at the oversampled rate, in place
    void resetWet() override;
};
```

Everything from Hyper! Hyper! Dimension that is not its own sound or its own sections now lives here.

## Control gallery (tools/, docs/controls/)

Every framework control in its states, in the GA theme, at 2x: one image per control in
[docs/controls](docs/controls). A plug-in builds the gallery with `ga_add_gallery(<plugin>)`
(`-DGA_GALLERY=ON`); `<plugin>Gallery check framework/docs/controls` fails if a framework change
alters any pixel of them.

| Sheet | |
| --- | --- |
| [section](docs/controls/section.png) | `ga::Section` with header controls (dropdown, dice, LED) and a body on four columns; the same section off; `ga::DiceButton` |
| [stages](docs/controls/stages.png) | `ga::StepBox` as a Stages box (`setWideNumber`): the size of a drop-down, arrows double / halve, drag for single steps |
| [solo](docs/controls/solo.png) | `ga::SoloButton`: off, hovered, on (alarm orange) |
| [modulation](docs/controls/modulation.png) | `Knob` with a modulation arc (positive, negative), as a drop candidate and as the drop target; `ga::ModSource`; `ga::ModDonut` at +60, -40, +100 and 0 % |
