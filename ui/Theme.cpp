#include "Theme.h"
#include "Style.h"

namespace ga
{
Theme Theme::withAccent (juce::Colour accent)
{
    Theme t;
    auto& c = t.colours;
    auto tint = [accent] (float saturation, float brightness) { return juce::Colour::fromHSV (accent.getHue(), saturation, brightness, 1.0f); };
    auto towardsWhite = [accent] (float amount) { return accent.interpolatedWith (juce::Colours::white, amount); };

    c.accent = accent;
    c.accentHi = towardsWhite (0.51f);
    c.accentLight = towardsWhite (0.72f);
    c.accentDeep = accent.withMultipliedSaturation (1.33f).withMultipliedBrightness (0.84f);
    c.locked = accent.withMultipliedSaturation (0.62f).withMultipliedBrightness (0.9f);

    c.lampLit = towardsWhite (0.62f);
    c.lampLitEdge = accent.withMultipliedSaturation (1.3f).withMultipliedBrightness (0.88f);
    c.ledLit = towardsWhite (0.85f);
    c.lensOff = tint (0.32f, 0.28f);
    c.lampOffEdge = tint (0.36f, 0.19f);
    c.ledOffEdge = tint (0.32f, 0.15f);

    c.displayTop = tint (0.4f, 0.04f);
    c.displayBottom = tint (0.39f, 0.09f);
    c.displayDotOff = tint (0.33f, 0.2f);
    c.displayDotOn = towardsWhite (0.85f);
    return t;
}

juce::Font Theme::font (const TextStyle& s) const
{
    const auto style = s.bold ? juce::Font::bold : juce::Font::plain;
    auto o = s.mono ? juce::FontOptions (type.mono.isEmpty() ? juce::Font::getDefaultMonospacedFontName() : type.mono, s.size, style)
                    : type.sans.isEmpty() ? juce::FontOptions (s.size, style) : juce::FontOptions (type.sans, s.size, style);
    if (s.kerning != 0.0f)
        o = o.withKerningFactor (s.kerning);
    return juce::Font (o);
}

const Theme& themeOf (const juce::Component& c)
{
    if (auto* laf = dynamic_cast<const LookAndFeel*> (&c.getLookAndFeel()))
        return laf->theme;
    static const Theme gaTheme;
    return gaTheme;
}
}
