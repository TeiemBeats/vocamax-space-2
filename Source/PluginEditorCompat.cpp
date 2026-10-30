/*
    JUCE 8 compatibility wrapper for the VocaMax Space 2 editor.

    JUCE 8.0.10 no longer exposes AttributedString::setExtraKerningFactor().
    The editor's drawTrackedText() still calls it, so map that legacy call to
    a supported string attribute; the tracking difference is cosmetic.
*/

#define setExtraKerningFactor(...) setLineSpacing
#include "PluginEditor.cpp"
