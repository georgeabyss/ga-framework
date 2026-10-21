#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ga
{
// Factory presets (built in) and user presets (one file each in the plug-in's preset folder),
// browsed as one list: factory first. A preset holds only the parameters the plug-in lists in its
// Config, typically the sound: oversampling, the anti-alias filter and Bypass stay as they are.
// Message thread only.
class PresetManager
{
public:
    struct Preset
    {
        juce::String name;
        bool isFactory = true;
        juce::String listTag;                    // shown small on the right of the list
        std::map<juce::String, float> values;    // parameter ID -> value; missing = default
        juce::File file;                         // user presets only
    };

    struct FactoryPreset
    {
        const char* name;
        std::vector<std::pair<const char*, float>> values;   // only what differs from the defaults
    };

    // what makes the presets of one plug-in; never change folder, extension or fileTag after a
    // release, or existing user presets are no longer found
    struct Config
    {
        juce::File folder;                       // user presets, e.g. ~/Music/GA/<plug-in>/Presets
        juce::String extension;                  // e.g. ".hhdpreset"
        juce::String fileTag;                    // root element of a preset file, e.g. "HHDPRESET"
        std::vector<const char*> parameters;     // the parameter IDs a preset stores
        std::vector<std::pair<const char*, const char*>> lockable;   // parameter ID -> state property holding its lock
        std::vector<FactoryPreset> factory;      // the first one is the default ("Init")
        std::function<juce::String (const PresetManager&, const Preset&)> listTag;   // optional
    };

    PresetManager (juce::AudioProcessorValueTreeState&, Config);

    const juce::File& folder() const { return config.folder; }
    void rescan();                               // re-read the user presets from the folder

    const std::vector<Preset>& all() const { return presets; }
    int currentIndex() const;                    // -1 if the current preset no longer exists
    juce::String currentName() const;
    bool isModified() const;                     // the settings differ from the preset as saved

    // the Config's lockable parameters (Dry/Wet and Bass Mono): a locked value stays when a preset
    // loads. The locks are saved with the session, not in presets.
    bool isLockable (const char* id) const;
    bool isLocked (const char* id) const;
    void setLocked (const char* id, bool locked);

    void load (int index);
    void step (int delta);                       // previous / next, wraps around
    bool save (const juce::String& name);        // as a user preset; replaces one of the same name
    bool rename (int index, const juce::String& newName);
    bool remove (int index);

    float valueOf (const Preset&, const char* id) const;   // the preset's value, or the default

private:
    const char* lockProperty (const char* id) const;
    void setCurrent (const Preset&);

    juce::AudioProcessorValueTreeState& apvts;
    Config config;
    std::vector<Preset> presets;
};
}
