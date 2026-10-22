#include "Modulation.h"

namespace ga
{
//==============================================================================
ModSource::ModSource (juce::String text)
    : ghostText (std::move (text))
{
    setSize (18, 18);   // native: a square button
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    setTooltip ("Drag onto a knob to modulate it");
}

void ModSource::paint (juce::Graphics& g)
{
    const auto& t = themeOf (*this);
    const auto c = getLocalBounds().toFloat().getCentre();
    const float size = (float) juce::jmin (getWidth(), getHeight()) - 2.0f;
    ui::drawRecess (g, t, c, size);
    if (isMouseOver())
        ui::drawHoverWash (g, t, juce::Rectangle<float> (size, size).withCentre (c));

    // a 2 x 3 grip
    const float u = size / 22.0f;
    g.setColour (t.colours.accent);
    for (int row = -1; row <= 1; ++row)
        for (int col : { -1, 1 })
        {
            const auto p = c + juce::Point<float> ((float) col * 2.0f * u * 1.25f, (float) row * 4.0f * u * 1.25f);
            g.fillEllipse (p.x - 1.1f * u * 1.25f, p.y - 1.1f * u * 1.25f, 2.2f * u * 1.25f, 2.2f * u * 1.25f);
        }
}

juce::Image ModSource::ghost() const
{
    // a pill in the accent colour, drawn at 2x so it stays sharp
    const auto& t = themeOf (*this);
    const auto font = t.font (t.type.panelTitle);
    const int w = juce::roundToInt (juce::GlyphArrangement::getStringWidth (font, ghostText)) + 14, h = 13;
    juce::Image image (juce::Image::ARGB, w * 2, h * 2, true);
    juce::Graphics g (image);
    g.addTransform (juce::AffineTransform::scale (2.0f));
    const auto r = juce::Rectangle<float> ((float) w, (float) h).reduced (0.5f);
    g.setColour (t.colours.accent);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    g.setColour (t.colours.onAccent);
    g.setFont (font);
    g.drawText (ghostText, r, juce::Justification::centred, false);
    return image;
}

void ModSource::mouseDrag (const juce::MouseEvent& e)
{
    if (e.getDistanceFromDragStart() < 3)
        return;
    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this))
        if (! container->isDragAndDropActive())
        {
            const auto image = ghost();
            const juce::Point<int> offset (-image.getWidth() / 4, -image.getHeight() / 4);   // centred under the cursor
            container->startDragging (modDragDescription, this, juce::ScaledImage (image, 2.0), false, &offset, &e.source);
        }
}

//==============================================================================
ModDonut::ModDonut (juce::RangedAudioParameter& p)
    : param (p),
      attach (p, [this] (float v) { value = v; repaint(); })
{
    setSize (14, 14);   // native
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    setTooltip ("Drag up/down: depth. Double-click: remove.");
    attach.sendInitialUpdate();
}

float ModDonut::depth() const
{
    return param.convertTo0to1 (value) * 2.0f - 1.0f;
}

void ModDonut::paint (juce::Graphics& g)
{
    // in the mockup's 20-unit box: ring at radius 6.5, 3.6 wide; centre disc 4.2; zero tick 1.5..5
    const auto& t = themeOf (*this);
    const auto c = getLocalBounds().toFloat().getCentre();
    const float u = (float) juce::jmin (getWidth(), getHeight()) / 20.0f;
    const juce::PathStrokeType ring (3.6f * u);

    juce::Path track;
    track.addCentredArc (c.x, c.y, 6.5f * u, 6.5f * u, 0.0f, 0.0f, juce::MathConstants<float>::twoPi, true);
    g.setColour (t.colours.track);
    g.strokePath (track, ring);

    const float a = juce::jlimit (-0.999f, 0.999f, depth()) * juce::MathConstants<float>::twoPi;
    if (std::abs (a) > 0.02f)
    {
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, 6.5f * u, 6.5f * u, 0.0f, juce::jmin (0.0f, a), juce::jmax (0.0f, a), true);
        g.setColour (isMouseOverOrDragging() ? t.colours.accentHi : t.colours.accent);
        g.strokePath (arc, ring);
    }

    g.setColour (t.colours.insetTop);
    g.fillEllipse (juce::Rectangle<float> (8.4f * u, 8.4f * u).withCentre (c));
    g.setColour (t.colours.markDim);
    g.drawLine (c.x, c.y - 8.5f * u, c.x, c.y - 5.0f * u, 1.0f);
}

void ModDonut::mouseEnter (const juce::MouseEvent&)
{
    repaint();
    if (onHover != nullptr) onHover (true);
}

void ModDonut::mouseExit (const juce::MouseEvent&)
{
    repaint();
    if (onHover != nullptr) onHover (false);
}

void ModDonut::mouseDown (const juce::MouseEvent& e)
{
    dragging = true;
    lastY = e.position.y;
    attach.beginGesture();
}

void ModDonut::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;
    // 1 % of the depth per pixel (Shift: a quarter)
    const float dy = (lastY - e.position.y) * (e.mods.isShiftDown() ? 0.25f : 1.0f);
    lastY = e.position.y;
    const float next = juce::jlimit (0.0f, 1.0f, param.convertTo0to1 (value) + dy / 200.0f);
    attach.setValueAsPartOfGesture (param.convertFrom0to1 (next));
}

void ModDonut::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
        attach.endGesture();
    dragging = false;
    repaint();
}

void ModDonut::mouseDoubleClick (const juce::MouseEvent&)
{
    if (onRemove != nullptr)
        onRemove();
}

//==============================================================================
void forEachModTarget (juce::Component& root, const std::function<void (Knob&)>& fn)
{
    for (auto* child : root.getChildren())
    {
        if (auto* knob = dynamic_cast<Knob*> (child); knob != nullptr && knob->acceptsModulation())
            fn (*knob);
        forEachModTarget (*child, fn);
    }
}
}
