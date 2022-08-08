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
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "Parameters", createParameters())
                        
#endif
{
    lastUIWidth = 775;
    lastUIHeight = 500;
    hq = apvts.getRawParameterValue("hq");    
    renderHQ = apvts.getRawParameterValue("renderHQ");
    apvts.addParameterListener("hq", this);
    apvts.addParameterListener("renderHQ", this);
    amp.inputGain = apvts.getRawParameterValue("gain");
    amp.outGain = apvts.getRawParameterValue("master");
    amp.tsXGain = apvts.getRawParameterValue("tsXgain");
    amp.bright = apvts.getRawParameterValue("bright");
    amp.crossover = apvts.getRawParameterValue("mode");
    outVol_dB = apvts.getRawParameterValue("outVol");
    legacyTone = apvts.getRawParameterValue("legacyTone");
    apvts.addParameterListener("bass", this);
    apvts.addParameterListener("mid", this);
    apvts.addParameterListener("treble", this);
    apvts.addParameterListener("presence", this);
    apvts.addParameterListener("legacyTone", this);

    oversample[1].clearOversamplingStages();
    oversample[1].addOversamplingStage(dsp::Oversampling<double>::filterHalfBandPolyphaseIIR,
        0.02, -90.0, 0.05, -90.0);
    oversample[1].addOversamplingStage(dsp::Oversampling<double>::filterHalfBandPolyphaseIIR,
        0.02, -90.0, 0.05, -90.0);

    oversample[2].clearOversamplingStages();
    oversample[2].addOversamplingStage(dsp::Oversampling<double>::filterHalfBandFIREquiripple,
        0.02, -90.0, 0.05, -90.0);
    oversample[2].addOversamplingStage(dsp::Oversampling<double>::filterHalfBandFIREquiripple,
        0.02, -90.0, 0.05, -90.0);
}

STRXAudioProcessor::~STRXAudioProcessor()
{
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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int STRXAudioProcessor::getCurrentProgram()
{
    return 0;
}

void STRXAudioProcessor::setCurrentProgram (int index)
{
}

const String STRXAudioProcessor::getProgramName (int index)
{
    return {};
}

void STRXAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void STRXAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{   
    lastDownSampleRate = sampleRate;
    numSamples = samplesPerBlock;

    updateOversample();

    if (isOversampled == true)
        lastSampleRate = sampleRate * 4;
    else
        lastSampleRate = sampleRate;

    dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = lastSampleRate;
    spec.numChannels = getTotalNumInputChannels();

    for (auto& oversampler : oversample)
        oversampler.initProcessing(static_cast<size_t>(samplesPerBlock));

    for (auto& oversampler : oversample)
        oversampler.reset();

    amp.prepare(spec, lastSampleRate);

    doubleBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);
}

void STRXAudioProcessor::releaseResources()
{
    for (auto& oversampler : oversample)
        oversampler.reset();

    amp.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool STRXAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

void STRXAudioProcessor::updateOversample()
{
    if (*renderHQ == true && isNonRealtime() == true) {
        osIndex = 2;
        isOversampled = true;
    }
    else if (*hq == true) {
        osIndex = 1;
        isOversampled = true;
    }
    else {
        osIndex = 0;
        isOversampled = false;
    }
}

void STRXAudioProcessor::parameterChanged(const String& parameterID, float newValue)
{
    if (parameterID == "renderHQ" || parameterID == "hq") {
        if (*renderHQ == true && isNonRealtime() == true) {
            osIndex = 2;
            lastSampleRate = 4.0 * lastDownSampleRate;
            isOversampled = true;
        }
        else if (*hq == true) {
            osIndex = 1;
            lastSampleRate = 4.0 * lastDownSampleRate;
            isOversampled = true;
        }
        else {
            osIndex = 0;
            lastSampleRate = lastDownSampleRate;
            isOversampled = false;
        }

        dsp::ProcessSpec newSpec;
        newSpec.sampleRate = lastSampleRate;
        newSpec.maximumBlockSize = numSamples;
        newSpec.numChannels = getTotalNumInputChannels();

        amp.prepare(newSpec, lastSampleRate);
    }

    updateFilters();
}

void STRXAudioProcessor::updateFilters()
{    
    float bassParam = *apvts.getRawParameterValue("bass");
    float midParam = *apvts.getRawParameterValue("mid");
    float trebleParam = *apvts.getRawParameterValue("treble");
    float presenceParam = *apvts.getRawParameterValue("presence");

    bassParam /= 10;
    midParam /= 10;
    trebleParam /= 10;
    presenceParam /= 10;

    float bassCook = 0.0, midCook = 0.0, trebleCook = 0.0, presenceCook = 0.0;

    if (!*legacyTone) {
        // convert 0 - 1 value into a dB value
        bassCook = jmap(bassParam, -12.f, 12.f);
        midCook = jmap(midParam, -7.f, 7.f);
        trebleCook = jmap(trebleParam, -14.f, 14.f);
        presenceCook = jmap(presenceParam, -8.f, 8.f);

        // convert to linear multiplier
        bassCook = pow(10, (bassCook / 20));
        midCook = pow(10, (midCook / 20));
        trebleCook = pow(10, (trebleCook / 20));
        presenceCook = pow(10, (presenceCook / 20));
    }
    else {
        // convert 0 - 1 value into legacy linear values
        bassCook = cookParams(bassParam, 0.2f, 1.666f);
        midCook = cookParams(midParam, 0.3f, 2.2f);
        trebleCook = cookParams(trebleParam, 0.2f, 3.0f);
        presenceCook = cookParams(presenceParam, 0.4f, 2.5f);
    }
    
    amp.updateFilters(lastSampleRate, bassCook, midCook, trebleCook, presenceCook);
}

void STRXAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    doubleBuffer.makeCopyOf(buffer, true);

    processDoubleBuffer(doubleBuffer);

    buffer.makeCopyOf(doubleBuffer, true);
}

