/*
    VocaMax Space 2 - editor
    By Mark33s Beats Lab
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

#include <atomic>
#include <memory>
#include <vector>

class VocaMaxLookAndFeel;
class VocaMaxKnobPanel;
class VocaMaxLevelMeter;
class VocaMaxFocusSelector;
class VocaMaxIconButton;

class VocaMaxSpace2AudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit VocaMaxSpace2AudioProcessorEditor (VocaMaxSpace2AudioProcessor&);
    ~VocaMaxSpace2AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    void syncFromProcessor();
    void savePresetFile();
    void showSettings();
    void showHelp();

    void paintBackground (juce::Graphics&);
    void paintHeader (juce::Graphics&);
    void paintPanel (juce::Graphics&);
    void paintFooter (juce::Graphics&);

    VocaMaxSpace2AudioProcessor& processor;

    std::unique_ptr<VocaMaxLookAndFeel> lookAndFeel;

    std::unique_ptr<VocaMaxKnobPanel> lowKnob;
    std::unique_ptr<VocaMaxKnobPanel> powerKnob;
    std::unique_ptr<VocaMaxKnobPanel> spaceKnob;
    std::unique_ptr<VocaMaxKnobPanel> highKnob;

    std::unique_ptr<VocaMaxLevelMeter> inputMeter;
    std::unique_ptr<VocaMaxLevelMeter> outputMeter;
    std::unique_ptr<VocaMaxFocusSelector> focusSelector;

    std::unique_ptr<juce::ComboBox> presetBox;
    std::unique_ptr<VocaMaxIconButton> previousPresetButton;
    std::unique_ptr<VocaMaxIconButton> nextPresetButton;
    std::unique_ptr<VocaMaxIconButton> saveButton;
    std::unique_ptr<VocaMaxIconButton> settingsButton;
    std::unique_ptr<VocaMaxIconButton> helpButton;

    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    std::unique_ptr<juce::FileChooser> fileChooser;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> powerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> spaceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highAttachment;

    std::atomic<float>* focusValue = nullptr;
    int lastFocus = -1;

    juce::Rectangle<float> logoBounds;
    juce::Rectangle<float> headerControlsBounds;
    juce::Rectangle<float> panelBounds;
    juce::Rectangle<float> knobRowBounds;
    juce::Rectangle<float> stripBounds;

    std::vector<juce::Point<float>> starPositions;
    std::vector<float> starSizes;
    std::vector<float> starAlphas;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocaMaxSpace2AudioProcessorEditor)
};
