#include "Style.h"

namespace ga::ui
{
    juce::Font font (float size, bool bold, float kerning)
    {
        auto o = juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain);
        if (kerning != 0.0f) o = o.withKerningFactor (kerning);
        return juce::Font (o);
    }

    juce::Font monoFont (float size, bool bold)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size,
                                              bold ? juce::Font::bold : juce::Font::plain));
    }

    namespace
    {
        const juce::Path& logoPath()
        {
            static const juce::Path gothicGA = makeGothicGA (0.75f, 6.2f, 38.0f);   // built once
            return gothicGA;
        }
    }

    juce::Point<float> polar (juce::Point<float> c, float r, float angle)
    {
        return { c.x + std::sin (angle) * r, c.y - std::cos (angle) * r };
    }

    // recessed dark square used by the step box and the toggles
    void drawRecess (juce::Graphics& g, const Theme& t, juce::Point<float> c, float size)
    {
        drawRecess (g, t, juce::Rectangle<float> (size, size).withCentre (c));
    }
    void drawRecess (juce::Graphics& g, const Theme& t, juce::Rectangle<float> r)
    {
        g.setGradientFill (juce::ColourGradient (t.colours.recessTop, 0.0f, r.getY(), t.colours.recessBottom, 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, t.sizes.recessCorner);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r, t.sizes.recessCorner, 1.0f);
        g.setColour (t.colours.recessEdge);
        g.drawLine (r.getX() + 1.5f, r.getBottom() + 0.5f, r.getRight() - 1.5f, r.getBottom() + 0.5f, 1.0f);
    }

    // Gothic "GA", drawn with a flat pen held at an angle (own letterforms, no font needed).
    juce::Path makeGothicGA (float scale, float nib, float angleDeg)
    {
        using P = juce::Point<float>;
        using Stroke = std::vector<P>;

        const std::vector<Stroke> glyphG {
            { { 23, 8 }, { 18, 4.5f }, { 12, 5 }, { 8, 10 }, { 6.5f, 17 }, { 8, 24 }, { 13, 29.5f }, { 19, 30 }, { 24, 25.5f } },
            { { 24, 25.5f }, { 24, 17.5f } },
            { { 14.5f, 17.5f }, { 25, 17.5f } },
            { { 19, 3 }, { 25, 7 } } };
        const std::vector<Stroke> glyphA {
            { { 19, 3.5f }, { 12, 8 }, { 7.5f, 15 }, { 8, 22.5f }, { 12, 26.5f }, { 18, 24.5f } },
            { { 24, 6 }, { 24, 29 }, { 27.5f, 31 } },
            { { 19, 3.5f }, { 24, 6 } },
            { { 11, 16.5f }, { 24, 16.5f } } };

        auto catmullRom = [] (const Stroke& s, int steps)
        {
            Stroke P;
            P.push_back (s.front());
            P.insert (P.end(), s.begin(), s.end());
            P.push_back (s.back());
            Stroke out;
            for (size_t i = 1; i + 2 < P.size(); ++i)
                for (int k = 0; k < steps; ++k)
                {
                    const float t = (float) k / (float) steps, t2 = t * t, t3 = t2 * t;
                    auto f = [&] (float p0, float p1, float p2, float p3)
                    {
                        return 0.5f * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2 + (-p0 + 3 * p1 - 3 * p2 + p3) * t3);
                    };
                    out.push_back ({ f (P[i - 1].x, P[i].x, P[i + 1].x, P[i + 2].x),
                                     f (P[i - 1].y, P[i].y, P[i + 1].y, P[i + 2].y) });
                }
            out.push_back (s.back());
            return out;
        };

        const float a = juce::degreesToRadians (angleDeg);
        const float nx = std::cos (a) * nib * 0.5f, ny = -std::sin (a) * nib * 0.5f;

        juce::Path path;
        auto addGlyph = [&] (const std::vector<Stroke>& strokes, float dx)
        {
            for (const auto& s : strokes)
            {
                Stroke pts;
                if (s.size() > 2) pts = catmullRom (s, 10);
                else for (int i = 0; i <= 8; ++i) pts.push_back (s[0] + (s[1] - s[0]) * ((float) i / 8.0f));

                for (size_t i = 0; i + 1 < pts.size(); ++i)
                {
                    P q[4] = { { pts[i].x - nx, pts[i].y - ny }, { pts[i].x + nx, pts[i].y + ny },
                               { pts[i + 1].x + nx, pts[i + 1].y + ny }, { pts[i + 1].x - nx, pts[i + 1].y - ny } };
                    float area = 0.0f;
                    for (int k = 0; k < 4; ++k)
                        area += q[k].x * q[(k + 1) % 4].y - q[(k + 1) % 4].x * q[k].y;
                    if (area < 0.0f) { std::swap (q[0], q[3]); std::swap (q[1], q[2]); }

                    path.startNewSubPath ((q[0].x + dx) * scale, q[0].y * scale);
                    for (int k = 1; k < 4; ++k)
                        path.lineTo ((q[k].x + dx) * scale, q[k].y * scale);
                    path.closeSubPath();
                }
            }
        };
        addGlyph (glyphG, 0.0f);
        addGlyph (glyphA, 30.0f);
        return path;
    }

    void drawScrew (juce::Graphics& g, const Theme& t, float cx, float cy)
    {
        // rack slot
        const auto slot = juce::Rectangle<float> (11.0f, 17.0f).withCentre ({ cx, cy });
        g.setColour (t.colours.hole);
        g.fillRoundedRectangle (slot, 5.5f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (slot, 5.5f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (slot.reduced (0.5f), 5.0f, 1.0f);

        // screw head with Phillips cross
        const float r = 4.2f;
        g.setGradientFill (juce::ColourGradient (t.colours.screwLight, cx - r * 0.3f, cy - r * 0.4f,
                                                 t.colours.screwDark, cx + r * 0.9f, cy + r * 0.9f, true));
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colours::black);
        g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);
        g.setColour (t.colours.screwCross);
        g.drawLine (cx - 2.5f, cy, cx + 2.5f, cy, 1.2f);
        g.drawLine (cx, cy - 2.5f, cx, cy + 2.5f, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.drawLine (cx - 2.5f, cy + 0.8f, cx + 2.5f, cy + 0.8f, 0.6f);
    }


    // darker inset panel that sets the bottom strip and the goniometer apart from the rest
    void drawInsetPanel (juce::Graphics& g, const Theme& t, juce::Rectangle<float> r)
    {
        const float s = t.sizes.insetCorner / Theme::Sizes{}.insetCorner;   // today's radii times s
        g.setGradientFill (juce::ColourGradient (t.colours.insetTop, 0.0f, r.getY(), t.colours.insetBottom, 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 5.0f * s);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r, 5.0f * s, 1.0f);
        g.setColour (t.colours.accent.withAlpha (0.18f));
        g.drawRoundedRectangle (r.reduced (1.0f), 4.0f * s, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawLine (r.getX() + 5.0f * s, r.getBottom() + 0.5f, r.getRight() - 5.0f * s, r.getBottom() + 0.5f, 1.0f);
    }


    // round LED: lit while the section is active (its amount above 0)
    void drawLed (juce::Graphics& g, const Theme& t, juce::Point<float> c, bool on, float alarm)
    {
        const float s = t.sizes.ledRadius / Theme::Sizes{}.ledRadius;   // today's sizes times s
        const float r = 3.0f * s, gr = 6.5f * s;
        auto mix = [alarm] (juce::Colour calm, juce::Colour hot) { return calm.interpolatedWith (hot, alarm); };
        const auto glow = mix (t.colours.accent, t.colours.alarm);
        if (on)
        {
            g.setGradientFill (juce::ColourGradient (glow.withAlpha (0.45f + 0.2f * alarm), c.x, c.y, glow.withAlpha (0.0f), c.x + gr, c.y, true));
            g.fillEllipse (c.x - gr, c.y - gr, gr * 2.0f, gr * 2.0f);
        }
        juce::ColourGradient lens (on ? mix (t.colours.ledLit, t.colours.alarmLight) : t.colours.lensOff, c.x - 0.6f * s, c.y - 0.9f * s,
                                   on ? mix (t.colours.accentDeep, t.colours.alarmDeep) : t.colours.ledOffEdge, c.x + r, c.y + r, true);
        if (on) lens.addColour (0.55, glow);
        g.setGradientFill (lens);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colours::black);
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.0f);
    }

    void drawName (juce::Graphics& g, const Theme& t, float cx, float cy, const juce::String& name)
    {
        g.setColour (t.colours.label);
        g.setFont (t.font (t.type.name));
        g.drawText (name.toUpperCase(), juce::Rectangle<float> (60.0f, 10.0f).withCentre ({ cx, cy + 43.0f - 3.0f }),
                    juce::Justification::centred, false);
    }

    void drawPadlock (juce::Graphics& g, float x, float y, bool open, juce::Colour colour)
    {
        juce::Path shackle;
        shackle.startNewSubPath (x + 1.25f, y + 2.8f);
        shackle.lineTo (x + 1.25f, y + 1.9f);
        shackle.addCentredArc (x + 2.5f, y + 1.9f, 1.25f, 1.25f, 0.0f, -juce::MathConstants<float>::halfPi,
                               juce::MathConstants<float>::halfPi, false);
        if (! open)
            shackle.lineTo (x + 3.75f, y + 2.8f);
        g.setColour (colour);
        g.strokePath (shackle, juce::PathStrokeType (0.8f));
        g.fillRoundedRectangle (x + 0.5f, y + 2.75f, 4.0f, 3.0f, 0.5f);
    }

    void drawHoverWash (juce::Graphics& g, const Theme& t, juce::Rectangle<float> r)
    {
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.fillRoundedRectangle (r, t.sizes.recessCorner);
    }

    void drawDropDownMark (juce::Graphics& g, float x, float y, juce::Colour colour)
    {
        juce::Path tri;
        tri.addTriangle (x - 2.4f, y - 1.2f, x + 2.4f, y - 1.2f, x, y + 1.8f);
        g.setColour (colour);
        g.fillPath (tri);
    }

    void drawTitle (juce::Graphics& g, const Theme& t, const juce::String& title, const juce::String& subtitle, float x, float baseline)
    {
        // the title in the theme's effect; its line runs from the top of the capitals (3/4 of the
        // size above the baseline) to a little below the baseline
        {
            juce::GlyphArrangement ga;
            ga.addLineOfText (t.font (t.type.title), title, x, baseline);
            juce::Path p;
            ga.createPath (p);
            const float size = t.type.title.size;
            const juce::Rectangle<float> line (x, baseline - size * 0.75f, p.getBounds().getRight() - x, size * 0.75f + size / 24.0f);
            t.looks.title->paint (g, p, line, t);
        }
        g.setColour (t.colours.subtitle);
        g.setFont (t.font (t.type.subtitle));
        g.drawText (subtitle, juce::roundToInt (x) + 2, juce::roundToInt (baseline) + 4, 200, 10, juce::Justification::centredLeft, false);
    }

    float titleWidth (const Theme& t, const juce::String& title)
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (t.font (t.type.title), title, 0.0f, 0.0f);
        return ga.getBoundingBox (0, -1, true).getRight();
    }

    juce::Rectangle<float> logoSize()
    {
        return logoPath().getBounds().withZeroOrigin();
    }

    void drawLogo (juce::Graphics& g, const Theme& t, float right, float top)
    {
        auto ga = logoPath();
        const auto b = ga.getBounds();
        const float ox = right - b.getRight(), oy = top - b.getY();
        ga.applyTransform (juce::AffineTransform::translation (ox, oy));
        juce::DropShadow (juce::Colours::black.withAlpha (0.75f), 3, { 0, 1 }).drawForPath (g, ga);
        juce::ColourGradient sg (t.colours.logoTop, 0.0f, oy + 2.0f, t.colours.logoBottom, 0.0f, oy + 42.0f, false);
        sg.addColour (0.45, t.colours.logoUpper);
        sg.addColour (0.55, t.colours.logoLower);
        g.setGradientFill (sg);
        g.fillPath (ga);
    }
}

