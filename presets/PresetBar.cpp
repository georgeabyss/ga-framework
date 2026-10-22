#include "PresetBar.h"

namespace ga
{

namespace
{
    constexpr float kArrowW = 17.0f, kGap = 2.0f;

}

PresetBar::PresetBar (PresetManager& manager)
    : presets (manager)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    refresh();
}

void PresetBar::refresh()
{
    const bool nowMuted = isMuted != nullptr && isMuted();
    const bool nowBlink = (juce::Time::getMillisecondCounter() / 450) % 2 == 0;
    if (nowMuted != muted || (nowMuted && nowBlink != blinkOn))
    {
        muted = nowMuted;
        blinkOn = nowBlink;
        repaint();
    }

    const int i = presets.currentIndex();
    const auto number = i >= 0 ? juce::String (i + 1).paddedLeft ('0', 2) : juce::String ("--");
    const auto newText = (number + " " + presets.currentName()).toUpperCase().substring (0, themeOf (*this).looks.presetDisplay->maxChars());
    const bool newModified = presets.isModified();
    if (newText != text || newModified != modified)
    {
        text = newText;
        modified = newModified;
        repaint();
    }
}

int PresetBar::widthForHeight (int height)
{
    return juce::roundToInt ((float) height * (float) kWidth / (float) kHeight);
}

juce::AffineTransform PresetBar::fit() const
{
    const float scale = juce::jmin ((float) getWidth() / (float) kWidth, (float) getHeight() / (float) kHeight);
    const auto r = juce::Rectangle<float> ((float) kWidth * scale, (float) kHeight * scale).withCentre (getLocalBounds().toFloat().getCentre());
    return juce::AffineTransform::scale (scale).translated (r.getX(), r.getY());
}

juce::Rectangle<float> PresetBar::zoneRect (Zone z) const
{
    const auto b = juce::Rectangle<float> ((float) kWidth, (float) kHeight);
    switch (z)
    {
        case Zone::prev:    return b.withWidth (kArrowW);
        case Zone::next:    return b.withTrimmedLeft (b.getWidth() - kArrowW);
        case Zone::display: return b.reduced (kArrowW + kGap, 0.0f);
        case Zone::none:    break;
    }
    return {};
}

PresetBar::Zone PresetBar::zoneAt (juce::Point<float> p) const
{
    p = p.transformedBy (fit().inverted());
    for (auto z : { Zone::prev, Zone::display, Zone::next })
        if (zoneRect (z).contains (p))
            return z;
    return Zone::none;
}

void PresetBar::paint (juce::Graphics& g)
{
    if (const auto t = fit(); ! t.isIdentity())
        g.addTransform (t);
    paintArrowButton (g, Zone::prev);
    paintArrowButton (g, Zone::next);
    const auto& t = themeOf (*this);
    t.looks.presetDisplay->paint (g, zoneRect (Zone::display), { text, modified, muted, blinkOn, menuOpen }, t);
}

void PresetBar::paintArrowButton (juce::Graphics& g, Zone z)
{
    const auto& t = themeOf (*this);
    const auto r = zoneRect (z);
    ui::drawRecess (g, t, r);
    if (hover == z)
    {
        g.setGradientFill (juce::ColourGradient (t.colours.accent.withAlpha (0.25f), r.getCentreX(), r.getCentreY(),
                                                 t.colours.accent.withAlpha (0.0f), r.getCentreX() + r.getWidth() * 0.7f, r.getCentreY(), true));
        g.fillRect (r.reduced (1.0f));
    }
    const auto c = r.getCentre();
    const float d = z == Zone::prev ? -1.0f : 1.0f;   // pointing left or right
    juce::Path tri;
    tri.addTriangle (c.x + 2.0f * d, c.y, c.x - 2.0f * d, c.y - 3.0f, c.x - 2.0f * d, c.y + 3.0f);
    g.setColour (hover == z ? t.colours.accentHi : t.colours.mark);
    g.fillPath (tri);
}


void PresetBar::mouseMove (const juce::MouseEvent& e)
{
    const auto z = zoneAt (e.position);
    if (z != hover) { hover = z; repaint(); }
}

void PresetBar::mouseExit (const juce::MouseEvent&)
{
    hover = Zone::none;
    repaint();
}

juce::String PresetBar::getTooltip()
{
    if (muted)
        return "Output muted: absurd level or broken audio was detected. Click to resume.";
    switch (hover)
    {
        case Zone::prev:    return "Previous preset";
        case Zone::next:    return "Next preset";
        case Zone::display: return "Presets: click for the list, Save, Rename and Delete";
        case Zone::none:    break;
    }
    return {};
}

