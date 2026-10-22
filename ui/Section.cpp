#include "Section.h"

namespace ga
{
ColumnGrid::ColumnGrid (juce::Rectangle<int> a, int columns, int g, std::vector<int> s)
    : area (a), gap (g), spans (std::move (s))
{
    jassert (std::accumulate (spans.begin(), spans.end(), 0) == columns);
    column = (area.getWidth() - gap * ((int) spans.size() - 1)) / columns;
}

juce::Rectangle<int> ColumnGrid::section (int index) const
{
    int x = area.getX();
    for (int i = 0; i < index; ++i)
        x += spans[(size_t) i] * column + gap;
    return { x, area.getY(), spans[(size_t) index] * column, area.getHeight() };
}

//==============================================================================
Section::Section (juce::String t, int c)
    : title (std::move (t)), columns (juce::jmax (1, c)), cells ((size_t) columns)
{
    setInterceptsMouseClicks (false, true);   // only the controls
    // it draws only inside its bounds; unclipped, the rules' antialiased ends stay whole at any scale
    setPaintingIsUnclipped (true);
}

void Section::setHeaderControls (std::vector<juce::Component*> leftToRight)
{
    header = std::move (leftToRight);
    for (auto* c : header)
        addAndMakeVisible (c);
    resized();
}

void Section::setControl (int column, juce::Component& c, juce::String name)
{
    jassert (column >= 0 && column < columns);
    cells[(size_t) column] = { &c, std::move (name) };
    addAndMakeVisible (c);
    resized();
}

void Section::setDimmed (bool h, bool b)
{
    const float dim = themeOf (*this).sizes.dimAlpha;
    for (auto* c : header)
        c->setAlpha (h ? dim : 1.0f);
    for (auto& cell : cells)
        if (cell.control != nullptr)
            cell.control->setAlpha (b ? dim : 1.0f);
    if (h != headerDimmed || b != bodyDimmed)
    {
        headerDimmed = h;
        bodyDimmed = b;
        repaint();
    }
}

float Section::columnCentre (int column) const
{
    return (float) getWidth() / (float) columns * ((float) column + 0.5f);
}

void Section::paint (juce::Graphics& g)
{
    const auto& t = themeOf (*this);
    const auto& s = t.sizes;
    const int w = getWidth();

    // header: the title and the rule under it. Dimmed, each part is drawn into a transparency
    // layer: setColour() would undo a plain setOpacity().
    auto dimmed = [&g, &s] (bool dim, auto&& draw)
    {
        if (dim) g.beginTransparencyLayer (s.dimAlpha);
        draw();
        if (dim) g.endTransparencyLayer();
    };
    dimmed (headerDimmed, [&]
    {
        g.setFont (t.font (t.type.sectionTitle));
        g.setColour (t.colours.sectionTitle);
        const int centre = juce::roundToInt (s.sectionHeaderCentre);
        g.drawText (title, juce::roundToInt (s.sectionTitleInset), centre - 5, 120, 10, juce::Justification::centredLeft, false);
        const int rule = juce::roundToInt (s.sectionRule);
        g.setColour (juce::Colours::black);
        g.drawHorizontalLine (rule, 0.0f, (float) w);
        g.setColour (t.colours.track);
        g.drawHorizontalLine (rule + 1, 0.0f, (float) w);
    });

    // the names of the controls that do not draw their own
    dimmed (bodyDimmed, [&]
    {
        for (int i = 0; i < columns; ++i)
            if (cells[(size_t) i].name.isNotEmpty())
                ui::drawName (g, t, (float) juce::roundToInt (columnCentre (i)), s.sectionBodyCentre, cells[(size_t) i].name);
    });
}

void Section::resized()
{
    const auto& s = themeOf (*this).sizes;

    // header controls from the right edge, centred on the header line
    int right = getWidth() - juce::roundToInt (s.sectionControlInset);
    const int centre = juce::roundToInt (s.sectionHeaderCentre);
    for (auto it = header.rbegin(); it != header.rend(); ++it)
    {
        auto* c = *it;
        c->setTopLeftPosition (right - c->getWidth(), centre - c->getHeight() / 2);
        right -= c->getWidth() + juce::roundToInt (s.sectionControlGap);
    }

    // one control per column, its anchor (a knob's dial) on the centre line
    const int cy = juce::roundToInt (s.sectionBodyCentre);
    for (int i = 0; i < columns; ++i)
        if (auto* c = cells[(size_t) i].control)
        {
            const auto anchor = anchorOf (*c);
            c->setTopLeftPosition (juce::roundToInt (columnCentre (i)) - anchor.x, cy - anchor.y);
        }
}
}