//==============================================================================
namespace ga
{
LookAndFeel::LookAndFeel (Theme t)
    : theme (std::move (t))
{
    const auto& c = theme.colours;
    setColour (juce::PopupMenu::backgroundColourId,            c.popup);
    setColour (juce::PopupMenu::textColourId,                  c.value);
    setColour (juce::PopupMenu::headerTextColourId,            c.label);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, c.accent);
    setColour (juce::PopupMenu::highlightedTextColourId,       c.onAccent);
    setColour (juce::TooltipWindow::backgroundColourId,        c.popup);
    setColour (juce::TooltipWindow::textColourId,              c.text);
    setColour (juce::TooltipWindow::outlineColourId,           c.accent.withAlpha (0.4f));

    // preset dialogs (name, delete)
    setColour (juce::AlertWindow::backgroundColourId,          c.popup);
    setColour (juce::AlertWindow::textColourId,                c.text);
    setColour (juce::AlertWindow::outlineColourId,             c.accent.withAlpha (0.35f));
    setColour (juce::TextButton::buttonColourId,               c.button);
    setColour (juce::TextButton::buttonOnColourId,             c.accent);
    setColour (juce::TextButton::textColourOffId,              c.text);
    setColour (juce::TextEditor::backgroundColourId,           c.recessTop);
    setColour (juce::TextEditor::textColourId,                 c.text);
    setColour (juce::TextEditor::highlightColourId,            c.accent.withAlpha (0.45f));
    setColour (juce::TextEditor::outlineColourId,              c.track);
    setColour (juce::TextEditor::focusedOutlineColourId,       c.accent);
    setColour (juce::CaretComponent::caretColourId,            c.accentHi);
}

