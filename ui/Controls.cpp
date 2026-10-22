#include "Controls.h"

namespace ga
{
namespace
{
    // Right-click menu for a parameter: the caller's items first (ids >= 100), then reset to
    // default and the host's own items for this parameter (automation, MIDI learn, ...).
    void showParameterMenu (juce::Component& target, juce::RangedAudioParameter& param, juce::PopupMenu items,
                            std::function<void (int)> onItem, std::function<void()> onReset)
    {
        juce::PopupMenu m;
        m.addSectionHeader (param.getName (64));
        if (items.getNumItems() > 0)
        {
            for (juce::PopupMenu::MenuItemIterator it (items); it.next();)
                m.addItem (it.getItem());
            m.addSeparator();
        }
        m.addItem (1, "Reset to default (" + param.getText (param.getDefaultValue(), 32)
                        + (param.getLabel().isNotEmpty() ? " " + param.getLabel() : juce::String()) + ")");

        std::shared_ptr<juce::HostProvidedContextMenu> hostMenu;
        if (auto* editor = target.findParentComponentOfClass<juce::AudioProcessorEditor>())
            if (auto* host = editor->getHostContext())
                hostMenu = host->getContextMenuForParameter (&param);
        if (hostMenu != nullptr)
            m.addSubMenu ("Host", hostMenu->getEquivalentPopupMenu());

        m.setLookAndFeel (&target.getLookAndFeel());   // popup menus do not inherit it by themselves
        juce::Component::SafePointer<juce::Component> safe (&target);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&target).withMousePosition(),
                         [safe, hostMenu, onItem = std::move (onItem), onReset = std::move (onReset)] (int result)
                         {
                             if (safe == nullptr || result == 0) return;
                             if (result == 1) onReset();
                             else if (result >= 100 && onItem != nullptr) onItem (result);
                         });
    }

    // wheel movement as one number, positive = up / right, honouring "natural" scrolling
    float wheelAmount (const juce::MouseWheelDetails& w)
    {
        const float d = std::abs (w.deltaX) > std::abs (w.deltaY) ? -w.deltaX : w.deltaY;
        return w.isReversed ? -d : d;
    }
}

//==============================================================================
Knob::Knob (juce::RangedAudioParameter& p, const juce::String& name, bool centreDetent, Style st)
    : param (p),
      nameText (name),
      style (st),
      detentWidth (centreDetent ? 0.04f : 0.0f),
      attach (p, [this] (float v) { value = v; repaint(); })
{
    setSize (st == Style::regular ? 56 : 80, st == Style::regular ? 70 : 26);   // native size
    setRepaintsOnMouseActivity (true);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    setTooltip ("Drag up/down (Shift: fine). Double-click: default. Right-click: menu.");
    attach.sendInitialUpdate();
}

juce::Point<int> Knob::dialCentre() const
{
    return style == Style::regular ? juce::Point<int> (28, 22) : juce::Point<int> (12, getHeight() / 2);
}

juce::Point<int> anchorOf (const juce::Component& c)
{
    if (auto* knob = dynamic_cast<const Knob*> (&c))
        return knob->dialCentre();
    return { c.getWidth() / 2, c.getHeight() / 2 };
}

// Travel is the drag position. With a detent, the centre is stretched over detentWidth,
// so the value rests at the centre for a few pixels and then moves on without a jump.
float Knob::toTravel (float prop) const
{
    if (detentWidth <= 0.0f || prop < 0.5f - 1.0e-4f) return prop;
    if (prop <= 0.5f + 1.0e-4f) return 0.5f + 0.5f * detentWidth;
    return prop + detentWidth;
}

float Knob::fromTravel (float t) const
{
    if (t < 0.5f) return t;
    if (t < 0.5f + detentWidth) return 0.5f;
    return t - detentWidth;
}

juce::String Knob::valueString() const
{
    const auto label = param.getLabel();
    return param.getText (proportion(), 32) + (label.isNotEmpty() ? " " + label : juce::String());
}

