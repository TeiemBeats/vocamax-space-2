/*
    VocaMax Space 2 - audio processor
    By Mark33s Beats Lab
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
struct FactoryPreset
{
    const char* name;
    float power;
    float space;
    float low;
    float high;
    int focus;
};

/*  Order of these presets is the program order exposed to the DAW. */
const FactoryPreset factoryPresets[] =
{
    { "Default",          100.0f,  65.0f, 45.0f, 55.0f, 1 },
    { "Natural Vocals",    85.0f,  48.0f, 32.0f, 50.0f, 0 },
    { "Clear Vocals",     100.0f,  60.0f, 60.0f, 70.0f, 1 },
    { "Wide Space",       100.0f,  88.0f, 40.0f, 60.0f, 2 },
    { "Deep Pocket",      100.0f,  35.0f, 55.0f, 38.0f, 3 },
    { "Tight & Bright",   100.0f,  22.0f, 72.0f, 82.0f, 1 },
    { "Max Space",        100.0f, 100.0f, 52.0f, 62.0f, 2 },
    { "Radio Ready",      100.0f,  45.0f, 62.0f, 60.0f, 1 }
};

constexpr int numFactoryPresets = (int) (sizeof (factoryPresets) / sizeof (factoryPresets[0]));

const char* const presetIndexProperty = "presetIndex";
}

juce::AudioProcessorValueTreeState::ParameterLayout VocaMaxSpace2AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto percentParameter = [] (const char* id, const char* name, float defaultValue)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 },
            name,
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
            defaultValue,
            juce::AudioParameterFloatAttributes().withLabel ("%"));
    };

    layout.add (percentParameter ("power", "Power", 100.0f));
    layout.add (percentParameter ("space", "Space", 65.0f));
    layout.add (percentParameter ("low", "Low Frequency", 45.0f));
    layout.add (percentParameter ("high", "High Frequency", 55.0f));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "focus", 1 },
        "Vocal Focus Mode",
        juce::StringArray { "Natural", "Clear", "Wide", "Deep" },
        1));

    return layout;
}

VocaMaxSpace2AudioProcessor::VocaMaxSpace2AudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    powerParam = apvts.getRawParameterValue ("power");
    spaceParam = apvts.getRawParameterValue ("space");
    lowParam = apvts.getRawParameterValue ("low");
    highParam = apvts.getRawParameterValue ("high");
    focusParam = apvts.getRawParameterValue ("focus");

    jassert (powerParam != nullptr && spaceParam != nullptr && lowParam != nullptr
             && highParam != nullptr && focusParam != nullptr);
}

void VocaMaxSpace2AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    dsp.prepare (sampleRate);
    dsp.setParameters (readParameters());
    dsp.snapToTargets();
    dsp.reset();

    inputLevelLeft.store (0.0f);
    inputLevelRight.store (0.0f);
    outputLevelLeft.store (0.0f);
    outputLevelRight.store (0.0f);

    setLatencySamples (0);
}

void VocaMaxSpace2AudioProcessor::releaseResources()
{
    dsp.reset();
}

void VocaMaxSpace2AudioProcessor::reset()
{
    dsp.reset();
}

bool VocaMaxSpace2AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto outputLayout = layouts.getMainOutputChannelSet();

    if (outputLayout != juce::AudioChannelSet::mono()
        && outputLayout != juce::AudioChannelSet::stereo())
        return false;

    return outputLayout == layouts.getMainInputChannelSet();
}

void VocaMaxSpace2AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, numSamples);

    const int numChannels = buffer.getNumChannels();
    if (numChannels <= 0)
        return;

    float* left = buffer.getWritePointer (0);
    float* right = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    const float inLeft = buffer.getMagnitude (0, 0, numSamples);
    const float inRight = numChannels > 1 ? buffer.getMagnitude (1, 0, numSamples) : inLeft;

    dsp.setParameters (readParameters());
    dsp.process (left, right, numSamples);

    inputLevelLeft.store (inLeft);
    inputLevelRight.store (inRight);
    outputLevelLeft.store (buffer.getMagnitude (0, 0, numSamples));
    outputLevelRight.store (numChannels > 1 ? buffer.getMagnitude (1, 0, numSamples) : inputLevelLeft.load());
}

vocamax::Parameters VocaMaxSpace2AudioProcessor::readParameters() const noexcept
{
    vocamax::Parameters p;
    p.power = powerParam->load (std::memory_order_relaxed) * 0.01f;
    p.space = spaceParam->load (std::memory_order_relaxed) * 0.01f;
    p.low = lowParam->load (std::memory_order_relaxed) * 0.01f;
    p.high = highParam->load (std::memory_order_relaxed) * 0.01f;
    p.focus = juce::jlimit (0, 3, (int) std::lround (focusParam->load (std::memory_order_relaxed)));
    return p;
}

juce::StringArray VocaMaxSpace2AudioProcessor::getFactoryPresetNames()
{
    juce::StringArray names;

    for (const auto& preset : factoryPresets)
        names.add (preset.name);

    return names;
}

int VocaMaxSpace2AudioProcessor::getNumFactoryPresets()
{
    return numFactoryPresets;
}

int VocaMaxSpace2AudioProcessor::getNumPrograms()
{
    return numFactoryPresets;
}

int VocaMaxSpace2AudioProcessor::getCurrentProgram()
{
    return currentProgram.load();
}

void VocaMaxSpace2AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void VocaMaxSpace2AudioProcessor::setPercentageParameter (const char* parameterID, float percent)
{
    if (auto* parameter = apvts.getParameter (parameterID))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (percent));
}

void VocaMaxSpace2AudioProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numFactoryPresets)
        return;

    const auto& preset = factoryPresets[index];

    setPercentageParameter ("power", preset.power);
    setPercentageParameter ("space", preset.space);
    setPercentageParameter ("low", preset.low);
    setPercentageParameter ("high", preset.high);

    if (auto* focusParameter = apvts.getParameter ("focus"))
        focusParameter->setValueNotifyingHost (focusParameter->convertTo0to1 ((float) preset.focus));

    currentProgram.store (index);
}

const juce::String VocaMaxSpace2AudioProcessor::getProgramName (int index)
{
    if (index >= 0 && index < numFactoryPresets)
        return factoryPresets[index].name;

    return "Default";
}

void VocaMaxSpace2AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty (presetIndexProperty, getCurrentProgram(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void VocaMaxSpace2AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto tree = juce::ValueTree::fromXml (*xml);
    if (! tree.isValid())
        return;

    const int storedProgram = (int) tree.getProperty (presetIndexProperty, 0);
    apvts.replaceState (tree);
    currentProgram.store (juce::jlimit (0, numFactoryPresets - 1, storedProgram));
}

juce::AudioProcessorEditor* VocaMaxSpace2AudioProcessor::createEditor()
{
    return new VocaMaxSpace2AudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocaMaxSpace2AudioProcessor();
}