juce::Font LookAndFeel::getPopupMenuFont()
{
    return theme.font (theme.type.menu);
}

void LookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    const auto r = juce::Rectangle<int> (width, height).toFloat();
    g.setGradientFill (juce::ColourGradient (theme.colours.faceplateTop, 0.0f, 0.0f, theme.colours.popupBottom, 0.0f, r.getBottom(), false));
    g.fillRect (r);
    g.setColour (theme.colours.accent.withAlpha (0.35f));
    g.drawRect (r, 1.0f);
}

void LookAndFeel::drawCornerResizer (juce::Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging)
{
    const float thickness = juce::jmin ((float) w, (float) h) * 0.08f;
    g.setColour (theme.colours.accent.withAlpha (isMouseOver || isMouseDragging ? 1.0f : 0.7f));
    for (float i = 0.0f; i < 1.0f; i += 0.3f)
        g.drawLine ((float) w * i, (float) h + 1.0f, (float) w + 1.0f, (float) h * i, thickness);
}

void LookAndFeel::drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& name)
{
    g.setFont (theme.font (theme.type.menuHeader));
    g.setColour (theme.colours.label);
    g.drawText (name.toUpperCase(), area.reduced (10, 0).withTrimmedTop (2), juce::Justification::centredLeft, true);
}
}
