#pragma once

#include "Controls.h"

namespace ga
{
// The modulation source's handle (the Env Follower's grip): a square recess with a 2 x 3 grip.
// Drag it onto a knob that takes modulation (Knob::onModDrop); while it is dragged, those knobs show
// a dashed outline and the one under the cursor a solid one. The editor shell runs the drag.
class ModSource : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ModSource (juce::String ghostText = "ENV");

    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

private:
    juce::Image ghost() const;   // the label under the cursor while dragging

    juce::String ghostText;
};

// A modulation destination's depth: a ring with an arc from 12 o'clock (clockwise = positive) and a
// zero tick. Drag up/down for the depth, double-click to remove, hover to highlight its knob.
class ModDonut : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ModDonut (juce::RangedAudioParameter& depth);   // a range symmetric around 0

    std::function<void()> onRemove;          // double-click
    std::function<void (bool)> onHover;      // the pointer entered (true) or left (false)

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    float depth() const;   // -1..1

private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attach;
    float value = 0.0f, lastY = 0.0f;
    bool dragging = false;
};

// every knob under root that takes modulation (for the editor shell, while a drag runs)
void forEachModTarget (juce::Component& root, const std::function<void (Knob&)>&);
}
