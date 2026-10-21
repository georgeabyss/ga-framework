#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "SharedIds.h"

namespace ga
{
// The processor every GA effect shares. It owns everything around the effect itself:
//   oversampling   Off / 2x / 4x / 8x, linear phase FIR or low latency IIR, latency reported
//   bypass         click-free (20 ms), at the effect's loudness; Dry/Wet at 0 bypasses too
//   safety mute    absurd levels (above +24 dBFS) or broken audio silence the output until resumed
//   scope          the output samples for the correlation meter and the goniometer
//   state          the parameters and properties, saved as XML; migrateState() for old sessions
// The plug-in implements the effect ("wet") through the virtual functions below; the shared
// parameters (ga::addSharedParameters) must be in its layout.
class ProcessorBase : public juce::AudioProcessor
{
public:
    explicit ProcessorBase (juce::AudioProcessorValueTreeState::ParameterLayout);
    ~ProcessorBase() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool hasEditor() const override { return true; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParam; }

    // Output samples for the correlation meter and the goniometer: the audio thread writes,
    // the editor reads. Samples are dropped while nobody reads (editor closed).
    int readScope (float* l, float* r, int maxNum);

    // the Bypass button, or Dry/Wet at 0: both bypass the whole effect (and dim the editor)
    bool isBypassedNow() const;

    // Safety mute: absurd levels (above +24 dBFS) or broken audio (NaN, infinity) silence the
    // output until resumeAfterSafetyMute() is called.
    bool isSafetyMuted() const { return safetyMuted; }
    void resumeAfterSafetyMute() { resumeRequested = true; }

    juce::AudioProcessorValueTreeState apvts;

protected:
    static constexpr int kMaxOsStages = 3;   // 2x, 4x, 8x

    // the effect. prepareWet: sampleRate is the rate it runs at (already multiplied by the
    // oversampling factor); it is called once with the highest rate first, so later calls at lower
    // rates must not allocate. processWet: in place, at the oversampled rate.
    virtual void prepareWet (double sampleRate, int channels, int oversampling) = 0;
    virtual void updateWetParams() = 0;      // once per block, before processWet
    virtual void processWet (float* const* io, int numSamples) = 0;
    virtual void resetWet() = 0;             // clear all delay lines and filters

    // optional: split the block at every note-on, so noteOn() lands on the exact sample (Retrig)
    virtual bool splitAtNoteOns() const { return false; }
    virtual void noteOn() {}

    // optional: fix up the state of an older session before it is loaded
    virtual void migrateState (juce::ValueTree&) {}

    float param (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

private:
    static constexpr int kScopeSize = 16384;
    static constexpr int kDryDelaySize = 4096;      // more than the longest oversampling latency

    void updateOversampling();                      // audio thread, after a change of oversampling or filter
    void processSegment (juce::AudioBuffer<float>&, int start, int num);
    void storeDry (const juce::AudioBuffer<float>&);
    void applyBypass (juce::AudioBuffer<float>&);
    void pushScope (const juce::AudioBuffer<float>&);
    bool isAbsurd (const juce::AudioBuffer<float>&) const;
    void silence (juce::AudioBuffer<float>&);   // the safety mute
    void resetDsp();

    static constexpr float kAbsurdLevel = 15.85f;   // +24 dBFS
    std::atomic<bool> safetyMuted { false }, resumeRequested { false };

    juce::AudioProcessorParameter* bypassParam = nullptr;
    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float> bypassMix { 0.0f };   // 0 = effect, 1 = bypassed

    // [filter type][stage - 1]; 0 = linear phase FIR, 1 = low latency IIR
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplers[2][kMaxOsStages];
    juce::dsp::Oversampling<float>* activeOs = nullptr;
    int osStages = -1, osFilter = -1, maxBlock = 0, numCh = 2;
    double baseRate = 44100.0;
    bool fadeIn = false;

    // the dry signal for the bypass crossfade is delayed by the oversampling latency
    juce::AudioBuffer<float> dryDelay { 2, kDryDelaySize };
    int dryDelayPos = 0, latency = 0;

    juce::AbstractFifo scopeFifo { kScopeSize };
    std::vector<float> scopeL, scopeR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorBase)
};
}
