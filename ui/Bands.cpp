#include "Bands.h"

namespace ga
{
namespace
{
    constexpr int kPad = 4;          // inside the card's edge
    constexpr int kTopRow = 18;      // LED, name, info, solo
    constexpr int kRangeRow = 10;
    constexpr float kCorner = 3.0f;
}

//==============================================================================
BandCard::BandCard()
{
    setSize (180 + 2 * glowPadding, 80 + 2 * glowPadding);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void BandCard::setControls (juce::Component* l, juce::Component* s, juce::Component* d)
{
    led = l;
    solo = s;
    dropdown = d;
    for (auto* c : { led, solo, dropdown })
        if (c != nullptr)
            addAndMakeVisible (c);
    resized();
}

void BandCard::setName (const juce::String& n)                 { name = n; repaint(); }
void BandCard::setSelected (bool s)                            { if (s != selected) { selected = s; repaint(); } }
void BandCard::setActive (bool a)                              { if (a != active) { active = a; repaint(); } }

void BandCard::setInfo (const juce::String& i, const juce::String& r, const juce::String& s)
{
    info = i;
    range = r;
    status = s;
    repaint();
}

void BandCard::setTraces (Trace m, Trace s)
{
    main = std::move (m);
    second = std::move (s);
    repaint (traceArea() + face().getPosition());
}

juce::Rectangle<int> BandCard::traceArea() const   // in the face
{
    const auto f = face();
    const int top = kPad + kTopRow + kRangeRow + 3;
    const int bottom = f.getHeight() - kPad - (dropdown != nullptr ? dropdown->getHeight() : 0) - 3;
    return { kPad + 2, top, f.getWidth() - 2 * kPad - 4, juce::jmax (0, bottom - top) };
}

void BandCard::paint (juce::Graphics& g)
{
    const auto& t = themeOf (*this);
    const auto& c = t.colours;
    const float dim = active ? 1.0f : t.sizes.dimAlpha;

    // selected: a glow in the padding around the face
    if (selected)
        juce::DropShadow (c.accent.withAlpha (0.25f), glowPadding, {}).drawForRectangle (g, face());

    // from here on, in the face
    const auto f = face();
    g.setOrigin (f.getPosition());
    const int width = f.getWidth(), height = f.getHeight();
    const auto b = juce::Rectangle<float> ((float) width, (float) height).reduced (0.5f);

    g.setColour (selected ? c.cardSelected : c.card);
    g.fillRoundedRectangle (b, kCorner);
    g.setColour (selected ? c.accent.withAlpha (0.55f) : c.track);
    g.drawRoundedRectangle (b, kCorner, 1.0f);

    // top line: name after the LED, info before the solo button
    const int nameX = kPad + (led != nullptr ? led->getWidth() + 3 : 2);
    const int infoRight = (solo != nullptr ? solo->getX() - f.getX() : width - kPad) - 4;
    g.setFont (t.font (t.type.sectionTitle));
    g.setColour ((selected ? c.accentHi : c.sectionTitle).withMultipliedAlpha (dim));
    g.drawText (name, nameX, kPad, infoRight - nameX, kTopRow, juce::Justification::centredLeft, true);
    g.setFont (t.font (t.type.value).boldened());
    g.setColour (c.text.withMultipliedAlpha (dim));
    g.drawText (info, nameX, kPad, infoRight - nameX, kTopRow, juce::Justification::centredRight, true);

    // range line
    g.setFont (t.font (t.type.value));
    g.setColour (c.mark.withMultipliedAlpha (dim));
    g.drawText (range, kPad + 2, kPad + kTopRow, width - 2 * kPad - 4, kRangeRow, juce::Justification::centredLeft, true);

    // status, bottom right
    if (status.isNotEmpty())
    {
        const int rowH = dropdown != nullptr ? dropdown->getHeight() : 12;
        g.setFont (t.font (t.type.panelTitle));
        g.setColour (c.mark.withMultipliedAlpha (dim));
        g.drawText (status, kPad, height - kPad - rowH, width - 2 * kPad - 2, rowH, juce::Justification::centredRight, true);
    }

    // traces: min / max per column around the centre line, the second one dimmer
    const auto area = traceArea().toFloat();
    const auto colour = (active ? (selected ? c.accentHi : c.trace) : c.traceOff);
    g.setColour (colour.withAlpha (0.5f));
    g.drawHorizontalLine (juce::roundToInt (area.getCentreY()), area.getX(), area.getRight());
    auto draw = [&] (const Trace& tr, float alpha)
    {
        const size_t n = juce::jmin (tr.low.size(), tr.high.size());
        if (n < 2 || area.isEmpty())
            return;
        const float half = area.getHeight() * 0.5f, cy = area.getCentreY();
        auto xAt = [&] (size_t i) { return area.getX() + area.getWidth() * (float) i / (float) (n - 1); };
        juce::Path p;
        p.startNewSubPath (xAt (0), cy - juce::jlimit (-1.0f, 1.0f, tr.high[0]) * half);
        for (size_t i = 1; i < n; ++i)
            p.lineTo (xAt (i), cy - juce::jlimit (-1.0f, 1.0f, tr.high[i]) * half);
        for (size_t i = n; i-- > 0;)
            p.lineTo (xAt (i), cy - juce::jlimit (-1.0f, 1.0f, tr.low[i]) * half);
        p.closeSubPath();
        g.setColour (colour.withAlpha (alpha));
        g.fillPath (p);
    };
    draw (second, 0.4f);
    draw (main, 1.0f);
}

void BandCard::resized()
{
    const auto f = face();
    const int cy = f.getY() + kPad + kTopRow / 2;
    if (led != nullptr)
        led->setTopLeftPosition (f.getX() + kPad, cy - led->getHeight() / 2);
    if (solo != nullptr)
        solo->setTopLeftPosition (f.getRight() - kPad - solo->getWidth(), cy - solo->getHeight() / 2);
    if (dropdown != nullptr)
        dropdown->setTopLeftPosition (f.getX() + kPad, f.getBottom() - kPad - dropdown->getHeight());
}

void BandCard::mouseDown (const juce::MouseEvent&)
{
    if (onSelect != nullptr)
        onSelect();
}

//==============================================================================
SplitHandle::SplitHandle (juce::RangedAudioParameter& p)
    : param (p),
      attach (p, [this] (float v) { value = v; repaint(); })
{
    setSize (54, 34);   // native
    setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    setTooltip ("Drag left/right to move the crossover (Shift: fine). Double-click: default.");
    attach.sendInitialUpdate();
}

void SplitHandle::paint (juce::Graphics& g)
{
    const auto& t = themeOf (*this);
    const auto box = juce::Rectangle<float> ((float) getWidth() - 6.0f, 17.0f).withCentre ({ (float) getWidth() * 0.5f, 10.0f });
    ui::drawRecess (g, t, box);
    if (isMouseOverOrDragging())
        ui::drawHoverWash (g, t, box);
    g.setFont (t.font (t.type.boxText));
    g.setColour (t.colours.boxText);
    g.drawText (param.getText (param.convertTo0to1 (value), 16), box, juce::Justification::centred, false);

    // ◂ SPLIT ▸
    const auto font = t.font (t.type.label);
    const float cy = 27.0f, w = juce::GlyphArrangement::getStringWidth (font, "SPLIT"), cx = (float) getWidth() * 0.5f;
    g.setFont (font);
    g.setColour (t.colours.label);
    g.drawText ("SPLIT", juce::Rectangle<float> (w + 4.0f, 9.0f).withCentre ({ cx, cy }), juce::Justification::centred, false);
    juce::Path arrows;
    const float l = cx - w * 0.5f - 6.0f, r = cx + w * 0.5f + 6.0f;
    arrows.addTriangle (l - 2.0f, cy, l + 1.0f, cy - 2.0f, l + 1.0f, cy + 2.0f);
    arrows.addTriangle (r + 2.0f, cy, r - 1.0f, cy - 2.0f, r - 1.0f, cy + 2.0f);
    g.fillPath (arrows);
}

void SplitHandle::set (float proportion, bool asGesture)
{
    float v = param.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, proportion));
    if (limit != nullptr)
        v = limit (v);
    if (asGesture)
        attach.setValueAsPartOfGesture (v);
    else
        attach.setValueAsCompleteGesture (v);
}

