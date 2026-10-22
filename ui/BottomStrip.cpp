#include "BottomStrip.h"
#include "plugin/SharedIds.h"

namespace ga
{
namespace
{
    // box widths and x positions inside the boxes, from the strip's left edge (inside the margin)
    constexpr int kOutX = 0, kOutW = 214;                     // Bypass, Dry/Wet, Bass Mono
    constexpr int kSetX = kOutX + kOutW + 4, kSetW = 184;     // oversampling, filter, size
    constexpr int kCorX = kSetX + kSetW + 4;                  // correlation, Phase Protect, goniometer
    constexpr int kCorW = BottomStrip::kWidth - kCorX;

    constexpr int kSdBypass = kOutX + 9, kSdDryWet = kOutX + 36, kSdBass = kOutX + 118;
    constexpr int kSdOs = kSetX + 8, kSdFilt = kSdOs + 41, kSdSize = kSdFilt + 87;
    constexpr int kSdMeterW = 220;                            // the correlation meter, from the box's left edge + 7
    constexpr int kSdProtect = kCorX + 230, kSdProtectW = 84;

    juce::RangedAudioParameter& parameter (juce::AudioProcessorValueTreeState& s, const char* id)
    {
        return *s.getParameter (id);
    }
}

BottomStrip::BottomStrip (juce::AudioProcessorValueTreeState& state, Host h)
    : host (std::move (h)),
      dryWetValue (state.getRawParameterValue (pid::dryWet)),
      dryWet   (parameter (state, pid::dryWet),   "Dry/Wet",   false, Knob::Style::mini),
      bassMono (parameter (state, pid::bassMono), "Bass Mono", false, Knob::Style::mini),
      osFactor (parameter (state, pid::osFactor), true, false),
      osFilter (parameter (state, pid::osFilter), true, false)
{
    setInterceptsMouseClicks (false, true);   // only the controls, not the margin

    for (auto* c : std::initializer_list<juce::Component*> { &bypass, &dryWet, &bassMono, &phaseProtect, &osFactor, &osFilter, &sizeBox,
                                                             &correlation, &scopeButton })
        addAndMakeVisible (c);

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    bypassAtt = std::make_unique<ButtonAttachment> (state, pid::bypass, bypass);
    protectAtt = std::make_unique<ButtonAttachment> (state, pid::phaseProtect, phaseProtect);

    // Dry/Wet and Bass Mono: click the name to lock the value against preset changes
    for (auto [knob, id] : { std::pair<Knob*, const char*> { &dryWet, pid::dryWet }, { &bassMono, pid::bassMono } })
        knob->setLockable ([this, id = id] { host.toggleLock (id); updateOutputControls(); });

    bypass.setTooltip ("Bypass: click-free, at the same loudness as the effect. Dry/Wet at 0 % bypasses too.");
    phaseProtect.setTooltip ("Phase Protect: keeps the effect mono-compatible. The LED flashes orange while it audibly pulls the width back.");
    scopeButton.setTooltip ("Show / hide the goniometer");
    osFilter.setTooltip ("Anti-alias filter: Linear Phase (cleanest, more latency) or Low Latency (minimal delay)");
    sizeBox.setTooltip ("Window size (or drag the bottom right corner)");

    scopeButton.onClick = [this] { if (onScopeToggle != nullptr) onScopeToggle (scopeButton.getToggleState()); };

    scopeL.assign (16384, 0.0f);
    scopeR.assign (16384, 0.0f);

    updateDimming();
    startTimerHz (60);
}

BottomStrip::~BottomStrip()
{
    stopTimer();
}

void BottomStrip::setArea (juce::Rectangle<int> area)
{
    const float scale = juce::jmin ((float) area.getWidth() / (float) kWidth, (float) area.getHeight() / (float) kBoxHeight);
    const auto boxes = juce::Rectangle<float> ((float) kWidth * scale, (float) kBoxHeight * scale).withCentre (area.toFloat().getCentre());
    const auto native = juce::Rectangle<int> (kWidth, kBoxHeight).expanded (kMargin);

    if (scale == 1.0f && boxes.getTopLeft() == boxes.getTopLeft().toInt().toFloat())
    {
        setTransform ({});
        setBounds (native + boxes.getTopLeft().toInt());
    }
    else
    {
        setBounds (native);
        setTransform (juce::AffineTransform::scale (scale).translated (boxes.getX(), boxes.getY()));
    }
}

int BottomStrip::heightForWidth (int width)
{
    return juce::roundToInt ((float) width * (float) kBoxHeight / (float) kWidth);
}

void BottomStrip::timerCallback()
{
    updateDimming();
    updateOutputControls();
    updateMeters();

    if (host.latencySamples() != shownLatency)
    {
        shownLatency = host.latencySamples();
        osFactor.setTooltip (host.oversamplingTooltip + " Latency now: " + juce::String (shownLatency) + " samples.");
    }
}

void BottomStrip::updateDimming()
{
    // Bypass (or Dry/Wet at 0) dims what shapes the sound; Bypass itself, the settings and the meters stay bright
    const bool off = host.isBypassed();
    if (dimmingKnown && off == allOff)
        return;

    for (auto* c : { (juce::Component*) &dryWet, (juce::Component*) &bassMono })
        c->setAlpha (off ? ui::kDimAlpha : 1.0f);

    allOff = off;
    dimmingKnown = true;
}

// cheap, so it runs on every timer tick
void BottomStrip::updateOutputControls()
{
    // at Dry/Wet 0 the effect is already bypassed: Bypass cannot be clicked then, so turning
    // Dry/Wet up again always brings the effect back
    const bool canBypass = dryWetValue->load() > 0.0f;
    if (bypass.isEnabled() != canBypass)
    {
        bypass.setEnabled (canBypass);
        bypass.setAlpha (canBypass ? 1.0f : 0.35f);
    }
    phaseProtect.setAlpha (allOff ? ui::kDimAlpha : 1.0f);
    dryWet.setLocked (host.isLocked (pid::dryWet));
    bassMono.setLocked (host.isLocked (pid::bassMono));
}

void BottomStrip::updateMeters()
{
    const int n = host.readScope (scopeL.data(), scopeR.data(), (int) scopeL.size());

    double lr = 0.0, ll = 0.0, rr = 0.0;
    for (int i = 0; i < n; ++i)
    {
        lr += (double) scopeL[(size_t) i] * scopeR[(size_t) i];
        ll += (double) scopeL[(size_t) i] * scopeL[(size_t) i];
        rr += (double) scopeR[(size_t) i] * scopeR[(size_t) i];
    }

    // nothing arriving (transport stopped) or below about -80 dBFS for ~200 ms: the meter glides back to 0
    const bool quiet = n == 0 || ll + rr < 1.0e-8 * (double) n;
    silentFrames = quiet ? silentFrames + 1 : 0;
    if (silentFrames > 12)
    {
        accLR = accLL = accRR = 0.0;
        correlation.setTarget (0.0f, false);
    }
    else if (n > 0)
    {
        // correlation = <L R> / sqrt (<L L> <R R>), averaged over roughly the last 150 ms
        accLR = accLR * 0.88 + lr;
        accLL = accLL * 0.88 + ll;
        accRR = accRR * 0.88 + rr;
        correlation.setTarget ((float) (accLR / std::sqrt (accLL * accRR + 1.0e-30)), true);
    }

    if (scopeButton.getToggleState() && onScopeData != nullptr)
        onScopeData (scopeL.data(), scopeR.data(), n);

    // Phase Protect making an audible difference (side pulled back by more than 2 dB): its LED
    // flashes orange, held 0.5 s so short bursts are seen. Below 2 dB it only fine-tunes; measured
    // on Hyper! Hyper! Dimension's factory presets, only those that would otherwise go negative get past 2 dB.
    const auto now = juce::Time::getMillisecondCounterHiRes();
    if (! allOff && host.protectActivity() > 0.206f)   // 1 - 10^(-2/20)
        protectHoldUntil = now + 500.0;
    const float flash = now < protectHoldUntil ? 0.5f + 0.5f * std::cos ((float) (now * 0.001) * juce::MathConstants<float>::twoPi * 1.4f) : 0.0f;
    if (std::abs (flash - protectFlash) > 0.01f)
    {
        protectFlash = flash;
        phaseProtect.setAlarm (flash);
    }
}

void BottomStrip::paint (juce::Graphics& g)
{
    const int x0 = kMargin, y0 = kMargin;
    for (auto [x, w] : { std::pair<int, int> { kOutX, kOutW }, { kSetX, kSetW }, { kCorX, kCorW } })
        ui::drawInsetPanel (g, juce::Rectangle<float> ((float) (x0 + x), (float) y0, (float) w, (float) kBoxHeight));
}

void BottomStrip::resized()
{
    const int x0 = kMargin, y0 = kMargin;
    const int scy = y0 + kBoxHeight / 2;

    bypass.setBounds   (x0 + kSdBypass, scy - 9, 18, 18);
    dryWet.setBounds   (x0 + kSdDryWet, y0 + 1, 78, kBoxHeight - 2);
    bassMono.setBounds (x0 + kSdBass,   y0 + 1, 86, kBoxHeight - 2);
    phaseProtect.setBounds (x0 + kSdProtect, scy - 9, kSdProtectW, 18);
    osFactor.setBounds (x0 + kSdOs,   scy - 8, 38, 17);
    osFilter.setBounds (x0 + kSdFilt, scy - 8, 84, 17);
    sizeBox.setBounds  (x0 + kSdSize, scy - 8, 42, 17);

    correlation.setBounds (x0 + kCorX + 7, y0, kSdMeterW, kBoxHeight);
    scopeButton.setBounds (x0 + kCorX + kCorW - 26, scy - 9, 18, 18);
}
}
