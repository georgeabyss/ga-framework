#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ga
{
struct Theme;

// The parts of a GA faceplate whose look can be swapped as a whole: each is an interface with
// today's GA look built in. A plug-in sets the ones it wants in its theme (Theme::looks), or writes
// its own class. Everything else is drawn from the theme's colours, type and sizes.

// How the title's letters are painted. text is the title as a path; bounds is its line: the left
// edge, the top of the capitals and a little below the baseline.
class TextEffect
{
public:
    virtual ~TextEffect() = default;
    virtual void paint (juce::Graphics&, const juce::Path& text, juce::Rectangle<float> bounds, const Theme&) const = 0;
};

// the letters with a soft shadow, filled with a vertical gradient: accentLight, accent, accentDeep
class PlainText : public TextEffect
{
public:
    void paint (juce::Graphics&, const juce::Path&, juce::Rectangle<float>, const Theme&) const override;
};

// PlainText over faint copies of itself, shifted right: a static unison echo
class EchoSmear : public TextEffect
{
public:
    struct Echo
    {
        float dx, dy;      // offset in px
        float opacity;     // of accentHi
    };
    std::vector<Echo> echoes { { 7.0f, 0.0f, 0.07f }, { 3.5f, 0.0f, 0.14f } };   // painted in this order

    void paint (juce::Graphics&, const juce::Path&, juce::Rectangle<float>, const Theme&) const override;
};

// The faceplate under everything, as large as the window.
class FaceplateStyle
{
public:
    virtual ~FaceplateStyle() = default;
    virtual void paint (juce::Graphics&, juce::Rectangle<float> bounds, const Theme&) const = 0;
};

// vertical gradient (faceplateTop to faceplateBottom), a lit top edge and a dark bottom edge
class GradientFaceplate : public FaceplateStyle
{
public:
    void paint (juce::Graphics&, juce::Rectangle<float>, const Theme&) const override;
};

// The display in the middle of the preset bar.
class PresetDisplayStyle
{
public:
    struct State
    {
        juce::String text;       // e.g. "07 TWIN PEAKS", already cut to maxChars()
        bool modified = false;   // the settings differ from the preset as saved
        bool muted = false;      // safety mute: show the warning instead
        bool blinkOn = true;     // the warning blinks
        bool menuOpen = false;   // the preset list is open
    };
    virtual ~PresetDisplayStyle() = default;
    virtual int maxChars() const = 0;
    virtual void paint (juce::Graphics&, juce::Rectangle<float> glass, const State&, const Theme&) const = 0;
};

// a grid of square LEDs behind dark glass; letters are 5 x 7 lit dots, the star marks a change
class LedMatrixDisplay : public PresetDisplayStyle
{
public:
    int maxChars() const override { return kSlots - 1; }
    void paint (juce::Graphics&, juce::Rectangle<float>, const State&, const Theme&) const override;

private:
    static constexpr float kPitch = 1.8f, kDot = 1.3f;
    static constexpr int kCols = 109, kRows = 12, kTextRow = 2, kTextCol = 3, kSlots = 17;
};

// The face of a knob: regular (the sections) and mini (the bottom strip). pos is 0..1,
// hot = hovered or dragged.
class KnobFace
{
public:
    virtual ~KnobFace() = default;
    virtual void paint (juce::Graphics&, const Theme&, juce::Point<float> centre, float pos, bool hot) const = 0;
    virtual void paintMini (juce::Graphics&, const Theme&, juce::Point<float> centre, float pos, bool hot) const = 0;
    // the modulation arc just outside the dial, from position `from` to `to` (0..1, either way round)
    virtual void paintModulation (juce::Graphics&, const Theme&, juce::Point<float> centre, float from, float to) const = 0;
};

// grey ring around a darker core, accent pointer, three position dots; the mini one with a value arc
class GaKnobFace : public KnobFace
{
public:
    void paint (juce::Graphics&, const Theme&, juce::Point<float>, float, bool) const override;
    void paintMini (juce::Graphics&, const Theme&, juce::Point<float>, float, bool) const override;
    void paintModulation (juce::Graphics&, const Theme&, juce::Point<float>, float, float) const override;
};
}
