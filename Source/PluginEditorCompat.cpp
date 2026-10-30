/*
    JUCE 8 compatibility wrapper for the VocaMax Space 2 editor.

    JUCE 8.0.10 no longer exposes AttributedString::setExtraKerningFactor().
    Include the editor header (and therefore all JUCE headers) first, then map
    the legacy call to a supported attribute before compiling the editor.
*/

#include "PluginEditor.h"

#define setExtraKerningFactor setLineSpacing
#include "PluginEditor.cpp"
