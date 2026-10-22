#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Style.h"

namespace ga
{
// Knob with the value printed below it and the name below that. The whole area reacts:
// drag up/down (Shift or Cmd = fine), double-click or Alt-click = default, wheel, right-click = menu.
class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    // regular: 56 x 70, value and name below. mini: small dial with name and value to its right.
    enum class Style { regular, mini };

    // centreDetent: the knob holds at its centre for a few pixels while dragging across it
    Knob (juce::RangedAudioParameter&, const juce::String& name, bool centreDetent = false, Style = Style::regular);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    // where the dial's centre is, in the knob (sections line knobs up by it)
    juce::Point<int> dialCentre() const;

    // mini style only: clicking the name calls onLockClick (like Valhalla's mix lock)
    void setLockable (std::function<void()> onLockClick);
    void setLocked (bool shouldBeLocked);

private:
    float proportion() const { return param.convertTo0to1 (value); }
    juce::Rectangle<float> nameArea() const;
    juce::Rectangle<float> lockArea() const;   // the name plus the padlock after it: the click area
    juce::String valueString() const;
    float toTravel (float prop) const;
    float fromTravel (float t) const;
    void resetToDefault();
    void showMenu();
    void showValueEditor();
    void closeValueEditor();

    juce::RangedAudioParameter& param;
    juce::String nameText;
    Style style;
    float detentWidth = 0.0f;   // in travel units (1 = full range)
    juce::ParameterAttachment attach;
    float value = 0.0f, travel = 0.0f, lastY = 0.0f;
    bool dragging = false, locked = false, overName = false;
    std::function<void()> onLockClick;
    std::unique_ptr<juce::TextEditor> valueEditor;
};

// integer / choice value with two small arrows. Number box: drag up/down, click the arrows, wheel.
// With showNames the box is wide, shows the current choice and opens a list when clicked;
// without arrows it is a plain drop-down. Right-click = menu (all values, reset, host items).
// setWideNumber() makes a number box the size of a drop-down (Stages): its arrows follow a rule of
// the plug-in's (double / halve), dragging moves single steps.
class StepBox : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit StepBox (juce::RangedAudioParameter&, bool showNames = false, bool showArrows = true);

    // arrowStep (value, +1 or -1) gives the value after an arrow click; pxPerStep is the drag distance
    // for one step (Shift: 2.4 times as far)
    void setWideNumber (std::function<int (int value, int direction)> arrowStep, float pxPerStep);

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::Rectangle<float> boxRect() const;
    int arrowAt (juce::Point<float>) const;   // +1 up arrow, -1 down arrow, 0 elsewhere
    void step (int delta);
    void showMenu (bool valuesOnly);
    juce::String valueText (int v) const;

    juce::RangedAudioParameter& param;
    bool names = false, arrows = true, wide = false;
    std::function<int (int, int)> arrowStep;
    float pxPerStep = 10.0f;
    juce::ParameterAttachment attach;
    int value = 0, minValue = 0, maxValue = 1, hoverArrow = 0;
    float lastY = 0.0f, dragAccum = 0.0f, wheelAccum = 0.0f;
    bool dragging = false;
};

// small square lens lamp. Only the lens itself reacts to the mouse.
class LampButton : public juce::Button
{
public:
    LampButton() : juce::Button ("lamp") { setSize (16, 16); setClickingTogglesState (true); setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    bool hitTest (int x, int y) override;
    void paintButton (juce::Graphics&, bool, bool) override;
};

// recessed square toggle with a small icon
class SquareToggle : public juce::Button
{
public:
    enum class Icon { power, scope };
    SquareToggle (Icon i, bool lightWhenOff) : juce::Button ("toggle"), icon (i), invert (lightWhenOff)
    {
        setClickingTogglesState (true);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    void paintButton (juce::Graphics&, bool, bool) override;

private:
    Icon icon;
    bool invert;
};

// round LED at the end of a section header. Click = section on/off, instant and click-free.
// It is lit while the section is really active (on, and its amount above 0).
class SectionLed : public juce::Button
{
public:
    SectionLed() : juce::Button ("led") { setSize (14, 14); setClickingTogglesState (true); setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void setLit (bool shouldBeLit) { if (shouldBeLit != lit) { lit = shouldBeLit; repaint(); } }
    void paintButton (juce::Graphics&, bool isOver, bool isDown) override;

private:
    bool lit = true;
};

// a label with a round LED after it; label and LED are one switch (Phase Protect)
class LedToggle : public juce::Button
{
public:
    explicit LedToggle (const juce::String& label) : juce::Button (label)
    {
        setClickingTogglesState (true);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    void setAlarm (float amount) { if (std::abs (amount - alarm) > 0.01f) { alarm = amount; repaint(); } }   // 0..1, orange
    void paintButton (juce::Graphics&, bool isOver, bool isDown) override;
    float ledCentreX() const;   // where the LED sits, after the label

private:
    float alarm = 0.0f;
};

// a dice icon: click to randomise something (a section, everything)
class DiceButton : public juce::Button
{
public:
    DiceButton() : juce::Button ("dice") { setSize (14, 14); setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paintButton (juce::Graphics&, bool isOver, bool isDown) override;
};

// The point a control is lined up by: a knob's dial, otherwise the middle of the control.
juce::Point<int> anchorOf (const juce::Component&);

// small drop-down box for a setting that is not a parameter (window size)
class MenuBox : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<juce::StringArray()> getItems;
    std::function<juce::String()> getText;   // what the box shows
    std::function<int()> getSelected;
    std::function<void (int)> onSelect;

    MenuBox() { setMouseCursor (juce::MouseCursor::PointingHandCursor); setRepaintsOnMouseActivity (true); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
};
}
