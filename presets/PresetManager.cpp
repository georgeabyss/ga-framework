#include "PresetManager.h"
#include "plugin/SharedIds.h"

namespace ga
{

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state, Config c)
    : apvts (state), config (std::move (c))
{
    rescan();
}

void PresetManager::rescan()
{
    presets.clear();
    for (auto& f : config.factory)
    {
        Preset p;
        p.name = f.name;
        for (auto& v : f.values)
            p.values[v.first] = v.second;
        if (config.listTag != nullptr)
            p.listTag = config.listTag (*this, p);
        presets.push_back (std::move (p));
    }

    auto files = folder().findChildFiles (juce::File::findFiles, false, "*" + config.extension);
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileName().compareNatural (b.getFileName()) < 0; });
    for (auto& file : files)
    {
        const auto xml = juce::XmlDocument::parse (file);
        if (xml == nullptr || ! xml->hasTagName (config.fileTag))
            continue;

        Preset p;
        p.isFactory = false;
        p.file = file;
        p.name = xml->getStringAttribute ("name", file.getFileNameWithoutExtension());
        for (auto* e : xml->getChildWithTagNameIterator ("PARAM"))
            p.values[e->getStringAttribute ("id")] = (float) e->getDoubleAttribute ("value");
        if (config.listTag != nullptr)
            p.listTag = config.listTag (*this, p);
        presets.push_back (std::move (p));
    }
}

float PresetManager::valueOf (const Preset& p, const char* id) const
{
    if (auto it = p.values.find (id); it != p.values.end())
        return it->second;
    auto* param = apvts.getParameter (id);
    return param->convertFrom0to1 (param->getDefaultValue());
}

juce::String PresetManager::currentName() const
{
    return apvts.state.getProperty (prop::preset, config.factory.empty() ? "" : config.factory.front().name).toString();
}

int PresetManager::currentIndex() const
{
    const auto name = currentName();
    const bool isUser = apvts.state.getProperty (prop::presetIsUser, false);
    for (size_t i = 0; i < presets.size(); ++i)
        if (presets[i].name == name && presets[i].isFactory != isUser)
            return (int) i;
    return -1;
}

// the state property that locks a parameter, or nullptr if it cannot be locked
const char* PresetManager::lockProperty (const char* id) const
{
    for (auto& l : config.lockable)
        if (juce::String (id) == l.first)
            return l.second;
    return nullptr;
}

bool PresetManager::isLockable (const char* id) const
{
    return lockProperty (id) != nullptr;
}

bool PresetManager::isLocked (const char* id) const
{
    const auto* property = lockProperty (id);
    return property != nullptr && (bool) apvts.state.getProperty (property, false);
}

void PresetManager::setLocked (const char* id, bool locked)
{
    if (const auto* property = lockProperty (id))
        apvts.state.setProperty (property, locked, nullptr);
}

bool PresetManager::isModified() const
{
    const int i = currentIndex();
    if (i < 0)
        return true;
    for (auto* id : config.parameters)
    {
        if (isLocked (id))   // a locked value is the user's, not the preset's
            continue;
        auto* param = apvts.getParameter (id);
        if (std::abs (param->getValue() - param->convertTo0to1 (valueOf (presets[(size_t) i], id))) > 1.0e-4f)
            return true;
    }
    return false;
}

void PresetManager::setCurrent (const Preset& p)
{
    apvts.state.setProperty (prop::preset, p.name, nullptr);
    apvts.state.setProperty (prop::presetIsUser, ! p.isFactory, nullptr);
}

void PresetManager::load (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return;
    const auto& p = presets[(size_t) index];
    for (auto* id : config.parameters)
    {
        if (isLocked (id))
            continue;
        auto* param = apvts.getParameter (id);
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (valueOf (p, id)));
        param->endChangeGesture();
    }
    setCurrent (p);
}

void PresetManager::step (int delta)
{
    const int n = (int) presets.size();
    if (n == 0)
        return;
    const int i = currentIndex();
    load (i < 0 ? (delta > 0 ? 0 : n - 1) : ((i + delta) % n + n) % n);
}

bool PresetManager::save (const juce::String& name)
{
    const auto clean = name.trim();
    if (clean.isEmpty() || ! folder().createDirectory())
        return false;

    juce::XmlElement xml (config.fileTag);
    xml.setAttribute ("name", clean);
    for (auto* id : config.parameters)
    {
        auto* e = xml.createNewChildElement ("PARAM");
        e->setAttribute ("id", id);
        e->setAttribute ("value", apvts.getParameter (id)->convertFrom0to1 (apvts.getParameter (id)->getValue()));
    }
    if (! xml.writeTo (folder().getChildFile (juce::File::createLegalFileName (clean) + config.extension)))
        return false;

    rescan();
    Preset saved;
    saved.name = clean;
    saved.isFactory = false;
    setCurrent (saved);
    return true;
}

bool PresetManager::rename (int index, const juce::String& newName)
{
    const auto clean = newName.trim();
    if (! juce::isPositiveAndBelow (index, (int) presets.size()) || presets[(size_t) index].isFactory || clean.isEmpty())
        return false;

    const auto old = presets[(size_t) index];
    const bool wasCurrent = index == currentIndex();
    auto xml = juce::XmlDocument::parse (old.file);
    if (xml == nullptr)
        return false;
    xml->setAttribute ("name", clean);
    const auto target = folder().getChildFile (juce::File::createLegalFileName (clean) + config.extension);
    if (! xml->writeTo (target))
        return false;
    if (target != old.file)
        old.file.deleteFile();

    rescan();
    if (wasCurrent)
    {
        Preset renamed;
        renamed.name = clean;
        renamed.isFactory = false;
        setCurrent (renamed);
    }
    return true;
}

bool PresetManager::remove (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets.size()) || presets[(size_t) index].isFactory)
        return false;
    const bool ok = presets[(size_t) index].file.deleteFile();
    rescan();
    return ok;
}
}
