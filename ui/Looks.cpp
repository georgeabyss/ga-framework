#include "Looks.h"
#include "Style.h"

namespace ga
{
namespace
{
    // a colour with each of r, g, b raised by lift (the knobs while hovered)
    juce::Colour lifted (juce::Colour c, int lift)
    {
        return juce::Colour ((juce::uint8) (c.getRed() + lift), (juce::uint8) (c.getGreen() + lift), (juce::uint8) (c.getBlue() + lift));
    }

    // classic 5 x 7 LED matrix font: one byte per row, bit 4 = leftmost dot
    const std::array<juce::uint8, 7>& glyph (juce::juce_wchar c)
    {
        static const std::map<juce::juce_wchar, std::array<juce::uint8, 7>> font {
            { 'A', { 0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 } },
            { 'B', { 0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110 } },
            { 'C', { 0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110 } },
            { 'D', { 0b11100, 0b10010, 0b10001, 0b10001, 0b10001, 0b10010, 0b11100 } },
            { 'E', { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111 } },
            { 'F', { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000 } },
            { 'G', { 0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111 } },
            { 'H', { 0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 } },
            { 'I', { 0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 } },
            { 'J', { 0b00111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100 } },
            { 'K', { 0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001 } },
            { 'L', { 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111 } },
            { 'M', { 0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001 } },
            { 'N', { 0b10001, 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001 } },
            { 'O', { 0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 } },
            { 'P', { 0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000 } },
            { 'Q', { 0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101 } },
            { 'R', { 0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001 } },
            { 'S', { 0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110 } },
            { 'T', { 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 } },
            { 'U', { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 } },
            { 'V', { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100 } },
            { 'W', { 0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010 } },
            { 'X', { 0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001 } },
            { 'Y', { 0b10001, 0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100 } },
            { 'Z', { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111 } },
            { '0', { 0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110 } },
            { '1', { 0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 } },
            { '2', { 0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111 } },
            { '3', { 0b11111, 0b00010, 0b00100, 0b00010, 0b00001, 0b10001, 0b01110 } },
            { '4', { 0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010 } },
            { '5', { 0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110 } },
            { '6', { 0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110 } },
            { '7', { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000 } },
            { '8', { 0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110 } },
            { '9', { 0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100 } },
            { '*', { 0b00000, 0b00100, 0b10101, 0b01110, 0b10101, 0b00100, 0b00000 } },
            { '-', { 0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000 } },
            { '!', { 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b00100 } },
            { '\'', { 0b00100, 0b00100, 0b01000, 0b00000, 0b00000, 0b00000, 0b00000 } },
            { '.', { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b01100, 0b01100 } },
            { '&', { 0b01100, 0b10010, 0b10100, 0b01000, 0b10101, 0b10010, 0b01101 } },
            { '?', { 0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b00000, 0b00100 } },
        };
        static const std::array<juce::uint8, 7> blank {};
        const auto it = font.find (c);
        return it != font.end() ? it->second : blank;
    }
}

//==============================================================================
void PlainText::paint (juce::Graphics& g, const juce::Path& text, juce::Rectangle<float> bounds, const Theme& t) const
{
    juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 5, { 0, 2 }).drawForPath (g, text);
    juce::ColourGradient tg (t.colours.accentLight, 0.0f, bounds.getY(), t.colours.accentDeep, 0.0f, bounds.getBottom(), false);
    tg.addColour (0.55, t.colours.accent);
    g.setGradientFill (tg);
    g.fillPath (text);
}

void EchoSmear::paint (juce::Graphics& g, const juce::Path& text, juce::Rectangle<float> bounds, const Theme& t) const
{
    for (const auto& e : echoes)
    {
        g.setColour (t.colours.accentHi.withAlpha (e.opacity));
        g.fillPath (text, juce::AffineTransform::translation (e.dx, e.dy));
    }
    PlainText().paint (g, text, bounds, t);
}

