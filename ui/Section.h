#pragma once

#include "Controls.h"

namespace ga
{
// Columns for a row of sections: `columns` equal columns, with a gap between neighbouring sections
// (not between the columns inside one). Each section spans some of the columns, left to right.
//
//   ga::ColumnGrid grid (row, 12, 14, { 7, 5 });   // Disperse | Env Follower
//   disperse.setBounds (grid.section (0));
struct ColumnGrid
{
    ColumnGrid (juce::Rectangle<int> area, int columns, int gap, std::vector<int> spans);

    juce::Rectangle<int> section (int index) const;
    int columnWidth() const { return column; }

private:
    juce::Rectangle<int> area;
    int column = 0, gap = 0;
    std::vector<int> spans;
};

// A section of a GA faceplate: the header line (title on the left, the plug-in's controls on the
// right, a rule underneath) and the body, one control per column, centred on the section's centre
// line (a knob by its dial). Controls keep their own size. The theme sets the header's type, colours
// and distances.
class Section : public juce::Component
{
public:
    explicit Section (juce::String title, int columns = 1);

    // the header's controls, right-aligned, left to right in the given order (e.g. dropdown, dice, LED)
    void setHeaderControls (std::vector<juce::Component*> leftToRight);

    // the control in a column (0 = leftmost); name is drawn under it, for controls that do not draw
    // their own (step boxes, lamps)
    void setControl (int column, juce::Component&, juce::String name = {});

    // dimmed: the header (title and its controls) while the effect is bypassed, the body (controls
    // and names) while the section is off too
    void setDimmed (bool header, bool body);

    void paint (juce::Graphics&) override;
    void resized() override;

    float columnCentre (int column) const;   // x, in the section

private:
    struct Cell
    {
        juce::Component* control = nullptr;
        juce::String name;
    };

    juce::String title;
    int columns;
    std::vector<juce::Component*> header;
    std::vector<Cell> cells;
    bool headerDimmed = false, bodyDimmed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Section)
};
}
