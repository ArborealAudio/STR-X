/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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
      apvts(*this, nullptr, "Parameters", createParameters()),
      stereoAmp(apvts), monoAmp(apvts)

#endif
{
    oversample.emplace_back(std::make_unique<dsp::Oversampling<double>>(2));
    oversample.emplace_back(std::make_unique<dsp::Oversampling<double>>(2, 2, dsp::Oversampling<double>::FilterType::filterHalfBandPolyphaseIIR, false, true));
    oversample.emplace_back(std::make_unique<dsp::Oversampling<double>>(2, 2, dsp::Oversampling<double>::FilterType::filterHalfBandFIREquiripple, true, true));

    lastUIWidth = 775;
    lastUIHeight = 500;
    hq = static_cast<strix::BoolParameter*>(apvts.getParameter("hq"));
    renderHQ = static_cast<strix::BoolParameter*>(apvts.getParameter("renderHQ"));
    stereo = static_cast<strix::ChoiceParameter*>(apvts.getParameter("stereo"));
    outVol_dB = static_cast<strix::FloatParameter*>(apvts.getParameter("outVol"));
    apvts.addParameterListener("mode", this);
    apvts.addParameterListener("legacyTone", this);
    apvts.addParameterListener("hq", this);
    apvts.addParameterListener("renderHQ", this);
}

STRXAudioProcessor::~STRXAudioProcessor()
{
    apvts.removeParameterListener("hq", this);
    apvts.removeParameterListener("renderHQ", this);
    apvts.removeParameterListener("mode", this);
    apvts.removeParameterListener("legacyTone", this);
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
    lastDownSampleRate = sampleRate;
    numSamples = samplesPerBlock;

    updateOversample();

    dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock * (isOversampled ? 4.0 : 1.0);
    spec.sampleRate = lastSampleRate;
    spec.numChannels = getTotalNumInputChannels();

    for (auto &ovs : oversample)
        ovs->initProcessing(samplesPerBlock);

    stereoAmp.prepare(spec);
    monoAmp.prepare(spec);
    stereoAmp.preAmp.updateCrossover(*apvts.getRawParameterValue("mode"));
    monoAmp.preAmp.updateCrossover(*apvts.getRawParameterValue("mode"));

    doubleBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);

    simd.setInterleavedBlockSize(spec.numChannels, spec.maximumBlockSize);
}

void STRXAudioProcessor::releaseResources()
{
    for (auto &oversampler : oversample)
        oversampler->reset();

    stereoAmp.reset();
    monoAmp.reset();
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
    if (*renderHQ && isNonRealtime())
    {
        osIndex = 2;
        lastSampleRate = 4.0 * lastDownSampleRate;
        isOversampled = true;
    }
    else if (*hq)
    {
        osIndex = 1;
        lastSampleRate = 4.0 * lastDownSampleRate;
        isOversampled = true;
    }
    else
    {
        osIndex = 0;
        lastSampleRate = lastDownSampleRate;
        isOversampled = false;
    }
}

void STRXAudioProcessor::parameterChanged(const String &parameterID, float)
{
    {
        std::unique_lock<std::mutex> lock(mutex);
        msgs.emplace(parameterID);
    }
    newMessages = true;
}

void STRXAudioProcessor::processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    doubleBuffer.makeCopyOf(buffer, true);

    processDoubleBuffer(doubleBuffer);

    buffer.makeCopyOf(doubleBuffer, true);
}

void STRXAudioProcessor::processBlock(AudioBuffer<double> &buffer, MidiBuffer &)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    processDoubleBuffer(buffer);
}

void STRXAudioProcessor::processDoubleBuffer(AudioBuffer<double> &buffer)
{
    if (newMessages)
        handleMessage();

    float out_raw = std::pow(10, (*outVol_dB * 0.05f));

    dsp::AudioBlock<double> block(buffer);

    auto osBlock = oversample[osIndex]->processSamplesUp(block);

    if (stereo->getIndex())
    {
        auto simdBlock = simd.interleaveBlock(osBlock);
        stereoAmp.processAmp(simdBlock);
        simd.deinterleaveBlock(simdBlock);
    }
    else
    {
        auto mono = osBlock.getSingleChannelBlock(0);
        monoAmp.processAmp(mono);
        FloatVectorOperations::copy(osBlock.getChannelPointer(1), mono.getChannelPointer(0), mono.getNumSamples());
    }

    oversample[osIndex]->processSamplesDown(block);
    block *= out_raw;

    setLatencySamples(oversample[osIndex]->getLatencyInSamples());
}

//==============================================================================
bool STRXAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor *STRXAudioProcessor::createEditor()
{
    return new STRXAudioProcessorEditor(*this);
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
    NormalisableRange<float> nRange(0.f, 10.f, 0.1, 1.0, true);
    nRange.setSkewForCentre(5.0);

    NormalisableRange<float> gainRange(1.f, 10.f, 0.1, 1.0, true);
    gainRange.setSkewForCentre(5.0);

    NormalisableRange<float> outVolRange(-20.f, 12.f, 0.1, 1.0, false);

    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    using fParam = strix::FloatParameter;
    using bParam = strix::BoolParameter;
    using cParam = strix::ChoiceParameter;

    params.push_back(std::make_unique<fParam>(ParameterID("gain", 1), "Preamp Gain", gainRange, 3.f));
    params.push_back(std::make_unique<cParam>(ParameterID("mode", 1), "Mode", StringArray{"Thick", "Normal", "Open"}, 1));
    params.push_back(std::make_unique<fParam>(ParameterID("bass", 1), "Bass", nRange, 5.f));
    params.push_back(std::make_unique<fParam>(ParameterID("mid", 1), "Mid", nRange, 5.f));
    params.push_back(std::make_unique<fParam>(ParameterID("treble", 1), "Treble", nRange, 5.f));
    params.push_back(std::make_unique<fParam>(ParameterID("presence", 1), "Presence", nRange, 5.f));
    params.push_back(std::make_unique<bParam>(ParameterID("bright", 1), "Bright", false));
    params.push_back(std::make_unique<fParam>(ParameterID("tsXgain", 1), "X Gain", 0.f, 10.0f, 0.f));
    params.push_back(std::make_unique<fParam>(ParameterID("master", 1), "Power Amp Gain", gainRange, 5.f));
    params.push_back(std::make_unique<cParam>(ParameterID("channel", 1), "Channel", StringArray{"Lo", "Hi"}, 1));
    params.push_back(std::make_unique<fParam>(ParameterID("outVol", 1), "Output Volume", outVolRange, 0.0));
    params.push_back(std::make_unique<bParam>(ParameterID("hq", 1), "HQ", false));
    params.push_back(std::make_unique<bParam>(ParameterID("renderHQ", 1), "Render HQ", false));
    params.push_back(std::make_unique<bParam>(ParameterID("legacyTone", 1), "Use Legacy Tone Controls", false));
    params.push_back(std::make_unique<cParam>(ParameterID("stereo", 1), "Mono/Stereo", StringArray{"Mono", "Stereo"}, 0));

    return {params.begin(), params.end()};
}