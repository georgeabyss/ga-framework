#pragma once

#include "BottomStrip.h"

namespace ga
{
// The editor every GA plug-in shares: one long faceplate with the plug-in's own panel on it, the
// bottom strip along its bottom edge and the goniometer opening to the right. Laid out at 100 % and
// scaled as a whole: drag the corner (fixed aspect ratio) or pick a size in the bottom strip.
// The scale and whether the goniometer is open are saved with the session.
//
//   class MyEditor : public ga::EditorShell
//   {
//       MyEditor (MyProcessor& p) : ga::EditorShell (p, p.apvts, host), panel (p) { setPanel (panel, 802, 192); }
//       MyPanel panel;   // transparent; draws the title and its own sections
//   };
class EditorShell : public juce::AudioProcessorEditor
{
public:
    EditorShell (juce::AudioProcessor&, juce::AudioProcessorValueTreeState&, BottomStrip::Host);
    ~EditorShell() override;

    void resized() override;

protected:
    // call once from the plug-in's editor constructor; the panel's size at 100 % includes the bottom strip
    void setPanel (juce::Component& panel, int baseWidth, int baseHeight);

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
    juce::Component* panel = nullptr;
    int baseWidth = 0, baseHeight = 0;
    juce::ComponentBoundsConstrainer constrainer;
    juce::TooltipWindow tooltips { this, 900 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditorShell)
};
}