//==============================================================================
void GradientFaceplate::paint (juce::Graphics& g, juce::Rectangle<float> b, const Theme& t) const
{
    g.setGradientFill (juce::ColourGradient (t.colours.faceplateTop, 0.0f, b.getY(), t.colours.faceplateBottom, 0.0f, b.getBottom(), false));
    g.fillRect (b);
    g.setColour (t.colours.faceplateEdge.withAlpha (0.7f));
    g.drawLine (b.getX(), b.getY() + 1.5f, b.getRight(), b.getY() + 1.5f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawLine (b.getX(), b.getBottom() - 1.5f, b.getRight(), b.getBottom() - 1.5f, 1.0f);
}

//==============================================================================
void LedMatrixDisplay::paint (juce::Graphics& g, juce::Rectangle<float> r, const State& state, const Theme& t) const
{
    // dark glass
    const auto& c = t.colours;
    g.setGradientFill (juce::ColourGradient (c.displayTop, 0.0f, r.getY(), c.displayBottom, 0.0f, r.getBottom(), false));
    g.fillRect (r);

    // which cells are lit: "NN NAME" from the left with the star in the last slot, or the
    // safety mute warning in the middle of the grid
    std::vector<bool> lit ((size_t) (kCols * kRows), false);
    auto put = [&lit] (juce::juce_wchar ch, int firstCol)
    {
        const auto& rows = glyph (ch);
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if ((rows[(size_t) row] >> (4 - col)) & 1)
                    lit[(size_t) ((kTextRow + row) * kCols + firstCol + col)] = true;
    };
    if (state.muted)
    {
        const juce::String warning ("! OUTPUT MUTED !");
        const int firstCol = (kCols - (warning.length() * 6 - 1)) / 2;
        if (state.blinkOn)
            for (int i = 0; i < warning.length(); ++i)
                put (warning[i], firstCol + i * 6);
    }
    else
    {
        for (int i = 0; i < state.text.length() && i < kSlots - 1; ++i)
            put (state.text[i], kTextCol + i * 6);
        if (state.modified)
            put ('*', kTextCol + (kSlots - 1) * 6);
    }

    // the grid, centred in the glass: every cell is a square LED, lit or not
    const float gridW = (float) (kCols - 1) * kPitch + kDot, gridH = (float) (kRows - 1) * kPitch + kDot;
    const float x0 = r.getX() + (r.getWidth() - gridW) * 0.5f, y0 = r.getY() + (r.getHeight() - gridH) * 0.5f;
    juce::Path off, on;
    for (int row = 0; row < kRows; ++row)
        for (int col = 0; col < kCols; ++col)
            (lit[(size_t) (row * kCols + col)] ? on : off).addRectangle (x0 + (float) col * kPitch, y0 + (float) row * kPitch, kDot, kDot);

    g.setColour (c.displayDotOff);
    g.fillPath (off);

    // glow: the lit dots blurred outwards in two soft passes, then the dots themselves
    const auto glow = state.muted ? c.displayMuteGlow : c.accent;   // safety mute: orange-red
    for (auto pass : { std::pair<float, float> { 2.6f, 0.10f }, { 1.4f, 0.22f } })
    {
        g.setColour (glow.withAlpha (pass.second));
        g.strokePath (on, juce::PathStrokeType (pass.first, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (state.muted ? c.displayMuteDot : c.displayDotOn);
    g.fillPath (on);

    // reflection on the glass and the frame (lavender while the list is open)
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f), 0.0f, r.getY(),
                                             juce::Colours::white.withAlpha (0.0f), 0.0f, r.getCentreY(), false));
    g.fillRect (r.withHeight (r.getHeight() * 0.45f));
    g.setColour (state.muted ? c.displayMuteEdge.withAlpha (0.8f) : state.menuOpen ? c.accent.withAlpha (0.7f) : juce::Colours::black);
    g.drawRect (r, 1.0f);
}

//==============================================================================
// Knob look taken from the Knob-Tuner, radius 14 by default. pos is 0..1.
void GaKnobFace::paint (juce::Graphics& g, const Theme& t, juce::Point<float> c, float pos, bool hot) const
{
    const float R = t.sizes.knobRadius, ri = R - 2.6f;
    const float startAngle = juce::degreesToRadians (225.0f), endAngle = juce::degreesToRadians (495.0f);

    // three position dots: start, centre, end
    {
        const float mid = 0.5f * (startAngle + endAngle);
        const std::pair<float, juce::Colour> dots[] = { { startAngle, t.colours.markDim },
                                                         { mid,        t.colours.mark },
                                                         { endAngle,   t.colours.markDim } };
        for (auto& d : dots)
        {
            const auto p = ui::polar (c, R + 5.5f, d.first);
            g.setColour (d.second);
            g.fillEllipse (p.x - 1.3f, p.y - 1.3f, 2.6f, 2.6f);
        }
    }

    // soft shadow
    {
        juce::Path sp;
        sp.addEllipse (c.x - R - 1.2f, c.y + 1.6f - R - 1.2f, (R + 1.2f) * 2.0f, (R + 1.2f) * 2.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.75f), 5, { 0, 0 }).drawForPath (g, sp);
    }

    // outer ring, linear fade from top to bottom (a little lighter while hovered / dragged)
    const int lift = hot ? t.sizes.knobHoverLift : 0;
    g.setGradientFill (juce::ColourGradient (lifted (t.colours.knobRingTop, lift), c.x, c.y - R,
                                             lifted (t.colours.knobRingBottom, lift), c.x, c.y + R, false));
    g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    g.setColour (juce::Colours::black);
    g.drawEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.drawEllipse (c.x - R + 0.7f, c.y - R + 0.7f, (R - 0.7f) * 2.0f, (R - 0.7f) * 2.0f, 1.0f);

    // darker core
    g.setGradientFill (juce::ColourGradient (t.colours.knobCoreTop, c.x, c.y - ri,
                                             t.colours.knobCoreBottom, c.x, c.y + ri, false));
    g.fillEllipse (c.x - ri, c.y - ri, ri * 2.0f, ri * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (c.x - ri, c.y - ri, ri * 2.0f, ri * 2.0f, 1.0f);

    // pointer from the rim inwards
    const float a = startAngle + pos * (endAngle - startAngle);
    const float r1 = R - 1.3f, r0 = r1 - 6.5f;
    juce::Path line;
    line.startNewSubPath (ui::polar (c, r0, a));
    line.lineTo (ui::polar (c, r1, a));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.strokePath (line, juce::PathStrokeType (3.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                  juce::AffineTransform::translation (0.5f, 0.7f));
    g.setColour (hot ? t.colours.accentHi : t.colours.accent);
    g.strokePath (line, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// small dial for the bottom strip: radius 7 by default, with a value arc around it
void GaKnobFace::paintMini (juce::Graphics& g, const Theme& t, juce::Point<float> c, float pos, bool hot) const
{
    const float R = t.sizes.miniKnobRadius, ra = R + 2.6f;
    const float startAngle = juce::degreesToRadians (225.0f), endAngle = juce::degreesToRadians (495.0f);
    const float a = startAngle + pos * (endAngle - startAngle);
    const juce::PathStrokeType arcStroke (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track, value;
    track.addCentredArc (c.x, c.y, ra, ra, 0.0f, startAngle, endAngle, true);
    g.setColour (t.colours.track);
    g.strokePath (track, arcStroke);
    if (pos > 0.002f)
    {
        value.addCentredArc (c.x, c.y, ra, ra, 0.0f, startAngle, a, true);
        g.setColour (hot ? t.colours.accentHi : t.colours.accent.withAlpha (0.85f));
        g.strokePath (value, arcStroke);
    }

    const int lift = hot ? t.sizes.knobHoverLift : 0;
    g.setGradientFill (juce::ColourGradient (lifted (t.colours.knobRingTop, lift), c.x, c.y - R,
                                             lifted (t.colours.miniRingBottom, lift), c.x, c.y + R, false));
    g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    g.setColour (juce::Colours::black);
    g.drawEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f, 1.0f);

    juce::Path line;
    line.startNewSubPath (ui::polar (c, 2.0f, a));
    line.lineTo (ui::polar (c, R - 1.5f, a));
    g.setColour (hot ? t.colours.accentHi : t.colours.accent);
    g.strokePath (line, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
}
