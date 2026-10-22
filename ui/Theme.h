#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Looks.h"

namespace ga
{
// How a text is set: size, weight, letter spacing, and sans or monospaced.
struct TextStyle
{
    float size = 10.0f;
    bool bold = false;
    float kerning = 0.0f;   // letter spacing, as a fraction of the size
    bool mono = false;
};

// Everything a GA faceplate is drawn with: colours, type, sizes and looks. The defaults are the GA look
// (Hyper! Hyper! Dimension). A plug-in passes its theme to ga::EditorShell; every component reads it
// through ga::themeOf(). For another colour, start from withAccent() and override what you like.
struct Theme
{
    struct Colours
    {
        // the accent: pointers, LEDs, active states, and the shades derived from it
        juce::Colour accent        { 0xffb18cfe };
        juce::Colour accentHi      { 0xffd9c7ff };   // hover, highlights, goniometer trace
        juce::Colour accentLight   { 0xffeadfff };   // title gradient, top
        juce::Colour accentDeep    { 0xff7c55d6 };   // title gradient, bottom; rim of a lit LED
        juce::Colour onAccent      { 0xff141518 };   // text on an accent background (menu highlight)
        juce::Colour locked        { 0xffb9a6e6 };   // the name of a locked value

        // lamps (square lens) and LEDs (round lens)
        juce::Colour lampLit       { 0xffe6d8ff };   // centre of a lit lamp
        juce::Colour lampLitEdge   { 0xff8559e0 };
        juce::Colour ledLit        { 0xfff3ecff };   // centre of a lit LED
        juce::Colour lensOff       { 0xff3a3148 };   // centre of an unlit lamp or LED
        juce::Colour lampOffEdge   { 0xff251f30 };
        juce::Colour ledOffEdge    { 0xff1e1a26 };

        // alarm: solo, Phase Protect pulling back, safety mute
        juce::Colour alarm         { 0xffff9a3c };
        juce::Colour alarmLight    { 0xfffff0e0 };   // centre of an LED in alarm
        juce::Colour alarmDeep     { 0xffc8641e };   // rim of an LED in alarm
        juce::Colour negative      { 0xffff7a8a };   // negative correlation

        // text
        juce::Colour text          { 0xfff0f1f3 };   // dialogs, tooltips, the value being typed
        juce::Colour value         { 0xffe4e6ea };   // values under knobs, the meter reading, menus
        juce::Colour valueActive   { 0xffffffff };   // the value while it is dragged
        juce::Colour boxText       { 0xfff1f2f5 };   // inside step boxes and drop-downs
        juce::Colour label         { 0xff8f9299 };   // names, titles
        juce::Colour labelHover    { 0xffc9cbd1 };
        juce::Colour subtitle      { 0xff7d8087 };
        juce::Colour mark          { 0xff9a9da6 };   // arrows, drop-down marks, the knob's centre dot
        juce::Colour markDim       { 0xff5d6068 };   // the knob's end dots, meter scale, idle reading, unlit icons

        // surfaces
        juce::Colour faceplateTop    { 0xff1a1b1e };
        juce::Colour faceplateBottom { 0xff0d0e10 };
        juce::Colour faceplateEdge   { 0xff4a4d54 };   // the faceplate's top edge
        juce::Colour recessTop       { 0xff08090a };   // recessed boxes (step boxes, toggles, meters, text fields)
        juce::Colour recessBottom    { 0xff131417 };
        juce::Colour recessEdge      { 0xff34373c };   // the lit lower edge of a recess; meter ticks
        juce::Colour insetTop        { 0xff0c0d0f };   // inset panels (bottom strip, goniometer)
        juce::Colour insetBottom     { 0xff121316 };
        juce::Colour hole            { 0xff050506 };   // rack slots, lamp holders
        juce::Colour track           { 0xff2a2c31 };   // mini knob track, goniometer outline, field outlines
        juce::Colour grid            { 0xff1f2125 };   // goniometer axes
        juce::Colour popup           { 0xff17181b };   // menus, tooltips, dialogs
        juce::Colour popupBottom     { 0xff111215 };   // menu gradient, bottom (the top is faceplateTop)
        juce::Colour button          { 0xff222327 };   // dialog buttons

        // screws
        juce::Colour screwLight      { 0xff7a7d84 };
        juce::Colour screwDark       { 0xff1c1d20 };
        juce::Colour screwCross      { 0xff0d0e10 };

