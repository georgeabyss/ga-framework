#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SharedIds.h"

namespace ga
{
// Appends the parameters every GA plug-in shares, in this order: Bass Mono, Dry/Wet, Bypass,
// Oversampling, Anti-Alias Filter, Phase Protect. Call it after the plug-in's own parameters,
// so hosts list them at the end. Never reorder: hosts and saved sessions depend on it.
void addSharedParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>&);
}
