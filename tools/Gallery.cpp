// The control gallery: every framework control in its states, in the GA theme, rendered at 2x into
// one PNG per sheet. The images in docs/controls are the reference; a framework change that alters
// one of them shows up in `check`.
//
//   <plugin>Gallery <dir>          write the sheets into <dir> (docs/controls)
//   <plugin>Gallery check <dir>    compare with them; any differing pixel fails

#include "ui/Section.h"

namespace
{
    using namespace juce;

    constexpr float kScale = 2.0f;

    // A sheet: controls on a piece of faceplate, each example with a caption above it.
    struct Sheet : public Component
    {
        explicit Sheet (int w, int h) { setSize (w, h); }

        void paint (Graphics& g) override
        {
            const auto& t = ga::themeOf (*this);
            t.looks.faceplate->paint (g, getLocalBounds().toFloat(), t);
            g.setFont (t.font (t.type.panelTitle));
            g.setColour (t.colours.label.withAlpha (0.6f));
            for (auto& c : captions)
                g.drawText (c.second, c.first, Justification::centredLeft, false);
        }

        void caption (Rectangle<int> r, const String& text) { captions.push_back ({ r, text }); }

        // owns what it shows: parameters, controls
        template <class T, typename... Args>
        T& make (Args&&... args)
        {
            auto p = std::make_unique<T> (std::forward<Args> (args)...);
            auto& ref = *p;
            owned.push_back (std::shared_ptr<void> (p.release(), [] (void* v) { delete static_cast<T*> (v); }));
            return ref;
        }

        std::vector<std::pair<Rectangle<int>, String>> captions;
        std::vector<std::shared_ptr<void>> owned;
    };

