/*
    VocaMax Space 2 - audio processor
    By Mark33s Beats Lab
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/VocaMaxDSP.h"

#include <atomic>
#include <cmath>

class VocaMaxSpace2AudioProcessor : public juce::AudioProcessor
{
public:
    VocaMaxSpace2AudioProcessor();
    ~VocaMaxSpace2AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static juce::StringArray getFactoryPresetNames();
    static int getNumFactoryPresets();

    /*  Meter values written by the audio thread, read by the editor. */
    std::atomic<float> inputLevelLeft { 0.0f };
    std::atomic<float> inputLevelRight { 0.0f };
    std::atomic<float> outputLevelLeft { 0.0f };
    std::atomic<float> outputLevelRight { 0.0f };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    vocamax::Parameters readParameters() const noexcept;
    void setPercentageParameter (const char* parameterID, float percent);

    vocamax::Core dsp;

    std::atomic<float>* powerParam = nullptr;
    std::atomic<float>* spaceParam = nullptr;
    std::atomic<float>* lowParam = nullptr;
    std::atomic<float>* highParam = nullptr;
    std::atomic<float>* focusParam = nullptr;

    std::atomic<int> currentProgram { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocaMaxSpace2AudioProcessor)
};
