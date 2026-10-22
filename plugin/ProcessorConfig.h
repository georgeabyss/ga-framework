#pragma once

#include <vector>

namespace ga
{
// What a GA plug-in uses of the shared processor, editor and bottom strip, and their limits. The
// defaults are Hyper! Hyper! Dimension. A feature that is off has no parameter and is not in the
// bottom strip. Never switch a feature off after a release: its parameter would vanish from sessions.
struct ProcessorConfig
{
    // features
    bool oversampling = true;    // the oversampling and anti-alias filter parameters and menus
    bool bassMono = true;
    bool phaseProtect = true;
    bool loudnessMatch = true;   // the output stage keeps the output as loud as the input
    bool scope = true;           // the goniometer: its button in the bottom strip and its side panel
    bool dryWetLocks = true;     // Dry/Wet and Bass Mono can be locked against preset changes
    bool midiInput = false;      // must match NEEDS_MIDI_INPUT in ga_add_plugin()

    // limits
    float safetyLevel = 15.85f;                         // linear peak (+24 dBFS); above it, or NaN, the output mutes
    std::vector<int> oversamplingFactors { 1, 2, 4, 8 };   // powers of two, 1 = off; at most 8
    int defaultOversampling = 2;
    float defaultScale = 1.0f;                           // screen pixels per layout pixel at 100 %
    std::vector<float> windowSizes { 0.75f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f };   // the size menu (relative to 100 %), without sizes larger than the screen
    float minScale = 0.6f, maxScale = 2.5f;              // dragging the corner, relative to 100 %
};
}
