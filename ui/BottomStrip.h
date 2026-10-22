#pragma once

#include "Controls.h"
#include "Meters.h"
#include "plugin/ProcessorConfig.h"

namespace ga
{
// The bottom row every GA plug-in shares, three boxes 4 px apart:
//   output:   Bypass, Dry/Wet, Bass Mono
//   settings: oversampling, anti-alias filter, window size
//   phase:    correlation meter, Phase Protect, goniometer button
// Features the processor config switches off are left out; the others keep their places, except
// Phase Protect, which moves to the right end when there is no goniometer button.
// Laid out at a fixed native size (kWidth x kBoxHeight) and scaled as one piece: the plug-in gives it
// an area with setArea(), and it fills it as far as its proportions allow, centred.
class BottomStrip : public juce::Component, private juce::Timer
{
public:
    // what the strip needs from the plug-in
    struct Host
    {
        std::function<bool()> isBypassed;                     // Bypass on or Dry/Wet at 0: dims the output controls
        std::function<bool (const char* id)> isLocked;        // Dry/Wet and Bass Mono, locked against preset changes
        std::function<void (const char* id)> toggleLock;      // (only with ProcessorConfig::dryWetLocks)
        std::function<int (float* l, float* r, int max)> readScope;   // newest output samples, returns how many
        std::function<float()> protectActivity;               // how far Phase Protect pulls the side back, 0..1
        std::function<int()> latencySamples;
        juce::String oversamplingTooltip;                     // the current latency is added at the end
    };

    static constexpr int kWidth = 750, kBoxHeight = 28;   // native size of the three boxes

    // Place the strip: the outer edges of its boxes fill the area, as far as the proportions allow,
    // centred. Use this, not setBounds(): the strip reaches a little beyond its boxes.
    void setArea (juce::Rectangle<int> area);
    static int heightForWidth (int width);   // the area height that fits a given width exactly

    BottomStrip (juce::AudioProcessorValueTreeState&, Host, const ProcessorConfig&);
    ~BottomStrip() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setScopeOpen (bool open) { if (scopeButton != nullptr) scopeButton->setToggleState (open, juce::dontSendNotification); }
    std::function<void (bool)> onScopeToggle;                              // the goniometer button
    std::function<void (const float*, const float*, int)> onScopeData;    // while the goniometer is open

    MenuBox sizeBox;   // window size; the editor shell fills it

private:
    void timerCallback() override;
    void updateDimming();            // the output controls are dimmed while the effect is bypassed
    void updateOutputControls();     // Bypass (not clickable at Dry/Wet 0), locks, Phase Protect
    void updateMeters();             // correlation and goniometer from the processor's output

    static constexpr int kMargin = 4;   // empty space around the boxes, so their edges are never clipped

    Host host;
    const bool locks;
    std::atomic<float>* dryWetValue;

    // null where the config switches the feature off
    SquareToggle bypass { SquareToggle::Icon::power, true };   // lit while the effect is active
    Knob dryWet;
    std::unique_ptr<Knob> bassMono;
    std::unique_ptr<LedToggle> phaseProtect;
    std::unique_ptr<StepBox> osFactor, osFilter;
    CorrelationMeter correlation;
    std::unique_ptr<SquareToggle> scopeButton;

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
