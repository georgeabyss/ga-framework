#pragma once

#include "presets/PresetBar.h"

namespace ga
{
// The title row every GA plug-in shares: the title (with its subtitle) on the row's left edge, the
// gothic GA on its right edge and the preset bar centred between the two. The plug-in gives it its
// row with setArea() and sets the preset bar's height and nudge with setPresetBar().
class Header : public juce::Component
{
public:
    Header (PresetManager&, juce::String title, juce::String subtitle);

    // the row: the title starts at its left edge, the logo ends at its right edge, both at its top.
    // Use this, not setBounds(): the title's echo and the shadows reach a little beyond the row.
    void setArea (juce::Rectangle<int> row);

    // the preset bar's height (its width follows from its proportions), and how far it is moved to the
    // right (negative: left) from the centre between title and logo, when that looks more balanced
    void setPresetBar (int height, int offsetX);

    // controls right of the preset bar (e.g. a dice), centred together with it; they keep their size
    void setPresetExtras (std::vector<juce::Component*>);

    void paint (juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override { resized(); }   // the theme may have changed

    PresetBar presetBar;

private:
    static constexpr int kBleed = 16;           // room for the echo and the shadows around the row

    juce::String title, subtitle;
    std::vector<juce::Component*> extras;
    static constexpr int kExtraGap = 6;
    int presetHeight = PresetBar::kHeight, presetOffset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Header)
};
}
