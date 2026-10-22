#pragma once

#include "BottomStrip.h"

namespace ga
{
// The plug-in's panel on the editor shell: transparent, as large as the window at 100 %. It gets the
// content area once (the window minus the plug-in's margin) and lays out everything in it, the
// bottom strip included.
class Panel : public juce::Component
{
public:
    // called once, before the panel gets its size
    virtual void layout (juce::Rectangle<int> content, BottomStrip&) = 0;
};

// The editor every GA plug-in shares: one long faceplate with the plug-in's panel on it, the bottom
// strip (placed by the panel) and the goniometer opening to the right. Laid out at 100 % and scaled
// as a whole: drag the corner (fixed aspect ratio) or pick a size in the bottom strip. The scale and
// whether the goniometer is open are saved with the session.
//
//   class MyEditor : public ga::EditorShell
//   {
//       MyEditor (MyProcessor& p) : ga::EditorShell (p, p.apvts, host), panel (p) { setPanel (panel, 802, 192, 26, 8); }
//       MyPanel panel;   // a ga::Panel: draws and places its own sections and the bottom strip
//   };
class EditorShell : public juce::AudioProcessorEditor
{
public:
    EditorShell (juce::AudioProcessor&, juce::AudioProcessorValueTreeState&, BottomStrip::Host, Theme = {});
    ~EditorShell() override;

    void resized() override;

protected:
    // call once from the plug-in's editor constructor: the window size at 100 % and the margin
    // around the content, left and right (marginX) and top and bottom (marginY)
    void setPanel (Panel& panel, int baseWidth, int baseHeight, int marginX, int marginY);

private:
    // the faceplate itself: background, rack screws and the goniometer box
    struct Faceplate : public juce::Component
    {
        EditorShell& shell;
        explicit Faceplate (EditorShell& s) : shell (s) {}
        void paint (juce::Graphics&) override;
    };

    int contentWidth() const;   // at 100 %, wider while the goniometer is open
    juce::Rectangle<int> scopeBox() const;
    void setScopeOpen (bool open);
    void updateConstraints();
    void applyScale (float scale);
    float currentScale() const { return (float) getHeight() / (float) baseHeight; }

    juce::AudioProcessorValueTreeState& state;
    ga::LookAndFeel laf;
    Faceplate faceplate { *this };
    BottomStrip strip;
    Goniometer goniometer;
    Panel* panel = nullptr;
    int baseWidth = 0, baseHeight = 0, marginX = 0;
    juce::ComponentBoundsConstrainer constrainer;
    juce::TooltipWindow tooltips { this, 900 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditorShell)
};
}
