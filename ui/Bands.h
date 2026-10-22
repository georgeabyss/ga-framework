#pragma once

#include "Controls.h"

namespace ga
{
// A band as a card: the plug-in's on/off LED, its name, a stage count (or any short info) and its
// solo button on the top line; a range line; one or two traces (e.g. the band's impulse response);
// the plug-in's drop-down and a status word on the bottom line. Click the card to select it.
class BandCard : public juce::Component
{
public:
    BandCard();

    // the plug-in's controls, placed by the card: LED left of the name, solo top right,
    // drop-down bottom left (each keeps its own size); any may be null
    void setControls (juce::Component* led, juce::Component* solo, juce::Component* dropdown);

    void setName (const juce::String&);
    void setInfo (const juce::String& topRight, const juce::String& range, const juce::String& status);

    // a trace: per column the lowest and highest value (-1..1), drawn over the card's width
    struct Trace
    {
        std::vector<float> low, high;
    };
    void setTraces (Trace main, Trace second = {});   // second: dimmer, e.g. the other channel

    void setSelected (bool);
    bool isSelected() const { return selected; }
    void setActive (bool);   // off: the trace turns grey and the texts dim
    std::function<void()> onSelect;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::Rectangle<int> traceArea() const;

    juce::Component *led = nullptr, *solo = nullptr, *dropdown = nullptr;
    juce::String name, info, range, status;
    Trace main, second;
    bool selected = false, active = true;
};

// A crossover between two band cards: the frequency in a recess, "SPLIT" under it. Drag left/right
// to move it, double-click to reset. limit, if set, keeps the value legal (e.g. away from the other
// splits).
class SplitHandle : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit SplitHandle (juce::RangedAudioParameter&);

    std::function<float (float proposed)> limit;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void set (float proportion, bool asGesture);

    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attach;
    float value = 0.0f, lastX = 0.0f;
    bool dragging = false;
};

// Band cards side by side, sharing the width, with fixed-width split handles between them.
class BandStrip : public juce::Component
{
public:
    BandStrip();

    // splits.size() must be cards.size() - 1; the strip shows them, the caller owns them
    void setBands (std::vector<BandCard*> cards, std::vector<SplitHandle*> splits);

    void paint (juce::Graphics&) override;   // the selected card's glow
    void resized() override;

private:
    std::vector<BandCard*> cards;
    std::vector<SplitHandle*> splits;
};
}