void Knob::paint (juce::Graphics& g)
{
    const auto& t = themeOf (*this);
    const bool hot = isMouseOverOrDragging();
    const auto valueColour = dragging ? t.colours.valueActive : t.colours.value;

    if (style == Style::mini)
    {
        const float cy = (float) getHeight() * 0.5f;
        t.looks.knob->paintMini (g, t, { 12.0f, cy }, proportion(), hot);

        // the name; with a lock it is the lock switch: lavender with a padlock while locked,
        // brighter with a faint open padlock while hovered
        const auto name = nameArea();
        g.setColour (locked ? t.colours.locked : overName ? t.colours.labelHover : t.colours.label);
        g.setFont (t.font (t.type.label));
        g.drawText (nameText.toUpperCase(), name, juce::Justification::centredLeft, false);
        if (locked || overName)
            ui::drawPadlock (g, name.getRight() + 2.5f, name.getY() + 1.5f, ! locked,
                             locked ? t.colours.accent : t.colours.accent.withAlpha (0.4f));
        if (valueEditor == nullptr)
        {
            g.setColour (valueColour);
            g.setFont (t.font (t.type.value));
            g.drawText (valueString(), juce::Rectangle<float> (26.0f, cy - 0.5f, (float) getWidth() - 26.0f, 11.0f),
                        juce::Justification::centredLeft, false);
        }
        return;
    }

    t.looks.knob->paint (g, t, { 28.0f, 22.0f }, proportion(), hot);
    if (valueEditor == nullptr)
    {
        g.setColour (valueColour);
        g.setFont (t.font (t.type.value));
        g.drawText (valueString(), juce::Rectangle<float> (56.0f, 12.0f).withCentre ({ 28.0f, 22.0f + 31.0f - 4.0f }),
                    juce::Justification::centred, false);
    }
    ui::drawName (g, t, 28.0f, 22.0f, nameText);
}

juce::Rectangle<float> Knob::nameArea() const
{
    const float cy = (float) getHeight() * 0.5f;
    const auto& t = themeOf (*this);
    const float w = juce::GlyphArrangement::getStringWidth (t.font (t.type.label), nameText.toUpperCase());
    return { 26.0f, cy - 10.0f, std::ceil (w) + 1.0f, 9.0f };
}

juce::Rectangle<float> Knob::lockArea() const
{
    return nameArea().withTrimmedRight (-9.0f).expanded (1.0f, 2.0f);
}

void Knob::setLockable (std::function<void()> onClick)
{
    onLockClick = std::move (onClick);
    setTooltip ("Drag up/down (Shift: fine). Double-click: default. Right-click: menu. "
                "Click the name to lock it: a locked value stays when you switch presets.");
}

void Knob::setLocked (bool shouldBeLocked)
{
    if (shouldBeLocked != locked) { locked = shouldBeLocked; repaint(); }
}

void Knob::mouseMove (const juce::MouseEvent& e)
{
    const bool over = onLockClick != nullptr && style == Style::mini && lockArea().contains (e.position);
    if (over != overName)
    {
        overName = over;
        setMouseCursor (over ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::UpDownResizeCursor);
        repaint();
    }
}

void Knob::mouseExit (const juce::MouseEvent&)
{
    if (overName) { overName = false; repaint(); }
}

void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { showMenu(); return; }
    if (e.mods.isAltDown())   { resetToDefault(); return; }
    if (overName && onLockClick != nullptr) { onLockClick(); return; }

    dragging = true;
    travel = toTravel (proportion());
    lastY = e.position.y;
    attach.beginGesture();
    e.source.enableUnboundedMouseMovement (true);   // hides the cursor, no screen edge
    repaint();
}

void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;

    // relative movement: the knob never jumps, and pressing Shift mid-drag just slows it down
    const float dy = lastY - e.position.y;
    lastY = e.position.y;
    const bool fine = e.mods.isShiftDown() || e.mods.isCommandDown();
    travel = juce::jlimit (0.0f, 1.0f + detentWidth, travel + dy / (fine ? 1600.0f : 220.0f));
    attach.setValueAsPartOfGesture (param.convertFrom0to1 (fromTravel (travel)));
}

