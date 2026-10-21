#pragma once
// ============================================================================
//  OutputStage - the end of every GA effect (no JUCE needed)
//
//  Per sample, the dry input (l, r) and the effect's output (outL, outR) go through:
//    dry/wet        global mix, smoothed
//    bass mono      everything below the cutoff summed to mono (Linkwitz-Riley 4th order)
//    Phase Protect  keeps the correlation the effect causes above +0.2
//    loudness match K-weighted (ITU-R BS.1770), keeps the output as loud as the input
//
//  With dry/wet at 0, or an effect that changed nothing, the output is the input.
// ============================================================================

#include <atomic>
#include "DspBlocks.h"

namespace ga::dsp
{

class OutputStage
{
public:
    struct Params
    {
        float dryWet = 1.0f;        // 0..1
        float bassMonoHz = 20.0f;   // everything below is made mono; <= 20 = off
        bool  phaseProtect = true;  // keep the correlation the effect adds above kGuardFloor
    };

    std::atomic<float> protectActivity { 0.0f };   // how far Phase Protect is pulling back right now, 0..1

    void prepare (double sampleRate, int channels);   // also resets
    void reset();
    void setParams (const Params& p) { prm = p; }

    // once per block, before the samples (picks up a new Bass Mono frequency)
    void beginBlock();
    // l / r is the dry input sample, outL / outR the effect's output, changed in place
    void process (float l, float r, float& outL, float& outR)
    {
        dryWet (l, r, outL, outR);
        if (bassMonoOn)
            bassMono (outL, outR);
        phaseProtect (l, r, outL, outR);
        loudnessMatch (l, r, outL, outR);
    }
    // once per block, after the samples (updates protectActivity)
    void endBlock();

private:
    void dryWet (float l, float r, float& outL, float& outR);
    void bassMono (float& outL, float& outR);
    void phaseProtect (float l, float r, float& outL, float& outR);
    void loudnessMatch (float l, float r, float& outL, float& outR);

    static constexpr double kGuardFloor = 0.2;   // lowest correlation Phase Protect lets the effect cause

    Params prm;
    double sr = 44100.0;
    int nCh = 2;

    double mixCoef = 0.01, gmS = 1.0;

    Crossover bmL, bmR;
    double bmFreq = -1.0;
    bool bassMonoOn = false;

    double guardPowCoef = 0.0001, guardDown = 0.001, guardUp = 0.0001;
    double guardInM = 0.0, guardInS = 0.0, guardDM = 0.0, guardDS = 0.0, guardCrossM = 0.0, guardCrossS = 0.0;
    double sideGain = 1.0, midGain = 1.0;   // how much of the effect's side / mid change is kept

    Biquad kwIn[2][2], kwOut[2][2];   // K-weighting for the loudness match, [channel][stage]
    double powCoef = 0.0001, gainCoef = 0.001, attackCoef = 0.01, powIn = 0.0, powOut = 0.0, normGain = 1.0;
};

} // namespace ga::dsp
