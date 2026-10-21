#pragma once

#include "Controls.h"
#include "Meters.h"

namespace ga
{
// The bottom row every GA plug-in shares, three boxes 4 px apart:
//   output:   Bypass, Dry/Wet, Bass Mono
//   settings: oversampling, anti-alias filter, window size
//   phase:    correlation meter, Phase Protect, goniometer button
// The output and settings boxes have fixed widths; the phase box takes the rest.
class BottomStrip : public juce::Component, private juce::Timer
{
public:
    // what the strip needs from the plug-in
    struct Host
    {
        std::function<bool()> isBypassed;                     // Bypass on or Dry/Wet at 0: dims the output controls
        std::function<bool (const char* id)> isLocked;        // Dry/Wet and Bass Mono, locked against preset changes
        std::function<void (const char* id)> toggleLock;
        std::function<int (float* l, float* r, int max)> readScope;   // newest output samples, returns how many
        std::function<float()> protectActivity;               // how far Phase Protect pulls the side back, 0..1
        std::function<int()> latencySamples;
        juce::String oversamplingTooltip;                     // the current latency is added at the end
    };

    static constexpr int kMargin = 4;   // empty space around the boxes, so their edges are never clipped
    static constexpr int kBoxHeight = 28;

    BottomStrip (juce::AudioProcessorValueTreeState&, Host);
    ~BottomStrip() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setScopeOpen (bool open) { scopeButton.setToggleState (open, juce::dontSendNotification); }
    std::function<void (bool)> onScopeToggle;                              // the goniometer button
    std::function<void (const float*, const float*, int)> onScopeData;    // while the goniometer is open

    MenuBox sizeBox;   // window size; the editor shell fills it

private:
    void timerCallback() override;
    void updateDimming();            // the output controls are dimmed while the effect is bypassed
    void updateOutputControls();     // Bypass (not clickable at Dry/Wet 0), locks, Phase Protect
    void updateMeters();             // correlation and goniometer from the processor's output

    Host host;
    std::atomic<float>* dryWetValue;

    SquareToggle bypass { SquareToggle::Icon::power, true };   // lit while the effect is active
    Knob dryWet, bassMono;
    LedToggle phaseProtect { "Phase Protect" };
    StepBox osFactor, osFilter;
    CorrelationMeter correlation;
    SquareToggle scopeButton { SquareToggle::Icon::scope, false };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAtt, protectAtt;

    std::vector<float> scopeL, scopeR;
    double accLR = 0.0, accLL = 0.0, accRR = 0.0;
    int silentFrames = 0, shownLatency = -1;
    bool allOff = false, dimmingKnown = false;
    float protectFlash = 0.0f;    // 0..1, the orange flash of the Phase Protect LED
    double protectHoldUntil = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BottomStrip)
};
}