void Knob::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
        attach.endGesture();
    dragging = false;
    repaint();
}

void Knob::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
        resetToDefault();
}

void Knob::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const float d = wheelAmount (w);
    if (dragging || d == 0.0f)
        return;
    const float next = juce::jlimit (0.0f, 1.0f, proportion() + d * (e.mods.isShiftDown() ? 0.03f : 0.15f));
    attach.setValueAsCompleteGesture (param.convertFrom0to1 (next));
}

void Knob::resetToDefault()
{
    attach.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue()));
}

void Knob::showMenu()
{
    juce::PopupMenu items;
    items.addItem (100, "Enter value...");
    juce::Component::SafePointer<Knob> safe (this);
    showParameterMenu (*this, param, items,
                       [safe] (int) { if (safe != nullptr) safe->showValueEditor(); },
                       [safe] { if (safe != nullptr) safe->resetToDefault(); });
}

void Knob::showValueEditor()
{
    const auto& t = themeOf (*this);
    valueEditor = std::make_unique<juce::TextEditor>();
    auto& ed = *valueEditor;
    ed.setJustification (juce::Justification::centred);
    ed.setFont (t.font (t.type.valueEdit));
    ed.setColour (juce::TextEditor::backgroundColourId, t.colours.recessTop);
    ed.setColour (juce::TextEditor::textColourId, t.colours.text);
    ed.setColour (juce::TextEditor::highlightColourId, t.colours.accent.withAlpha (0.45f));
    ed.setColour (juce::TextEditor::outlineColourId, t.colours.accent.withAlpha (0.6f));
    ed.setColour (juce::TextEditor::focusedOutlineColourId, t.colours.accent);
    ed.setColour (juce::CaretComponent::caretColourId, t.colours.accentHi);
    ed.setIndents (2, 1);
    ed.setText (param.getText (proportion(), 32), false);
    if (style == Style::mini)
        ed.setBounds (24, getHeight() / 2 - 1, getWidth() - 24, 13);
    else
        ed.setBounds (1, 40, 54, 14);
    ed.setJustification (style == Style::mini ? juce::Justification::centredLeft : juce::Justification::centred);
    addAndMakeVisible (ed);
    ed.selectAll();
    ed.grabKeyboardFocus();

    ed.onReturnKey = [this]
    {
        const auto t = valueEditor->getText().trim();
        closeValueEditor();
        if (t.isNotEmpty())
            attach.setValueAsCompleteGesture (param.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, param.getValueForText (t))));
    };
    ed.onEscapeKey = [this] { closeValueEditor(); };
    ed.onFocusLost = [this] { closeValueEditor(); };
    repaint();
}

void Knob::closeValueEditor()
{
    if (valueEditor == nullptr || ! valueEditor->isVisible())
        return;
    valueEditor->setVisible (false);
    // deleted later: this is called from inside the editor's own callbacks
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<Knob> (this)]
    {
        if (safe != nullptr) { safe->valueEditor.reset(); safe->repaint(); }
    });
}

//==============================================================================
StepBox::StepBox (juce::RangedAudioParameter& p, bool showNames, bool showArrows)
    : param (p),
      names (showNames),
      arrows (showArrows || ! showNames),
      attach (p, [this] (float v) { value = (int) std::lround (v); repaint(); })
{
    const auto r = p.getNormalisableRange();
    minValue = (int) r.start;
    maxValue = (int) r.end;
    setSize (names ? 64 : 30, names ? 19 : 30);   // native size
    setMouseCursor (names ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::UpDownResizeCursor);
    setTooltip (! names ? "Drag up/down, click the arrows or use the wheel. Double-click: default. Right-click: menu."
                        : arrows ? "Click: choose. Arrows or wheel: step. Right-click: menu."
                                 : "Click: choose. Wheel: step. Right-click: menu.");
    attach.sendInitialUpdate();
}

