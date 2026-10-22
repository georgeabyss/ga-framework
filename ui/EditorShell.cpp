#include "EditorShell.h"
#include "plugin/SharedIds.h"

namespace ga
{
namespace
{
    constexpr float kSizes[] = { 0.75f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f };
    constexpr float kMinScale = 0.6f, kMaxScale = 2.5f;   // dragging the corner
}

EditorShell::EditorShell (juce::AudioProcessor& p, juce::AudioProcessorValueTreeState& s, BottomStrip::Host host, Theme theme)
    : AudioProcessorEditor (&p), state (s), laf (std::move (theme)), strip (s, std::move (host))
{
    setLookAndFeel (&laf);

    strip.sizeBox.getItems = []
    {
        juce::StringArray items;
        for (float sc : kSizes) items.add (juce::String (juce::roundToInt (sc * 100.0f)) + "%");
        return items;
    };
    strip.sizeBox.getSelected = [this]
    {
        for (int i = 0; i < (int) std::size (kSizes); ++i)
            if (std::abs (kSizes[i] - currentScale()) < 0.01f) return i;
        return -1;
    };
    strip.sizeBox.getText = [this] { return juce::String (juce::roundToInt (currentScale() * 100.0f)) + "%"; };
    strip.sizeBox.onSelect = [this] (int i) { applyScale (kSizes[i]); };

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

    const bool open = (bool) state.state.getProperty (prop::scopeOpen, false);   // saved with the session
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
    applyScale ((float) (double) state.state.getProperty (prop::uiScale, 1.0));   // saved with the session
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

void EditorShell::setScopeOpen (bool open)
{
    state.state.setProperty (prop::scopeOpen, open, nullptr);
    goniometer.setVisible (open);
    updateConstraints();
    applyScale (currentScale());
    faceplate.repaint();
}

void EditorShell::updateConstraints()
{
    const double w = (double) contentWidth(), h = (double) baseHeight;
    constrainer.setFixedAspectRatio (w / h);
    constrainer.setSizeLimits (juce::roundToInt (w * kMinScale), juce::roundToInt (h * kMinScale),
                               juce::roundToInt (w * kMaxScale), juce::roundToInt (h * kMaxScale));
}

void EditorShell::applyScale (float scale)
{
    scale = juce::jlimit (kMinScale, kMaxScale, scale);
    setSize (juce::roundToInt ((float) contentWidth() * scale), juce::roundToInt ((float) baseHeight * scale));
}

void EditorShell::resized()
{
    if (panel == nullptr)
        return;

    // the faceplate is laid out at 100 % and scaled as a whole
    const float scale = currentScale();
    faceplate.setTransform (juce::AffineTransform::scale (scale));
    faceplate.setBounds (0, 0, contentWidth(), baseHeight);
    state.state.setProperty (prop::uiScale, scale, nullptr);
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