        // knob faces (r, g, b lifted by knobHoverLift while hovered or dragged)
        juce::Colour knobRingTop     { 0xff5a5a5e };
        juce::Colour knobRingBottom  { 0xff424246 };
        juce::Colour miniRingBottom  { 0xff38383c };
        juce::Colour knobCoreTop     { 0xff2a2a2c };
        juce::Colour knobCoreBottom  { 0xff232325 };

        // the preset display
        juce::Colour displayTop      { 0xff07060a };   // the glass
        juce::Colour displayBottom   { 0xff120e17 };
        juce::Colour displayDotOff   { 0xff2a2232 };
        juce::Colour displayDotOn    { 0xfff1e9ff };   // glows in accent
        juce::Colour displayMuteGlow { 0xffff6e32 };   // safety mute warning
        juce::Colour displayMuteDot  { 0xffffd2b0 };
        juce::Colour displayMuteEdge { 0xffff7832 };

        // the gothic GA: silver gradient, top to bottom
        juce::Colour logoTop         { 0xffffffff };
        juce::Colour logoUpper       { 0xffaeb2ba };   // at 45 %
        juce::Colour logoLower       { 0xffd9dce1 };   // at 55 %
        juce::Colour logoBottom      { 0xff737780 };
    } colours;

    struct Type
    {
        juce::String sans;   // typeface names; empty = the system's default sans / monospaced
        juce::String mono;

        TextStyle title      { 24.0f, true, 0.075f };
        TextStyle subtitle   { 7.5f, false, 0.37f };
        TextStyle name       { 6.8f, false, 0.15f };     // under a knob or control
        TextStyle label      { 6.5f, false, 0.15f };     // small labels in the bottom strip
        TextStyle scale      { 6.0f };                   // meter scale (-1, +1)
        TextStyle panelTitle { 6.5f, true, 0.14f };      // GONIOMETER
        TextStyle scopeAxis  { 6.5f, true, 0.12f };      // M, L, R
        TextStyle value      { 8.5f, false, 0.0f, true };
        TextStyle valueEdit  { 9.0f, false, 0.0f, true };
        TextStyle boxText    { 7.0f, true, 0.0f, true }; // step boxes with names, drop-downs
        TextStyle boxNumber  { 11.5f, true, 0.0f, true };
        TextStyle menu       { 10.5f };
        TextStyle menuHeader { 7.5f, true, 0.14f };
    } type;

    // Sizes: the drawing code keeps today's sizes and multiplies them by token / default, which is
    // exactly 1 for the defaults. A larger knob, LED or lamp grows as a whole; the defaults stay
    // bit-identical.
    struct Sizes
    {
        float knobRadius = 14.0f;       // the whole face grows with it: dots, shadow, pointer
        float miniKnobRadius = 7.0f;    // with its value arc and pointer
        int   knobHoverLift = 12;       // knob face brightening while hovered, per colour channel
        float ledRadius = 3.0f;         // its glow, lens highlight and hover ring grow with it
        float lampSize = 12.0f;         // the lens; its holder and reflections grow with it
        float recessCorner = 2.5f;
        float insetCorner = 5.0f;       // its inner edge and bottom highlight follow
        float screwInsetX = 12.0f;      // screw centres, from the window's corners
        float screwInsetY = 22.0f;
        float titleBaseline = 17.0f;    // from the top of the header row; the subtitle follows the title's size
        float logoScale = 1.0f;         // the gothic GA
        float dimAlpha = 0.45f;         // controls of a section (or effect) that is off
    } sizes;

    // the parts drawn by a swappable look (ui/Looks.h): the GA ones, or the plug-in's own
    struct Looks
    {
        std::shared_ptr<const TextEffect> title = std::make_shared<EchoSmear>();
        std::shared_ptr<const FaceplateStyle> faceplate = std::make_shared<GradientFaceplate>();
        std::shared_ptr<const PresetDisplayStyle> presetDisplay = std::make_shared<LedMatrixDisplay>();
        std::shared_ptr<const KnobFace> knob = std::make_shared<GaKnobFace>();
    } looks;

    // The GA theme with another accent colour: the highlight and the dark shades (lenses, the preset
    // display) are derived from it. Override any of them afterwards.
    static Theme withAccent (juce::Colour accent);

    juce::Font font (const TextStyle&) const;
};

// the theme of the editor a component is in (its ga::LookAndFeel), or the default GA theme
const Theme& themeOf (const juce::Component&);
}
