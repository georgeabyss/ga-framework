#include "OutputStage.h"

namespace ga::dsp
{

void OutputStage::prepare (double sampleRate, int channels)
{
    sr = sampleRate;
    nCh = std::max (1, std::min (2, channels));

    auto coef = [this] (double seconds) { return 1.0 - std::exp (-1.0 / (seconds * sr)); };
    mixCoef = coef (0.02);
    guardPowCoef = coef (0.05);   // Phase Protect: ~50 ms, quicker than the meter
    guardDown = coef (0.01);      // narrows quickly
    guardUp = coef (0.40);        // and widens again slowly
    powCoef = coef (prm.loudnessWindow);   // loudness window, ~300 ms by default
    powWindow = prm.loudnessWindow;
    gainCoef = coef (0.12);       // gain rises slowly (~120 ms)
    attackCoef = coef (0.008);    // and falls fast (~8 ms) so jumps never overshoot
    for (int c = 0; c < 2; ++c)
        for (auto* k : { kwIn[c], kwOut[c] }) { k[0].kShelf (sr); k[1].kHighpass (sr); }
    reset();
}

void OutputStage::reset()
{
    powIn = powOut = 0.0;
    normGain = 1.0;
    for (int c = 0; c < 2; ++c)
        for (int k = 0; k < 2; ++k) { kwIn[c][k].reset(); kwOut[c][k].reset(); }
    gmS = (double) prm.dryWet;
    guardInM = guardInS = guardDM = guardDS = guardCrossM = guardCrossS = 0.0;
    sideGain = midGain = 1.0;
    bmL.reset(); bmR.reset(); bmFreq = -1.0;
}

void OutputStage::beginBlock()
{
    if (prm.loudnessWindow != powWindow)   // the same coefficient prepare() makes
    {
        powCoef = 1.0 - std::exp (-1.0 / (prm.loudnessWindow * sr));
        powWindow = prm.loudnessWindow;
    }
    bassMonoOn = prm.bassMonoHz > 20.5f;
    if (bassMonoOn && std::fabs ((double) prm.bassMonoHz - bmFreq) > 0.01)
    {
        bmFreq = (double) prm.bassMonoHz;
        bmL.set (bmFreq, sr); bmR.set (bmFreq, sr);
    }
    if (! bassMonoOn)
        bmFreq = -1.0;
}

void OutputStage::endBlock()
{
    protectActivity.store (prm.phaseProtect && nCh > 1 ? (float) (1.0 - std::min (sideGain, midGain)) : 0.0f,
                           std::memory_order_relaxed);
}

// before bass mono and the loudness match, so the level stays put
void OutputStage::dryWet (float l, float r, float& outL, float& outR)
{
    gmS += ((double) prm.dryWet - gmS) * mixCoef;
    outL = l + (float) gmS * (outL - l);
    outR = r + (float) gmS * (outR - r);
}

// everything below the cutoff is summed to mono
void OutputStage::bassMono (float& outL, float& outR)
{
    float ll, lh, rl, rh;
    bmL.split (outL, ll, lh);
    bmR.split (outR, rl, rh);
    const float bass = 0.5f * (ll + rl);
    outL = bass + lh;
    outR = bass + rh;
}

// If the effect pushes the correlation below kGuardFloor, what the effect changed is pulled back
// until the correlation is there again: first the side signal it added, and only if that is not
// enough, the mid signal it took away (a widener can lower the mid when it pans bands outwards).
// The input itself is never touched, so where the effect changed nothing the output is the input.
void OutputStage::phaseProtect (float l, float r, float& outL, float& outR)
{
    if (! prm.phaseProtect || nCh < 2)
        return;

    const double im = 0.5 * ((double) l + r), is = 0.5 * ((double) l - r);
    const double dM = 0.5 * ((double) outL + outR) - im;   // what the effect did to the mid
    const double dS = 0.5 * ((double) outL - outR) - is;   // ... and to the side
    auto track = [this] (double& avg, double x) { avg += (x - avg) * guardPowCoef; };
    track (guardInM, im * im);  track (guardInS, is * is);
    track (guardDM, dM * dM);   track (guardDS, dS * dS);
    track (guardCrossM, im * dM);  track (guardCrossS, is * dS);

    // correlation r = (M^2 - S^2) / (M^2 + S^2), so r >= floor  <=>  S^2 <= ratio M^2 with
    // ratio = (1 - floor) / (1 + floor). The input's own width is allowed too, but the effect
    // may not take it past r = 0 (ratio 1).
    constexpr double floorRatio = (1.0 - kGuardFloor) / (1.0 + kGuardFloor);
    const double ratio = std::clamp (guardInS / (guardInM + 1.0e-12), floorRatio, 1.0);

    // powers with the effect's mid / side scaled by gm / gs
    auto midPower  = [this] (double gm) { return guardInM + 2.0 * gm * guardCrossM + gm * gm * guardDM; };
    auto sidePower = [this] (double gs) { return guardInS + 2.0 * gs * guardCrossS + gs * gs * guardDS; };
    auto fits = [&] (double gm, double gs) { return sidePower (gs) <= ratio * midPower (gm) + 1.0e-15; };

    // largest g in 0..1 for which fits holds (fits (0) is the input itself)
    auto largest = [] (auto&& ok)
    {
        if (ok (1.0)) return 1.0;
        if (! ok (0.0)) return 0.0;
        double lo = 0.0, hi = 1.0;
        for (int i = 0; i < 12; ++i)
        {
            const double mid = 0.5 * (lo + hi);
            (ok (mid) ? lo : hi) = mid;
        }
        return lo;
    };
    const double wantS = largest ([&] (double g) { return fits (1.0, g); });
    const double wantM = wantS > 0.0 ? 1.0 : largest ([&] (double g) { return fits (g, 0.0); });

    auto follow = [this] (double& gain, double want) { gain += (want - gain) * (want < gain ? guardDown : guardUp); };
    follow (sideGain, wantS);
    follow (midGain, wantM);

    // exactly 0 where the effect changed nothing
    const float deltaM = (float) ((midGain - 1.0) * dM), deltaS = (float) ((sideGain - 1.0) * dS);
    outL += deltaM + deltaS;
    outR += deltaM - deltaS;
}

// Keep the output as loud as the input, whatever the effect does to it. Both sides are
// K-weighted, so added highs (drive, lo-fi) count as louder, the way the ear hears them.
void OutputStage::loudnessMatch (float l, float r, float& outL, float& outR)
{
    auto kWeight = [] (Biquad* f, float x) { return (double) f[1].process (f[0].process (x)); };
    const float monoR = nCh > 1 ? outR : outL;   // mono: only the left channel is output
    const double kil = kWeight (kwIn[0], l), kir = kWeight (kwIn[1], r);
    const double kol = kWeight (kwOut[0], outL), kor = kWeight (kwOut[1], monoR);
    powIn  += (0.5 * (kil * kil + kir * kir) - powIn) * powCoef;
    powOut += (0.5 * (kol * kol + kor * kor) - powOut) * powCoef;
    if (powIn > 1.0e-9 && powOut > 1.0e-12)
    {
        const double want = std::min (2.0, std::max (0.08, std::sqrt (powIn / powOut)));
        normGain += (want - normGain) * (want < normGain ? attackCoef : gainCoef);
    }
    outL *= (float) normGain;
    outR *= (float) normGain;
}

} // namespace ga::dsp