void StepBox::setWideNumber (std::function<int (int, int)> rule, float px)
{
    wide = true;
    arrowStep = std::move (rule);
    pxPerStep = px;
    setSize (64, 19);   // the size of a drop-down
    setTooltip ("Drag up/down for single steps; the arrows double / halve. Double-click: default. Right-click: menu.");
    repaint();
}

juce::Rectangle<float> StepBox::boxRect() const
{
    if (names || wide)
        return getLocalBounds().toFloat().reduced (2.0f, 1.0f);
    return juce::Rectangle<float> (22.0f, 22.0f).withCentre (getLocalBounds().toFloat().getCentre());
}

int StepBox::arrowAt (juce::Point<float> p) const
{
    if (! arrows)
        return 0;
    const auto r = boxRect();
    if (p.x < r.getRight() - (names || wide ? 12.0f : 9.0f) || p.x > r.getRight() + 2.0f)
        return 0;
    return p.y < r.getCentreY() ? 1 : -1;
}

juce::String StepBox::valueText (int v) const
{
    return param.getText (param.convertTo0to1 ((float) v), 24);
}

void StepBox::step (int delta)
{
    int next = value + delta;
    if (names)   // choices wrap around
        next = next > maxValue ? minValue : (next < minValue ? maxValue : next);
    attach.setValueAsCompleteGesture ((float) juce::jlimit (minValue, maxValue, next));
}

void StepBox::paint (juce::Graphics& g)
{
    const auto& t = themeOf (*this);
    const auto r = boxRect();
    const auto c = r.getCentre();
    ui::drawRecess (g, t, r);
    if (isMouseOverOrDragging() && hoverArrow == 0)
    {
        ui::drawHoverWash (g, t, r);
    }

    g.setColour (t.colours.boxText);
    if (names || wide)
    {
        g.setFont (t.font (t.type.boxText));
        g.drawText (wide ? juce::String (value) : valueText (value).toUpperCase(),
                    r.withTrimmedRight (11.0f).withTrimmedLeft (2.0f), juce::Justification::centred, false);
    }
    else
    {
        g.setFont (t.font (t.type.boxNumber));
        g.drawText (juce::String (value), juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ c.x - 3.0f, c.y }),
                    juce::Justification::centred, false);
    }

    const float ax = r.getRight() - 5.0f;
    const float ay = c.y;
    if (! arrows)   // drop-down mark
    {
        ui::drawDropDownMark (g, ax, ay, isMouseOver() ? t.colours.accentHi : t.colours.mark);
        return;
    }
    juce::Path up, down;
    up.addTriangle (ax - 2.4f, ay - 1.2f, ax, ay - 4.0f, ax + 2.4f, ay - 1.2f);
    down.addTriangle (ax - 2.4f, ay + 1.2f, ax, ay + 4.0f, ax + 2.4f, ay + 1.2f);
    g.setColour (hoverArrow == 1 ? t.colours.accentHi : t.colours.mark);
    g.fillPath (up);
    g.setColour (hoverArrow == -1 ? t.colours.accentHi : t.colours.mark);
    g.fillPath (down);
}

void StepBox::mouseMove (const juce::MouseEvent& e)
{
    const int a = arrowAt (e.position);
    if (a != hoverArrow) { hoverArrow = a; repaint(); }
    setMouseCursor (a != 0 || names ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::UpDownResizeCursor);
}

void StepBox::mouseExit (const juce::MouseEvent&)
{
    hoverArrow = 0;
    repaint();
}

void StepBox::mouseDown (const juce::MouseEvent& e)
{
    dragging = false;
    if (e.mods.isPopupMenu()) { showMenu (false); return; }

    if (const int a = arrowAt (e.position))
    {
        if (arrowStep != nullptr)
            attach.setValueAsCompleteGesture ((float) juce::jlimit (minValue, maxValue, arrowStep (value, a)));
        else
            step (a);
        return;
    }
    if (names) { showMenu (true); return; }   // the name box is a drop-down

    dragging = true;
    lastY = e.position.y;
    dragAccum = 0.0f;
    attach.beginGesture();
    e.source.enableUnboundedMouseMovement (true);
}