void STRXAudioProcessor::processBlock (AudioBuffer<double>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    processDoubleBuffer(buffer);
}

void STRXAudioProcessor::processDoubleBuffer(AudioBuffer<double>& buffer)
{
    float out_raw = pow(10, (*outVol_dB / 20));

    dsp::AudioBlock<double> block(buffer);
    dsp::ProcessContextReplacing<double> context(block);
    const auto& inBlock = context.getInputBlock();
    auto& outBlock = context.getOutputBlock();

    auto osBlock = oversample[osIndex].processSamplesUp(inBlock);
    dsp::ProcessContextReplacing<double> ampContext(osBlock);

    amp.processAmp(ampContext);

    oversample[osIndex].processSamplesDown(outBlock);
    outBlock *= out_raw;

    setLatencySamples(oversample[osIndex].getLatencyInSamples());
}

//==============================================================================
bool STRXAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* STRXAudioProcessor::createEditor()
{
    return new STRXAudioProcessorEditor (*this);
}

//==============================================================================
void STRXAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    xml->setAttribute("uiWidth", lastUIWidth);
    xml->setAttribute("uiHeight", lastUIHeight);
    copyXmlToBinary(*xml, destData);
}

void STRXAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr) {
        lastUIWidth = xmlState->getIntAttribute("uiWidth", lastUIWidth);
        lastUIHeight = xmlState->getIntAttribute("uiHeight", lastUIHeight);
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(ValueTree::fromXml(*xmlState));
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
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

    params.push_back(std::make_unique<AudioParameterFloat>
        ("gain", "Gain", gainRange, 3.f));
    params.push_back(std::make_unique <AudioParameterChoice>
        ("mode", "Mode", juce::StringArray{ "Thick", "Normal", "Open" }, 1));
    params.push_back(std::make_unique <AudioParameterFloat>
        ("bass", "Bass", nRange, 5.f));
    params.push_back(std::make_unique <AudioParameterFloat>
        ("mid", "Mid", nRange, 5.f));
    params.push_back(std::make_unique <AudioParameterFloat>
        ("treble", "Treble", nRange, 5.f));
    params.push_back(std::make_unique <AudioParameterFloat>
        ("presence", "Presence", nRange, 5.f));
    params.push_back(std::make_unique<AudioParameterBool>("bright", "Bright", false));
    params.push_back(std::make_unique <AudioParameterFloat>("tsXgain", "X Gain", 0.f, 10.0f, 0.f));
    params.push_back(std::make_unique <AudioParameterFloat>
        ("master", "Master", gainRange, 5.f));
    params.push_back(std::make_unique<AudioParameterFloat>
        ("outVol", "Output Volume", outVolRange, 0.0));
    params.push_back(std::make_unique<AudioParameterBool>("hq", "HQ", false));    
    params.push_back(std::make_unique<AudioParameterBool>("renderHQ", "Render HQ", false));
    params.push_back(std::make_unique<AudioParameterBool>("legacyTone", "Use Legacy Tone Controls", false));
    

    return { params.begin(), params.end() };  
    
}