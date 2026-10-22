#include "Header.h"

namespace ga
{
Header::Header (PresetManager& presets, juce::String t, juce::String s)
    : presetBar (presets), title (std::move (t)), subtitle (std::move (s))
{
    setInterceptsMouseClicks (false, true);   // only the preset bar
    addAndMakeVisible (presetBar);
}

void Header::setArea (juce::Rectangle<int> row)
{
    setBounds (row.expanded (kBleed));
}

void Header::setPresetBar (int height, int offsetX)
{
    presetHeight = height;
    presetOffset = offsetX;
    resized();
}

void Header::paint (juce::Graphics& g)
{
    const auto& t = themeOf (*this);
    const auto row = getLocalBounds().reduced (kBleed).toFloat();
    ui::drawTitle (g, t, title, subtitle, row.getX(), row.getY() + t.sizes.titleBaseline);
    ui::drawLogo (g, t, row.getRight(), row.getY());
}

void Header::resized()
{
    const auto row = getLocalBounds().reduced (kBleed);
    const float titleRight = (float) row.getX() + ui::titleWidth (themeOf (*this), title);
    const float logoLeft = (float) row.getRight() - ui::logoSize (themeOf (*this)).getWidth();
    const int width = PresetBar::widthForHeight (presetHeight);

    presetBar.setBounds (juce::roundToInt ((titleRight + logoLeft - (float) width) * 0.5f) + presetOffset,
                         row.getCentreY() - presetHeight / 2, width, presetHeight);
}
}
