#pragma once

#include "Theme.h"

// The drawing primitives every part of a GA faceplate shares, in the colours, type and sizes of a
// ga::Theme.
namespace ga::ui
{
    // plain fonts, for a plug-in's own drawing; the framework draws with Theme::font()
    juce::Font font (float size, bool bold = false, float kerning = 0.0f);
    juce::Font monoFont (float size, bool bold = false);

    // point at radius r and angle (radians, clockwise from 12 o'clock) around c
    juce::Point<float> polar (juce::Point<float> c, float r, float angle);

    // recessed dark box (step boxes, toggles, meters)
    void drawRecess (juce::Graphics&, const Theme&, juce::Rectangle<float>);
    void drawRecess (juce::Graphics&, const Theme&, juce::Point<float> centre, float size);

    // darker inset panel with a faint accent edge (bottom strip, correlation box, goniometer)
    void drawInsetPanel (juce::Graphics&, const Theme&, juce::Rectangle<float>);

    void drawHoverWash (juce::Graphics&, const Theme&, juce::Rectangle<float>);
    void drawDropDownMark (juce::Graphics&, float x, float y, juce::Colour);

    // round LED; alarm 0..1 turns a lit LED from the accent to the alarm colour
    void drawLed (juce::Graphics&, const Theme&, juce::Point<float> centre, bool on, float alarm = 0.0f);

    // small padlock, 5 x 6, top left at (x, y); open = shackle lifted
    void drawPadlock (juce::Graphics&, float x, float y, bool open, juce::Colour);

    void drawScrew (juce::Graphics&, const Theme&, float cx, float cy);

    // Gothic "GA", drawn with a flat pen held at an angle (own letterforms, no font needed)
    juce::Path makeGothicGA (float scale, float nib, float angleDeg);

    // small caps name under a control; (cx, cy) is the centre of the control
    void drawName (juce::Graphics&, const Theme&, float cx, float cy, const juce::String& name);

    // plug-in title: big title in the theme's title effect, its text starting at x on the baseline,
    // small grey subtitle below
    void drawTitle (juce::Graphics&, const Theme&, const juce::String& title, const juce::String& subtitle, float x, float baseline);
    float titleWidth (const Theme&, const juce::String& title);   // of the title's letters, without the echo

    // the silver gothic GA, its right edge at x = right and its top at y = top
    void drawLogo (juce::Graphics&, const Theme&, float right, float top);
    juce::Rectangle<float> logoSize();   // its width and height
}

namespace ga
{
// Menus, tooltips and dialogs in the theme's colours. The editor shell holds one, with the plug-in's
// theme; every component finds the theme through it (ga::themeOf).
class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit LookAndFeel (Theme = {});

    const Theme theme;

    // compact menus in the faceplate colours, accent highlight
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    void drawPopupMenuSectionHeader (juce::Graphics&, const juce::Rectangle<int>&, const juce::String&) override;
    void drawCornerResizer (juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;   // accent
};
}
