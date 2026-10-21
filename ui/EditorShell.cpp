#include "EditorShell.h"
#include "plugin/SharedIds.h"

namespace ga
{
namespace
{
    constexpr float kSizes[] = { 0.75f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f };
    constexpr float kMinScale = 0.6f, kMaxScale = 2.5f;   // dragging the corner

    constexpr int kSideMargin = 26;          // left and right of the plug-in's sections and the bottom strip
    constexpr int kStripFromBottom = 36;     // top of the bottom strip, from the bottom edge
}

EditorShell::EditorShell (juce::AudioProcessor& p, juce::AudioProcessorValueTreeState& s, BottomStrip::Host host)
    : AudioProcessorEditor (&p), state (s), strip (s, std::move (host))
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

void EditorShell::setPanel (juce::Component& p, int w, int h)
{
    panel = &p;
    baseWidth = w;
    baseHeight = h;

    const bool open = (bool) state.state.getProperty (prop::scopeOpen, false);   // saved with the session
    strip.setScopeOpen (open);
    goniometer.setVisible (open);

    faceplate.addAndMakeVisible (p);
    faceplate.addAndMakeVisible (strip);
    faceplate.addChildComponent (goniometer);
    p.setBounds (0, 0, w, h);
    strip.setBounds (kSideMargin - BottomStrip::kMargin, h - kStripFromBottom - BottomStrip::kMargin,
                     w - 2 * kSideMargin + 2 * BottomStrip::kMargin, BottomStrip::kBoxHeight + 2 * BottomStrip::kMargin);
    goniometer.setBounds (scopeBox().reduced (6));
    addAndMakeVisible (faceplate);

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
    return baseWidth + (goniometer.isVisible() ? scopeBox().getRight() + kSideMargin - baseWidth : 0);
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
    const float W = (float) getWidth(), H = (float) getHeight();

    // one long faceplate
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a1b1e), 0.0f, 0.0f, juce::Colour (0xff0d0e10), 0.0f, H, false));
    g.fillAll();
    g.setColour (juce::Colour (0xff4a4d54).withAlpha (0.7f));
    g.drawLine (0.0f, 1.5f, W, 1.5f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawLine (0.0f, H - 1.5f, W, H - 1.5f, 1.0f);

    if (shell.goniometer.isVisible())
    {
        const auto box = shell.scopeBox();
        ui::drawInsetPanel (g, box.toFloat());
        g.setFont (ui::font (6.5f, true, 0.14f));
        g.setColour (ui::labelGrey);
        g.drawText ("GONIOMETER", box.getX() + 8, box.getY() + 6, 80, 9, juce::Justification::centredLeft, false);
    }

    // rack screws
    for (float x : { 12.0f, W - 12.0f })
        for (float y : { 22.0f, H - 22.0f })
            ui::drawScrew (g, x, y);
}
}