void StepBox::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;
    dragAccum += lastY - e.position.y;
    lastY = e.position.y;
    const float px = wide ? pxPerStep * (e.mods.isShiftDown() ? 2.4f : 1.0f) : (e.mods.isShiftDown() ? 24.0f : 10.0f);
    const int steps = (int) (dragAccum / px);
    if (steps != 0)
    {
        dragAccum -= (float) steps * px;
        attach.setValueAsPartOfGesture ((float) juce::jlimit (minValue, maxValue, value + steps));
    }
}

void StepBox::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
        attach.endGesture();
    dragging = false;
}

void StepBox::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (! names && arrowAt (e.position) == 0 && ! e.mods.isPopupMenu())
        attach.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue()));
}

void StepBox::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    const float d = wheelAmount (w);
    if (dragging || d == 0.0f)
        return;
    if (! w.isSmooth) { step (d > 0.0f ? 1 : -1); return; }   // mouse wheel: one notch, one step

    // trackpad: many tiny events, so collect them first
    wheelAccum += d;
    constexpr float perStep = 0.12f;
    while (std::abs (wheelAccum) >= perStep)
    {
        step (wheelAccum > 0.0f ? 1 : -1);
        wheelAccum -= std::copysign (perStep, wheelAccum);
    }
}

void StepBox::showMenu (bool valuesOnly)
{
    juce::PopupMenu items;
    for (int v = minValue; v <= maxValue; ++v)
        items.addItem (100 + v - minValue, valueText (v), true, v == value);

    juce::Component::SafePointer<StepBox> safe (this);
    auto onItem = [safe] (int id)
    {
        if (safe != nullptr)
            safe->attach.setValueAsCompleteGesture ((float) (safe->minValue + id - 100));
    };

    items.setLookAndFeel (&getLookAndFeel());
    if (valuesOnly)
    {
        items.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                             [onItem] (int id) { if (id >= 100) onItem (id); });
        return;
    }
    showParameterMenu (*this, param, items, onItem,
                       [safe] { if (safe != nullptr) safe->attach.setValueAsCompleteGesture (safe->param.convertFrom0to1 (safe->param.getDefaultValue())); });
}

//==============================================================================
bool LampButton::hitTest (int x, int y)
{
    const int size = juce::roundToInt (themeOf (*this).sizes.lampSize);   // the lens
    return juce::Rectangle<int> (size, size).withCentre (getLocalBounds().getCentre()).contains (x, y);
}

void LampButton::paintButton (juce::Graphics& g, bool, bool)
{
    const auto& t = themeOf (*this);
    const bool on = getToggleState();
    const auto c = getLocalBounds().toFloat().getCentre();
    const float s = t.sizes.lampSize / Theme::Sizes{}.lampSize;   // today's sizes times s (exactly 1 by default)

    // thin dark holder, then the milky lens (glow stays inside)
    const auto holder = juce::Rectangle<float> (14.4f * s, 14.4f * s).withCentre (c);
    g.setColour (t.colours.hole);
    g.fillRoundedRectangle (holder, 1.2f * s);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (holder, 1.2f * s, 1.0f);

    const auto lens = juce::Rectangle<float> (12.0f * s, 12.0f * s).withCentre (c);
    {
        const auto centre = juce::Point<float> (c.x, c.y - 0.6f * s);
        juce::ColourGradient grad (on ? t.colours.lampLit : t.colours.lensOff, centre.x, centre.y,
                                   on ? t.colours.lampLitEdge : t.colours.lampOffEdge, centre.x + 7.8f * s, centre.y, true);
        if (on) grad.addColour (0.55, t.colours.accent);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (lens, 0.8f * s);
    }
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (lens, 0.8f * s, 1.0f);

    // two soft reflections, drawn inside the lens as vector gradients (no clipping, so they
    // stay smooth at every window size)
    {
        const float ys[2] = { c.y - 4.3f * s, c.y + 4.3f * s };
        const float as[2] = { on ? 0.60f : 0.26f, on ? 0.42f : 0.17f };
        for (int i = 0; i < 2; ++i)
        {
            const auto band = juce::Rectangle<float> (lens.getX() + 1.0f * s, ys[i] - 1.6f * s, lens.getWidth() - 2.0f * s, 3.2f * s);
            juce::ColourGradient rg (juce::Colours::white.withAlpha (as[i]), band.getCentreX(), band.getCentreY(),
                                     juce::Colours::white.withAlpha (0.0f), band.getCentreX(), band.getY(), false);
            g.setGradientFill (rg);
            g.fillRoundedRectangle (band, 1.2f * s);
        }
    }
}

