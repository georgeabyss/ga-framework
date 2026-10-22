#include "EditorShell.h"
#include "plugin/SharedIds.h"

namespace ga
{
EditorShell::EditorShell (ProcessorBase& p, juce::AudioProcessorValueTreeState& s, BottomStrip::Host host, Theme theme)
    : AudioProcessorEditor (&p), state (s), config (p.getConfig()), laf (std::move (theme)), strip (s, std::move (host), p.getConfig())
{
    setLookAndFeel (&laf);

    strip.sizeBox.getItems = [this]
    {
        juce::StringArray items;
        for (float sc : menuSizes()) items.add (juce::String (juce::roundToInt (sc * 100.0f)) + "%");
        return items;
    };
    strip.sizeBox.getSelected = [this]
    {
        const auto sizes = menuSizes();
        for (int i = 0; i < (int) sizes.size(); ++i)
            if (std::abs (sizes[(size_t) i] - windowScale()) < 0.01f) return i;
        return -1;
    };
    strip.sizeBox.getText = [this] { return juce::String (juce::roundToInt (windowScale() * 100.0f)) + "%"; };
    strip.sizeBox.onSelect = [this] (int i) { applyScale (menuSizes()[(size_t) i]); };

    strip.onScopeToggle = [this] (bool open) { setScopeOpen (open); };
    strip.onScopeData = [this] (const float* l, const float* r, int n) { goniometer.push (l, r, n); };
}

EditorShell::~EditorShell()
{
    setLookAndFeel (nullptr);
}

void EditorShell::setPanel (Panel& p, int w, int h, int mx, int my)
{
    panel = &p;
    baseWidth = w;
    baseHeight = h;
    marginX = mx;
    addAndMakeVisible (faceplate);   // first, so the panel already sees the theme while it lays out

    const bool open = config.scope && (bool) state.state.getProperty (prop::scopeOpen, false);   // saved with the session
    strip.setScopeOpen (open);
    goniometer.setVisible (open);

    faceplate.addAndMakeVisible (p);
    faceplate.addAndMakeVisible (strip);
    faceplate.addChildComponent (goniometer);
    p.layout (juce::Rectangle<int> (w, h).reduced (mx, my), strip);
    p.setBounds (0, 0, w, h);
    goniometer.setBounds (scopeBox().reduced (6));

    setConstrainer (&constrainer);
    updateConstraints();
    setResizable (true, true);
    applyScale ((float) (double) state.state.getProperty (prop::uiScale, 1.0));   // saved with the session, relative to 100 %
}

// the goniometer box, to the right of the plug-in's panel
juce::Rectangle<int> EditorShell::scopeBox() const
{
    const int size = baseHeight - 28;
    return { baseWidth - 14, 14, size, size };
}

int EditorShell::contentWidth() const
{
    // open: the goniometer box plus the same margin on its right as on the left
    return baseWidth + (goniometer.isVisible() ? scopeBox().getRight() + marginX - baseWidth : 0);
}

std::vector<float> EditorShell::menuSizes() const
{
    // the screen the editor is on (or the main one), less the menu bar and dock
    const auto& displays = juce::Desktop::getInstance().getDisplays();
    const auto* display = isShowing() ? displays.getDisplayForRect (getScreenBounds()) : displays.getPrimaryDisplay();

    std::vector<float> sizes;
    for (float sc : config.windowSizes)
        if (display == nullptr || sizes.empty()
            || ((float) contentWidth() * sc * config.defaultScale <= display->userBounds.getWidth()
                && (float) baseHeight * sc * config.defaultScale <= display->userBounds.getHeight()))
            sizes.push_back (sc);   // the smallest always stays
    return sizes;
}

void EditorShell::setScopeOpen (bool open)
{
    state.state.setProperty (prop::scopeOpen, open, nullptr);
    goniometer.setVisible (open);
    updateConstraints();
    applyScale (windowScale());
    faceplate.repaint();
}

void EditorShell::updateConstraints()
{
    const double w = (double) contentWidth() * config.defaultScale, h = (double) baseHeight * config.defaultScale;   // at 100 %
    constrainer.setFixedAspectRatio (w / h);
    constrainer.setSizeLimits (juce::roundToInt (w * config.minScale), juce::roundToInt (h * config.minScale),
                               juce::roundToInt (w * config.maxScale), juce::roundToInt (h * config.maxScale));
}

void EditorShell::applyScale (float scale)
{
    const float layoutScale = juce::jlimit (config.minScale, config.maxScale, scale) * config.defaultScale;
    setSize (juce::roundToInt ((float) contentWidth() * layoutScale), juce::roundToInt ((float) baseHeight * layoutScale));
}

void EditorShell::resized()
{
    if (panel == nullptr)
        return;

    // the faceplate is laid out in layout pixels and scaled as a whole
    faceplate.setTransform (juce::AffineTransform::scale (layoutScale()));
    faceplate.setBounds (0, 0, contentWidth(), baseHeight);
    state.state.setProperty (prop::uiScale, windowScale(), nullptr);
    faceplate.repaint();
}

void EditorShell::Faceplate::paint (juce::Graphics& g)
{
    const auto& t = shell.laf.theme;
    const float W = (float) getWidth(), H = (float) getHeight();

    // one long faceplate
    t.looks.faceplate->paint (g, getLocalBounds().toFloat(), t);

    if (shell.goniometer.isVisible())
    {
        const auto box = shell.scopeBox();
        ui::drawInsetPanel (g, t, box.toFloat());
        g.setFont (t.font (t.type.panelTitle));
        g.setColour (t.colours.label);
        g.drawText ("GONIOMETER", box.getX() + 8, box.getY() + 6, 80, 9, juce::Justification::centredLeft, false);
    }

    // rack screws, from the corners
    for (float x : { t.sizes.screwInsetX, W - t.sizes.screwInsetX })
        for (float y : { t.sizes.screwInsetY, H - t.sizes.screwInsetY })
            ui::drawScrew (g, t, x, y);
}
}