void SplitHandle::mouseDown (const juce::MouseEvent& e)
{
    dragging = true;
    lastX = e.position.x;
    attach.beginGesture();
}

void SplitHandle::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;
    const float dx = (e.position.x - lastX) * (e.mods.isShiftDown() ? 0.25f : 1.0f);
    lastX = e.position.x;
    set (param.convertTo0to1 (value) + dx / 200.0f, true);
}

void SplitHandle::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
        attach.endGesture();
    dragging = false;
    repaint();
}

void SplitHandle::mouseDoubleClick (const juce::MouseEvent&)
{
    set (param.getDefaultValue(), false);
}

//==============================================================================
BandStrip::BandStrip()
{
    setInterceptsMouseClicks (false, true);
}

void BandStrip::setBands (std::vector<BandCard*> c, std::vector<SplitHandle*> s)
{
    jassert (c.empty() || s.size() == c.size() - 1);
    for (auto* old : cards) removeChildComponent (old);
    for (auto* old : splits) removeChildComponent (old);
    cards = std::move (c);
    splits = std::move (s);
    for (auto* card : cards) addAndMakeVisible (card);
    for (auto* split : splits) addAndMakeVisible (split);
    resized();
    repaint();
}

void BandStrip::setArea (juce::Rectangle<int> row)
{
    setBounds (row.expanded (BandCard::glowPadding));
}

void BandStrip::resized()
{
    if (cards.empty())
        return;
    // the row (the faces) inside the glow padding; each card's bounds add the padding back
    const auto row = getLocalBounds().reduced (BandCard::glowPadding);
    const int n = (int) cards.size();
    const int splitW = splits.empty() ? 0 : splits.front()->getWidth();
    const int cardW = (row.getWidth() - splitW * (n - 1)) / n;
    int x = row.getX();
    for (int i = 0; i < n; ++i)
    {
        const int w = i == n - 1 ? row.getRight() - x : cardW;   // the last card takes the remainder
        cards[(size_t) i]->setBounds (juce::Rectangle<int> (x, row.getY(), w, row.getHeight()).expanded (BandCard::glowPadding));
        x += w;
        if (i < n - 1)
        {
            auto* split = splits[(size_t) i];
            split->setTopLeftPosition (x + (splitW - split->getWidth()) / 2, row.getY() + (row.getHeight() - split->getHeight()) / 2);
            x += splitW;
        }
    }
}
}
