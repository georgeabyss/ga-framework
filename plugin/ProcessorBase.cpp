#include "ProcessorBase.h"

namespace ga
{

ProcessorBase::ProcessorBase (juce::AudioProcessorValueTreeState::ParameterLayout layout, ProcessorConfig c)
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", std::move (layout)),
      config (std::move (c))
{
   #ifdef JucePlugin_WantsMidiInput
    jassert (config.midiInput == (bool) JucePlugin_WantsMidiInput);   // ga_add_plugin (NEEDS_MIDI_INPUT) must agree
   #endif
    if (config.oversampling)
    {
        for (int f : config.oversamplingFactors)
        {
            jassert (juce::isPowerOfTwo (f) && f <= (1 << kMaxOsStages));
            maxOsStages = juce::jmax (maxOsStages, juce::jlimit (0, kMaxOsStages, juce::roundToInt (std::log2 ((double) f))));
        }
        osFactorValue = apvts.getRawParameterValue (pid::osFactor);
        osFilterValue = apvts.getRawParameterValue (pid::osFilter);
    }
    bypassParam = apvts.getParameter (pid::bypass);
    scopeL.assign ((size_t) kScopeSize, 0.0f);
    scopeR.assign ((size_t) kScopeSize, 0.0f);
}

bool ProcessorBase::isBypassedNow() const
{
    return param (pid::bypass) > 0.5f || param (pid::dryWet) <= 0.0f;
}

bool ProcessorBase::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void ProcessorBase::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    baseRate = sampleRate;
    maxBlock = juce::jmax (1, samplesPerBlock);
    numCh = juce::jmax (1, juce::jmin (2, getTotalNumInputChannels()));

    using OS = juce::dsp::Oversampling<float>;
    for (int f = 0; f < 2; ++f)
        for (int s = 0; s < (config.oversampleWet ? maxOsStages : 0); ++s)
        {
            auto& o = oversamplers[f][s];
            if (o == nullptr)
                o = std::make_unique<OS> (2, (size_t) (s + 1),
                                          f == 0 ? OS::filterHalfBandFIREquiripple : OS::filterHalfBandPolyphaseIIR,
                                          true, true);   // max quality, integer latency
            o->initProcessing ((size_t) maxBlock);
        }

    // allocate for the highest rate (or, oversampling itself, the highest factor)
    prepareWet (config.oversampleWet ? sampleRate * (1 << maxOsStages) : sampleRate, numCh, 1 << maxOsStages);
    osStages = osFilter = -1;
    updateOversampling();
    fadeIn = false;

    dryBuffer.setSize (2, maxBlock);
    bypassMix.reset (sampleRate, 0.02);
    bypassMix.setCurrentAndTargetValue (isBypassedNow() ? 1.0f : 0.0f);
}

void ProcessorBase::updateOversampling()
{
    int stages = 0, filter = 0;
    if (osFactorValue != nullptr)
    {
        const auto& factors = config.oversamplingFactors;
        const int index = juce::jlimit (0, (int) factors.size() - 1, (int) (osFactorValue->load() + 0.5f));
        stages = juce::jlimit (0, maxOsStages, juce::roundToInt (std::log2 ((double) factors[(size_t) index])));
        filter = juce::jlimit (0, 1, (int) (osFilterValue->load() + 0.5f));
    }
    if (stages == osStages && filter == osFilter)
        return;

    osStages = stages;
    osFilter = filter;
    activeOs = config.oversampleWet && stages > 0 ? oversamplers[filter][stages - 1].get() : nullptr;
    if (activeOs != nullptr)
        activeOs->reset();

    // no allocation here: everything was sized for the highest rate in prepareToPlay
    prepareWet (config.oversampleWet ? baseRate * (1 << stages) : baseRate, numCh, 1 << stages);
    latency = activeOs != nullptr ? juce::roundToInt (activeOs->getLatencyInSamples())
                                  : config.oversampleWet ? 0 : wetLatency();
    setLatencySamples (latency);
    dryDelay.clear();
    dryDelayPos = 0;
    fadeIn = true;   // the effect restarts empty, so fade the output back in
}

void ProcessorBase::processSegment (juce::AudioBuffer<float>& buffer, int start, int num)
{
    const int chans = juce::jmin (buffer.getNumChannels(), 2);
    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) chans, (size_t) buffer.getNumSamples());

    for (int done = 0; done < num;)   // the oversamplers take at most maxBlock samples at a time
    {
        const int len = juce::jmin (maxBlock, num - done);
        auto sub = block.getSubBlock ((size_t) (start + done), (size_t) len);
        if (activeOs != nullptr)
        {
            auto up = activeOs->processSamplesUp (sub);
            float* io[2] = { up.getChannelPointer (0), chans > 1 ? up.getChannelPointer (1) : nullptr };
            processWet (io, (int) up.getNumSamples());
            activeOs->processSamplesDown (sub);
        }
        else
        {
            float* io[2] = { sub.getChannelPointer (0), chans > 1 ? sub.getChannelPointer (1) : nullptr };
            processWet (io, len);
        }
        done += len;
    }
}

