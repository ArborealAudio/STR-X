/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "zig/processor.h"

//==============================================================================
STRXAudioProcessor::STRXAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", AudioChannelSet::stereo(), true)
#endif
                         ),
      apvts(*this, nullptr, "Parameters", createParameters())

#endif
{
    lastUIWidth = 775;
    lastUIHeight = 500;

    proc = processor_init(getTotalNumInputChannels());
    assert(proc);
    apvts.addParameterListener("amp_on", this);
    apvts.addParameterListener("amp_type", this);
    apvts.addParameterListener("amp_mode", this);
    apvts.addParameterListener("gain_ch", this);
    apvts.addParameterListener("bright", this);
    apvts.addParameterListener("pedal_on", this);
    apvts.addParameterListener("pedal_type", this);
    apvts.addParameterListener("pedal_gain", this);
    apvts.addParameterListener("pedal_tone", this);
    apvts.addParameterListener("pedal_output", this);
    apvts.addParameterListener("preamp_gain", this);
    apvts.addParameterListener("low_gain", this);
    apvts.addParameterListener("hi_gain", this);
    apvts.addParameterListener("bass", this);
    apvts.addParameterListener("mid", this);
    apvts.addParameterListener("treble", this);
    apvts.addParameterListener("presence", this);
    apvts.addParameterListener("master_gain", this);
    apvts.addParameterListener("out_vol", this);
}

STRXAudioProcessor::~STRXAudioProcessor()
{
    apvts.removeParameterListener("amp_on", this);
    apvts.removeParameterListener("amp_type", this);
    apvts.removeParameterListener("amp_mode", this);
    apvts.removeParameterListener("gain_ch", this);
    apvts.removeParameterListener("bright", this);
    apvts.removeParameterListener("pedal_on", this);
    apvts.removeParameterListener("pedal_type", this);
    apvts.removeParameterListener("pedal_gain", this);
    apvts.removeParameterListener("pedal_tone", this);
    apvts.removeParameterListener("pedal_output", this);
    apvts.removeParameterListener("preamp_gain", this);
    apvts.removeParameterListener("low_gain", this);
    apvts.removeParameterListener("hi_gain", this);
    apvts.removeParameterListener("bass", this);
    apvts.removeParameterListener("mid", this);
    apvts.removeParameterListener("treble", this);
    apvts.removeParameterListener("presence", this);
    apvts.removeParameterListener("master_gain", this);
    apvts.removeParameterListener("out_vol", this);
    processor_deinit(proc);
}

//==============================================================================
const String STRXAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool STRXAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool STRXAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool STRXAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double STRXAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int STRXAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int STRXAudioProcessor::getCurrentProgram()
{
    return 0;
}

void STRXAudioProcessor::setCurrentProgram(int index)
{
}

const String STRXAudioProcessor::getProgramName(int index)
{
    return "Default";
}

void STRXAudioProcessor::changeProgramName(int index, const String &newName)
{
}

//==============================================================================
void STRXAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    processor_prepare(proc, sampleRate, samplesPerBlock, getTotalNumInputChannels());
    updateOversample();
}

void STRXAudioProcessor::releaseResources()
{
    processor_reset(proc);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool STRXAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

void STRXAudioProcessor::updateOversample()
{
}

void STRXAudioProcessor::parameterChanged(const String &parameterID, float val)
{
    processor_param_change(proc, parameterID.toRawUTF8(), val);
}

void STRXAudioProcessor::processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    processor_process(proc, buffer.getArrayOfWritePointers(), buffer.getNumSamples(), buffer.getNumChannels());
}

void STRXAudioProcessor::processBlock(AudioBuffer<double> &buffer, MidiBuffer &)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
}

void STRXAudioProcessor::processDoubleBuffer(AudioBuffer<double> &buffer)
{}

//==============================================================================
bool STRXAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor *STRXAudioProcessor::createEditor()
{
    return new STRXAudioProcessorEditor(*this);
    // return new GenericAudioProcessorEditor(*this);
}