//==============================================================================
void SquareToggle::paintButton (juce::Graphics& g, bool over, bool)
{
    const auto& t = themeOf (*this);
    const bool lit = invert ? ! getToggleState() : getToggleState();
    const auto c = getLocalBounds().toFloat().getCentre();
    const float size = (float) juce::jmin (getWidth(), getHeight()) - 2.0f;
    ui::drawRecess (g, t, c, size);

    if (over)
    {
        ui::drawHoverWash (g, t, juce::Rectangle<float> (size, size).withCentre (c));
    }

    const auto col = lit ? t.colours.accent : t.colours.markDim;

    if (lit)
    {
        const float gr = size * 0.4f;
        g.setGradientFill (juce::ColourGradient (t.colours.accent.withAlpha (0.28f), c.x, c.y, t.colours.accent.withAlpha (0.0f), c.x + gr, c.y, true));
        g.fillEllipse (c.x - gr, c.y - gr, gr * 2.0f, gr * 2.0f);
    }

    g.setColour (col);
    if (icon == Icon::scope)
    {
        // tiny goniometer: the diamond with a narrow, nearly mono trace
        constexpr float s = 4.6f;
        juce::Path d;
        d.startNewSubPath (c.x, c.y - s);
        d.lineTo (c.x + s, c.y);
        d.lineTo (c.x, c.y + s);
        d.lineTo (c.x - s, c.y);
        d.closeSubPath();
        g.strokePath (d, juce::PathStrokeType (1.0f));
        g.fillEllipse (c.x - 1.1f, c.y - 2.8f, 2.2f, 5.6f);
    }
    else
    {
        juce::Path p;
        p.addCentredArc (c.x, c.y, 5.0f, 5.0f, 0.0f, juce::degreesToRadians (57.7f), juce::degreesToRadians (302.3f), true);
        p.startNewSubPath (c.x, c.y - 6.0f);
        p.lineTo (c.x, c.y - 0.8f);
        g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

//==============================================================================
void SectionLed::paintButton (juce::Graphics& g, bool isOver, bool)
{
    const auto& t = themeOf (*this);
    const auto c = getLocalBounds().toFloat().getCentre();
    ui::drawLed (g, t, c, lit);
    if (isOver)
    {
        const float s = t.sizes.ledRadius / Theme::Sizes{}.ledRadius;   // the hover ring grows with the LED
        g.setColour (t.colours.accentHi.withAlpha (0.45f));
        g.drawEllipse (c.x - 4.5f * s, c.y - 4.5f * s, 9.0f * s, 9.0f * s, 1.0f);
    }
}

//==============================================================================
void LedToggle::paintButton (juce::Graphics& g, bool isOver, bool)
{
    // the label, then the LED right after it
    const auto& t = themeOf (*this);
    const float cy = (float) getHeight() * 0.5f;
    const auto font = t.font (t.type.label);
    const auto text = getButtonText().toUpperCase();
    const float textW = juce::GlyphArrangement::getStringWidth (font, text);
    g.setColour (isOver ? t.colours.labelHover : t.colours.label);
    g.setFont (font);
    g.drawText (text, juce::Rectangle<float> (2.0f, cy - 5.0f, textW + 2.0f, 10.0f), juce::Justification::centredLeft, false);
    ui::drawLed (g, t, { ledCentreX(), cy }, getToggleState(), alarm);
}

float LedToggle::ledCentreX() const
{
    const auto& t = themeOf (*this);
    return 2.0f + juce::GlyphArrangement::getStringWidth (t.font (t.type.label), getButtonText().toUpperCase()) + 8.0f;
}

//==============================================================================
void SoloButton::paintButton (juce::Graphics& g, bool over, bool)
{
    const auto& t = themeOf (*this);
    const auto c = getLocalBounds().toFloat().getCentre();
    const float size = (float) juce::jmin (getWidth(), getHeight()) - 2.0f;
    const auto r = juce::Rectangle<float> (size, size).withCentre (c);
    const bool on = getToggleState();

    if (on)
    {
        // lit: a glow around it, then the orange face
        g.setGradientFill (juce::ColourGradient (t.colours.alarm.withAlpha (0.45f), c.x, c.y, t.colours.alarm.withAlpha (0.0f), c.x + size, c.y, true));
        g.fillEllipse (r.expanded (size * 0.5f));
        juce::ColourGradient face (t.colours.alarmLight, c.x - size * 0.05f, c.y - size * 0.1f, t.colours.alarmDeep, r.getRight(), r.getBottom(), true);
        face.addColour (0.6, t.colours.alarm);
        g.setGradientFill (face);
        g.fillRoundedRectangle (r, t.sizes.recessCorner);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r, t.sizes.recessCorner, 1.0f);
    }
    else
    {
        ui::drawRecess (g, t, r);
        if (over)
            ui::drawHoverWash (g, t, r);
    }

    g.setFont (t.font (t.type.buttonLetter));
    g.setColour (on ? t.colours.onAlarm : over ? t.colours.text : t.colours.mark);
    g.drawText ("S", r, juce::Justification::centred, false);
}

//==============================================================================
void DiceButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    // a die: rounded square with five pips
    const auto& t = themeOf (*this);
    const auto r = getLocalBounds().toFloat().reduced (1.5f);
    const float s = r.getWidth() / 11.0f;
    g.setColour (over || down ? t.colours.accentHi : t.colours.markDim);
    g.drawRoundedRectangle (r, 2.4f * s, 1.1f * s);
    for (auto p : { juce::Point<float> (3.2f, 3.2f), { 7.8f, 3.2f }, { 5.5f, 5.5f }, { 3.2f, 7.8f }, { 7.8f, 7.8f } })
        g.fillEllipse (r.getX() + p.x * s - 0.95f * s, r.getY() + p.y * s - 0.95f * s, 1.9f * s, 1.9f * s);
}

