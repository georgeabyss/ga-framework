#pragma once

// Parameter IDs every GA plug-in shares (the output stage and settings in the bottom strip).
// Never change them: hosts and saved sessions refer to parameters by these strings.
namespace ga::pid
{
    inline constexpr const char* dryWet       = "mix";
    inline constexpr const char* bassMono     = "bassmono";
    inline constexpr const char* bypass       = "bypass";
    inline constexpr const char* osFactor     = "os";
    inline constexpr const char* osFilter     = "osfilter";
    inline constexpr const char* phaseProtect = "phaseguard";   // was called Phase Guard
}

// Editor settings that are not parameters, saved as properties of the state tree
namespace ga::prop
{
    inline constexpr const char* scopeOpen = "scopeOpen";
    inline constexpr const char* uiScale   = "uiScale";
}
