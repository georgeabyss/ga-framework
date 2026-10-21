#include "SharedParams.h"

namespace ga
{
void addSharedParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& p)
{
    using namespace juce;

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { pid::bassMono, 1 }, "Bass Mono",
                                                        NormalisableRange<float> (20.f, 400.f, 1.f, 0.5f), 20.f,
                                                        AudioParameterFloatAttributes()
                                                            .withStringFromValueFunction ([] (float v, int) { return v <= 20.5f ? String ("Off") : String ((int) v) + " Hz"; })
                                                            .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue() > 20.5f ? t.getFloatValue() : 20.0f; })));
    // 0..100 %, shown as a whole number
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { pid::dryWet, 1 }, "Dry/Wet", NormalisableRange<float> (0.f, 100.f, 0.1f), 100.f,
                                                        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                                                        {
                                                            return String (roundToInt (v)) + " %";
                                                        })));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { pid::bypass, 1 }, "Bypass", false));
    // not automatable: these change the latency
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { pid::osFactor, 1 }, "Oversampling", StringArray { "Off", "2x", "4x", "8x" }, 1,
                                                         AudioParameterChoiceAttributes().withAutomatable (false)));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { pid::osFilter, 1 }, "Anti-Alias Filter", StringArray { "Linear Phase", "Low Latency" }, 0,
                                                         AudioParameterChoiceAttributes().withAutomatable (false)));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { pid::phaseProtect, 1 }, "Phase Protect", true));
}
}