void PresetBar::mouseDown (const juce::MouseEvent& e)
{
    if (muted)   // any click resumes
    {
        if (resume != nullptr)
            resume();
        return;
    }
    switch (zoneAt (e.position))
    {
        case Zone::prev:    presets.step (-1); refresh(); break;
        case Zone::next:    presets.step (+1); refresh(); break;
        case Zone::display: showMenu(); break;
        case Zone::none:    break;
    }
}

void PresetBar::showMenu()
{
    enum { saveId = 1, saveAsId, renameId, deleteId, folderId, presetBase = 1000 };

    presets.rescan();   // pick up files added in the folder meanwhile
    const auto& all = presets.all();
    const int current = presets.currentIndex();
    const bool currentIsUser = current >= 0 && ! all[(size_t) current].isFactory;

    juce::PopupMenu m;
    m.setLookAndFeel (&getLookAndFeel());
    bool userHeader = false;
    m.addSectionHeader ("Factory");
    for (size_t i = 0; i < all.size(); ++i)
    {
        if (! all[i].isFactory && ! userHeader)
        {
            m.addSectionHeader ("User");
            userHeader = true;
        }
        juce::PopupMenu::Item item (juce::String ((int) i + 1).paddedLeft ('0', 2) + "  " + all[i].name);
        item.itemID = presetBase + (int) i;
        item.isTicked = (int) i == current;
        item.shortcutKeyDescription = all[i].listTag;   // drawn small on the right
        m.addItem (item);
    }
    m.addSeparator();
    m.addItem (saveId, currentIsUser ? "Save" : "Save...");
    m.addItem (saveAsId, "Save As...");
    m.addItem (renameId, "Rename...", currentIsUser);
    m.addItem (deleteId, "Delete", currentIsUser);
    m.addItem (folderId, "Show Presets Folder");

    menuOpen = true;
    repaint();
    juce::Component::SafePointer<PresetBar> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth ((int) zoneRect (Zone::display).transformedBy (fit()).getWidth()),
                     [safe, current, currentIsUser] (int id)
    {
        if (safe == nullptr)
            return;
        auto& self = *safe;
        self.menuOpen = false;
        const auto name = self.presets.currentName();
        auto saveAs = [safe, name]
        {
            safe->askForName ("Save Preset", name, [safe] (juce::String n) { if (safe != nullptr) safe->presets.save (n); });
        };

        if (id >= presetBase)
            self.presets.load (id - presetBase);
        else if (id == saveId)
        {
            if (currentIsUser) self.presets.save (name);
            else saveAs();
        }
        else if (id == saveAsId)
            saveAs();
        else if (id == renameId)
            self.askForName ("Rename Preset", name, [safe, current] (juce::String n) { if (safe != nullptr) safe->presets.rename (current, n); });
        else if (id == deleteId)
            self.confirm ("Delete Preset", "Delete \"" + name + "\"? This cannot be undone.",
                          [safe, current] { if (safe != nullptr) safe->presets.remove (current); });
        else if (id == folderId)
        {
            self.presets.folder().createDirectory();
            self.presets.folder().startAsProcess();
        }
        self.refresh();
        self.repaint();
    });
}

LookAndFeel& PresetBar::dialogLookAndFeel()
{
    if (dialogLook == nullptr)
        dialogLook = std::make_shared<LookAndFeel> (themeOf (*this));
    return *dialogLook;
}

void PresetBar::askForName (const juce::String& title, const juce::String& initial, std::function<void (juce::String)> done)
{
    auto* w = new juce::AlertWindow (title, {}, juce::MessageBoxIconType::NoIcon);
    w->setLookAndFeel (&dialogLookAndFeel());
    w->addTextEditor ("name", initial, "Name:");
    w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    if (auto* editor = w->getTextEditor ("name"))
        editor->selectAll();

    // the callback runs before the window is deleted, and keeps the look and feel alive until then
    w->enterModalState (true, juce::ModalCallbackFunction::create ([w, look = dialogLook, done = std::move (done), safe = juce::Component::SafePointer<PresetBar> (this)] (int result)
    {
        if (result == 1 && safe != nullptr && w->getTextEditorContents ("name").trim().isNotEmpty())
        {
            done (w->getTextEditorContents ("name").trim());
            safe->refresh();
        }
    }), true);
}

void PresetBar::confirm (const juce::String& title, const juce::String& message, std::function<void()> done)
{
    auto* w = new juce::AlertWindow (title, message, juce::MessageBoxIconType::NoIcon);
    w->setLookAndFeel (&dialogLookAndFeel());
    w->addButton ("Delete", 1);
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([look = dialogLook, done = std::move (done), safe = juce::Component::SafePointer<PresetBar> (this)] (int result)
    {
        if (result == 1 && safe != nullptr)
        {
            done();
            safe->refresh();
        }
    }), true);
}
}