void ProcessorBase::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, n);
    if (n == 0 || buffer.getNumChannels() == 0)
        return;

    updateOversampling();

    // Safety mute (like REAPER): on absurd levels or broken audio the output goes silent and stays
    // silent until the user resumes (click on the preset display).
    if (resumeRequested.exchange (false))
    {
        resetDsp();
        safetyMuted = false;
        fadeIn = true;
    }
    if (safetyMuted || isAbsurd (buffer))   // checked on the input ...
    {
        silence (buffer);
        return;
    }

    storeDry (buffer);
    updateWetParams();

    // process in segments split at every note-on, so noteOn() lands on the exact sample
    int done = 0;
    auto processUpTo = [&] (int end)
    {
        if (end <= done) return;
        processSegment (buffer, done, end - done);
        done = end;
    };
    if (config.midiInput && splitAtNoteOns())
        for (const auto meta : midi)
            if (meta.getMessage().isNoteOn())
            {
                processUpTo (juce::jlimit (0, n, meta.samplePosition));
                noteOn();
            }
    processUpTo (n);

    const int chans = juce::jmin (buffer.getNumChannels(), 2);
    if (fadeIn)
    {
        for (int ch = 0; ch < chans; ++ch)
            buffer.applyGainRamp (ch, 0, n, 0.0f, 1.0f);
        fadeIn = false;
    }

    applyBypass (buffer);
    if (isAbsurd (buffer))   // ... and on the output
    {
        silence (buffer);
        return;
    }
    pushScope (buffer);
}

bool ProcessorBase::isAbsurd (const juce::AudioBuffer<float>& buffer) const
{
    for (int ch = 0; ch < juce::jmin (buffer.getNumChannels(), 2); ++ch)
    {
        const auto* x = buffer.getReadPointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (! (std::abs (x[i]) <= config.safetyLevel))   // also catches NaN and infinity
                return true;
    }
    return false;
}

void ProcessorBase::silence (juce::AudioBuffer<float>& buffer)
{
    if (! safetyMuted)
    {
        safetyMuted = true;
        resetDsp();   // a filter or delay that took in the bad values would keep producing them
    }
    buffer.clear();
    pushScope (buffer);
}

void ProcessorBase::resetDsp()
{
    resetWet();
    if (activeOs != nullptr)
        activeOs->reset();
    dryDelay.clear();
}

void ProcessorBase::storeDry (const juce::AudioBuffer<float>& buffer)
{
    // the dry copy for the click-free bypass crossfade, delayed by the oversampling latency
    const int n = buffer.getNumSamples();
    if (dryBuffer.getNumSamples() < n)
        dryBuffer.setSize (2, n, false, false, true);
    for (int ch = 0; ch < juce::jmin (buffer.getNumChannels(), 2); ++ch)
    {
        auto* ring = dryDelay.getWritePointer (ch);
        const auto* in = buffer.getReadPointer (ch);
        auto* dry = dryBuffer.getWritePointer (ch);
        for (int i = 0, pos = dryDelayPos; i < n; ++i, pos = (pos + 1) & (kDryDelaySize - 1))
        {
            ring[pos] = in[i];
            dry[i] = ring[(pos - latency) & (kDryDelaySize - 1)];
        }
    }
    dryDelayPos = (dryDelayPos + n) & (kDryDelaySize - 1);
}

void ProcessorBase::applyBypass (juce::AudioBuffer<float>& buffer)
{
    bypassMix.setTargetValue (isBypassedNow() ? 1.0f : 0.0f);
    if (! bypassMix.isSmoothing() && bypassMix.getCurrentValue() <= 0.0f)
        return;

    const int chans = juce::jmin (buffer.getNumChannels(), 2);
    if (! bypassMix.isSmoothing())
    {
        // fully bypassed: hand over the dry signal itself, untouched by anything the effect does
        for (int ch = 0; ch < chans; ++ch)
            buffer.copyFrom (ch, 0, dryBuffer, ch, 0, buffer.getNumSamples());
        return;
    }

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float m = bypassMix.getNextValue();
        for (int ch = 0; ch < chans; ++ch)
        {
            auto* out = buffer.getWritePointer (ch);
            out[i] += (dryBuffer.getReadPointer (ch)[i] - out[i]) * m;
        }
    }
}

void ProcessorBase::pushScope (const juce::AudioBuffer<float>& buffer)
{
    const auto* outL = buffer.getReadPointer (0);
    const auto* outR = buffer.getReadPointer (buffer.getNumChannels() > 1 ? 1 : 0);
    const auto w = scopeFifo.write (buffer.getNumSamples());
    auto put = [&] (int startIndex, int size, int offset)
    {
        std::copy (outL + offset, outL + offset + size, scopeL.begin() + startIndex);
        std::copy (outR + offset, outR + offset + size, scopeR.begin() + startIndex);
    };
    put (w.startIndex1, w.blockSize1, 0);
    put (w.startIndex2, w.blockSize2, w.blockSize1);
}

int ProcessorBase::readScope (float* l, float* r, int maxNum)
{
    const auto rd = scopeFifo.read (juce::jmin (maxNum, scopeFifo.getNumReady()));
    auto get = [&] (int startIndex, int size, int offset)
    {
        std::copy (scopeL.begin() + startIndex, scopeL.begin() + startIndex + size, l + offset);
        std::copy (scopeR.begin() + startIndex, scopeR.begin() + startIndex + size, r + offset);
    };
    get (rd.startIndex1, rd.blockSize1, 0);
    get (rd.startIndex2, rd.blockSize2, rd.blockSize1);
    return rd.blockSize1 + rd.blockSize2;
}

void ProcessorBase::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ProcessorBase::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            migrateState (state);
            apvts.replaceState (state);
        }
}

}