//==============================================================================
void MenuBox::paint (juce::Graphics& g)
{
    const auto& t = themeOf (*this);
    const auto r = getLocalBounds().toFloat().reduced (1.0f);
    ui::drawRecess (g, t, r);
    if (isMouseOver())
    {
        ui::drawHoverWash (g, t, r);
    }
    g.setFont (t.font (t.type.boxText));
    g.setColour (t.colours.boxText);
    g.drawText (getText != nullptr ? getText().toUpperCase() : juce::String(), r.withTrimmedRight (9.0f),
                juce::Justification::centred, false);

    ui::drawDropDownMark (g, r.getRight() - 6.0f, r.getCentreY(), t.colours.mark);
}

void MenuBox::mouseDown (const juce::MouseEvent&)
{
    if (getItems == nullptr)
        return;
    const auto items = getItems();
    const int selected = getSelected != nullptr ? getSelected() : -1;
    juce::PopupMenu m;
    for (int i = 0; i < items.size(); ++i)
        m.addItem (1 + i, items[i], true, i == selected);

    m.setLookAndFeel (&getLookAndFeel());
    juce::Component::SafePointer<MenuBox> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                     [safe] (int id)
                     {
                         if (safe != nullptr && id > 0 && safe->onSelect != nullptr)
                         {
                             safe->onSelect (id - 1);
                             safe->repaint();
                         }
                     });
}
}