//==============================================================================
void STRXAudioProcessor::getStateInformation(MemoryBlock &destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    xml->setAttribute("uiWidth", lastUIWidth);
    xml->setAttribute("uiHeight", lastUIHeight);
    copyXmlToBinary(*xml, destData);
}

void STRXAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        lastUIWidth = xmlState->getIntAttribute("uiWidth", lastUIWidth);
        lastUIHeight = xmlState->getIntAttribute("uiHeight", lastUIHeight);
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(ValueTree::fromXml(*xmlState));
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new STRXAudioProcessor();
}

AudioProcessorValueTreeState::ParameterLayout STRXAudioProcessor::createParameters()
{
    NormalisableRange<float> nRange(0.f, 10.f, 0.1f, 1.f, true);
    nRange.setSkewForCentre(5.f);

    NormalisableRange<float> gainRange(1.f, 10.f, 0.1f, 1.f, true);
    gainRange.setSkewForCentre(5.f);

    NormalisableRange<float> outVolRange(-24.f, 24.f, 0.1f, 1.f, false);

    std::vector<std::unique_ptr<RangedAudioParameter>> params;
    
    using fParam = strix::FloatParameter;
    using bParam = strix::BoolParameter;
    using cParam = strix::ChoiceParameter;

    params.push_back(std::make_unique<bParam>(ParameterID("amp_on", 1), "Amp On", true));
    params.push_back(std::make_unique<cParam>(ParameterID("amp_type", 1), "Amp", StringArray{
                                                  "STR_X",
                                                  "STR_Y",
                                                  "STR_Z",
                                              }, 0));
    params.push_back(std::make_unique<fParam>(ParameterID("preamp_gain", 1), "Preamp Gain", gainRange, 3.f));
    params.push_back(std::make_unique<fParam>(ParameterID("low_gain", 1), "Low Gain", nRange, 5.f));
    params.push_back(std::make_unique<fParam>(ParameterID("hi_gain", 1), "High Gain", nRange, 5.f));
    params.push_back(std::make_unique<cParam>(ParameterID("amp_mode", 1), "Mode",
                                                  StringArray{"Thick", "Normal", "Open"}, 1));
    params.push_back(std::make_unique<fParam>(ParameterID("bass", 1), "Bass", nRange, 5.f));
    params.push_back(std::make_unique<fParam>(ParameterID("mid", 1), "Mid", nRange, 5.f));
    params.push_back(std::make_unique<fParam>(ParameterID("treble", 1), "Treble", nRange, 5.f));
    params.push_back(std::make_unique<fParam>(ParameterID("presence", 1), "Presence", nRange, 5.f));
    params.push_back(std::make_unique<bParam>(ParameterID("bright", 1), "Bright", false));
    params.push_back(std::make_unique<bParam>(ParameterID("pedal_on", 1), "Pedal On", false));
    params.push_back(std::make_unique<cParam>(ParameterID("pedal_type", 1), "Pedal",
                                                  StringArray{"TSX", "RXT"}, 0));
    params.push_back(std::make_unique<fParam>(ParameterID("pedal_gain", 1), "Pedal Gain", 0.f, 10.0f, 0.f));
    params.push_back(std::make_unique<fParam>(ParameterID("pedal_tone", 1), "Pedal Tone", 0.f, 10.0f, 0.f));
    params.push_back(std::make_unique<fParam>(ParameterID("pedal_output", 1), "Pedal Output", 0.f, 10.0f, 5.f));
    params.push_back(std::make_unique<fParam>(ParameterID("master_gain", 1), "Master Gain", gainRange, 5.f));
    params.push_back(std::make_unique<cParam>(ParameterID("gain_ch", 1), "Channel",
                                              StringArray{"Lo", "Hi"}, 1));
    params.push_back(std::make_unique<fParam>(ParameterID("out_vol", 1), "Output Volume", outVolRange, 0.0));
    params.push_back(std::make_unique<cParam>(ParameterID("stereo", 1), "Mono/Stereo", 
                                              StringArray{"Mono", "Stereo"}, 0));
    
    return {params.begin(), params.end()};
}