    RangedAudioParameter& percent (Sheet& s, const char* id, float value)
    {
        auto& p = s.make<AudioParameterFloat> (ParameterID { id, 1 }, id, NormalisableRange<float> (0.0f, 100.0f, 0.1f), value,
                                               AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " %"; }));
        return p;
    }
    RangedAudioParameter& choice (Sheet& s, const char* id, StringArray items, int value)
    {
        return s.make<AudioParameterChoice> (ParameterID { id, 1 }, id, items, value);
    }

    // ---- the sheets, one per framework control (or family)
    std::unique_ptr<Sheet> sectionSheet()
    {
        auto sheet = std::make_unique<Sheet> (420, 270);
        auto& s = *sheet;

        auto addSection = [&s] (int y, const String& caption, bool dimHeader, bool dimBody) -> ga::Section&
        {
            auto& section = s.make<ga::Section> ("DISPERSE", 4);
            auto& dropdown = s.make<ga::StepBox> (choice (s, "ch", { "LINK", "M", "S" }, 0), true);
            auto& dice = s.make<ga::DiceButton>();
            auto& led = s.make<ga::SectionLed>();
            section.setHeaderControls ({ &dropdown, &dice, &led });
            section.setControl (0, s.make<ga::StepBox> (choice (s, "mode", { "Behaved", "Unhinged" }, 1), true), "Mode");
            section.setControl (1, s.make<ga::LampButton>(), "Jitter");
            section.setControl (2, s.make<ga::Knob> (percent (s, "pinch", 65.0f), "Pinch"));
            section.setControl (3, s.make<ga::Knob> (percent (s, "mix", 100.0f), "Mix"));
            section.setBounds (14, y, 4 * 66, 100);
            section.setDimmed (dimHeader, dimBody);
            s.addAndMakeVisible (section);
            s.caption ({ 14, y - 12, 300, 10 }, caption);
            return section;
        };
        addSection (20, "SECTION: HEADER CONTROLS RIGHT-ALIGNED, ONE CONTROL PER COLUMN", false, false);
        addSection (150, "SECTION OFF (BODY DIMMED)", false, true);

        // dice: normal, hovered
        auto& dice = s.make<ga::DiceButton>();
        auto& diceOver = s.make<ga::DiceButton>();
        dice.setTopLeftPosition (310, 40);
        diceOver.setTopLeftPosition (340, 40);
        diceOver.setState (Button::buttonOver);
        s.addAndMakeVisible (dice);
        s.addAndMakeVisible (diceOver);
        s.caption ({ 300, 20, 110, 10 }, "DICE, HOVERED");
        return sheet;
    }

    // Stages: a number box the size of a drop-down; arrows double / halve, dragging moves single steps
    std::unique_ptr<Sheet> stagesSheet()
    {
        auto sheet = std::make_unique<Sheet> (340, 70);
        auto& s = *sheet;
        auto doubleOrHalve = [] (int v, int direction)
        {
            return direction > 0 ? (v >= 512 ? 1000 : v * 2) : (v == 1000 ? 512 : juce::jmax (1, v / 2));
        };
        int x = 14;
        for (float value : { 64.0f, 1000.0f, 1.0f })
        {
            auto& p = s.make<AudioParameterFloat> (ParameterID { "stages", 1 }, "Stages", NormalisableRange<float> (1.0f, 1000.0f, 1.0f), value);
            auto& box = s.make<ga::StepBox> (p);
            box.setWideNumber (doubleOrHalve, 3.0f);
            box.setTopLeftPosition (x, 32);
            s.addAndMakeVisible (box);
            x += 76;
        }
        auto& dropdown = s.make<ga::StepBox> (choice (s, "mode", { "Behaved", "Unhinged" }, 0), true);
        dropdown.setTopLeftPosition (x + 10, 32);
        s.addAndMakeVisible (dropdown);
        s.caption ({ 14, 14, 220, 10 }, "STAGES BOX: 64, 1000, 1");
        s.caption ({ x + 10, 14, 90, 10 }, "DROP-DOWN");
        return sheet;
    }

    // Solo: off, hovered, on
    std::unique_ptr<Sheet> soloSheet()
    {
        auto sheet = std::make_unique<Sheet> (200, 64);
        auto& s = *sheet;
        int x = 20;
        for (int state : { 0, 1, 2 })
        {
            auto& solo = s.make<ga::SoloButton>();
            solo.setToggleState (state == 2, dontSendNotification);
            if (state == 1)
                solo.setState (Button::buttonOver);
            solo.setTopLeftPosition (x, 32);
            s.addAndMakeVisible (solo);
            x += 50;
        }
        s.caption ({ 14, 14, 180, 10 }, "SOLO: OFF, HOVERED, ON");
        return sheet;
    }

    struct SheetDef
    {
        const char* name;
        std::function<std::unique_ptr<Sheet>()> make;
    };

    const std::vector<SheetDef>& sheets()
    {
        static const std::vector<SheetDef> all {
            { "section", sectionSheet },
            { "stages", stagesSheet },
            { "solo", soloSheet },
        };
        return all;
    }

    Image render (const SheetDef& def)
    {
        ga::LookAndFeel laf;   // the GA theme
        auto sheet = def.make();
        sheet->setLookAndFeel (&laf);
        auto image = sheet->createComponentSnapshot (sheet->getLocalBounds(), true, kScale);
        sheet->setLookAndFeel (nullptr);
        return image;
    }

    bool same (const Image& a, const Image& b)
    {
        if (a.getWidth() != b.getWidth() || a.getHeight() != b.getHeight())
            return false;
        for (int y = 0; y < a.getHeight(); ++y)
            for (int x = 0; x < a.getWidth(); ++x)
                if (a.getPixelAt (x, y) != b.getPixelAt (x, y))
                    return false;
        return true;
    }
}

// called from tools/GalleryMain.cpp
int gaRenderGallery (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI gui;
    const bool check = argc > 1 && String (argv[1]) == "check";
    const File dir (File::getCurrentWorkingDirectory().getChildFile (argc > (check ? 2 : 1) ? argv[check ? 2 : 1] : "docs/controls"));
    dir.createDirectory();

    int failures = 0;
    for (const auto& def : sheets())
    {
        const auto image = render (def);
        const auto file = dir.getChildFile (String (def.name) + ".png");
        // compare after the same PNG round trip the reference went through
        MemoryOutputStream png;
        PNGImageFormat().writeImageToStream (image, png);
        if (check)
        {
            const bool ok = same (ImageFileFormat::loadFrom (file), ImageFileFormat::loadFrom (png.getData(), png.getDataSize()));
            std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", def.name);
            failures += ok ? 0 : 1;
        }
        else
        {
            file.replaceWithData (png.getData(), png.getDataSize());
            std::printf ("wrote %s (%d x %d)\n", file.getFullPathName().toRawUTF8(), image.getWidth(), image.getHeight());
        }
    }
    if (check)
        std::printf ("\n%s\n", failures == 0 ? "All sheets match." : "Sheets differ.");
    return failures == 0 ? 0 : 1;
}
