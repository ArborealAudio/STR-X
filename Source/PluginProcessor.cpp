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
    hq = apvts.getRawParameterValue("hq");
    //apvts.addParameterListener("hq", hqListener);
    renderHQ = apvts.getRawParameterValue("renderHQ");
    //apvts.addParameterListener("renderHQ", renderListener);
    amp.inputGain = apvts.getRawParameterValue("gain");
    amp.outGain = apvts.getRawParameterValue("master");
    amp.tsXGain = apvts.getRawParameterValue("tsXgain");
    amp.bright = apvts.getRawParameterValue("bright");
    amp.crossover = apvts.getRawParameterValue("mode");
    oversample.reset(new dsp::Oversampling<float>(2));
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
    oversample->clearOversamplingStages();
    updateOversample();
    
    dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = lastSampleRate;
    spec.numChannels = getTotalNumInputChannels();

    oversample->initProcessing(static_cast<size_t>(samplesPerBlock));

    amp.prepare(spec, lastSampleRate);
    
    outVol.prepare(spec);
    
    oversample->reset();
        
    outVol.reset();
}

void STRXAudioProcessor::releaseResources()
{
    oversample->reset();
    amp.reset();
    outVol.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool STRXAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void STRXAudioProcessor::updateOversample()
{
    if (*renderHQ == true && isNonRealtime() == true) {
        oversample->addOversamplingStage(dsp::Oversampling<float>::FilterType::filterHalfBandFIREquiripple,
            0.02, -90.0, 0.05, -90.0);
        oversample->addOversamplingStage(dsp::Oversampling<float>::FilterType::filterHalfBandFIREquiripple,
            0.02, -90.0, 0.05, -90.0);
        lastSampleRate = 4 * getSampleRate();
    }
    else if (*hq == true) {
        oversample->addOversamplingStage(dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR,
            0.02, -90.0, 0.05, -90.0);
        oversample->addOversamplingStage(dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR,
            0.02, -90.0, 0.05, -90.0);
        lastSampleRate = 4 * getSampleRate();
    }
    else {
        oversample->clearOversamplingStages();
        oversample->addDummyOversamplingStage();
        lastSampleRate = getSampleRate();
    }
}

void STRXAudioProcessor::parameterChanged(const String& parameterID, float newValue)
{
    /*if (parameterID == "renderHQ" && newValue == 1 && isNonRealtime() == true) {
        oversample->addOversamplingStage(dsp::Oversampling<float>::FilterType::filterHalfBandFIREquiripple,
            0.02, -90.0, 0.05, -90.0);
        oversample->addOversamplingStage(dsp::Oversampling<float>::FilterType::filterHalfBandFIREquiripple,
            0.02, -90.0, 0.05, -90.0);
        lastSampleRate = 4 * getSampleRate();
    }
    else if (parameterID == "hq" && newValue == 1) {
        oversample->addOversamplingStage(dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR,
            0.02, -90.0, 0.05, -90.0);
        oversample->addOversamplingStage(dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR,
            0.02, -90.0, 0.05, -90.0);
        lastSampleRate = 4 * getSampleRate();
    }
    else {
        oversample->clearOversamplingStages();
        oversample->addDummyOversamplingStage();
        lastSampleRate = getSampleRate();
    }*/
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

    float bassCook = cookParams(bassParam, 0.2, 1.666);
    float midCook = cookParams(midParam, 0.3, 2.2);
    float trebleCook = cookParams(trebleParam, 0.2, 3.0);
    float presenceCook = cookParams(presenceParam, 0.4, 2.5);
    
    amp.updateFilters(lastSampleRate, bassCook, midCook, trebleCook, presenceCook);
}

float STRXAudioProcessor::updateParameters()
{
    float outVol_dB = *apvts.getRawParameterValue("outVol");

    return outVol_dB;
}

void STRXAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    updateFilters();
    float outVol_dB = updateParameters();

    dsp::AudioBlock<float> block(buffer);
    dsp::ProcessContextReplacing<float> context(block);
    const auto& inBlock = context.getInputBlock();
    auto& outBlock = context.getOutputBlock();

    auto osBlock = oversample->processSamplesUp(inBlock);
    dsp::ProcessContextReplacing<float> ampContext(osBlock);

    amp.processAmp(ampContext);
    outVol.setGainDecibels(outVol_dB);
    outVol.process(ampContext);

    oversample->processSamplesDown(outBlock);

    setLatencySamples(oversample->getLatencyInSamples());
    
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
    copyXmlToBinary(*xml, destData);
}

void STRXAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new STRXAudioProcessor();
}

AudioProcessorValueTreeState::ParameterLayout STRXAudioProcessor::createParameters()
{
    NormalisableRange<float> nRange(1.f, 10.f, 0.1, 1.0, true);
    nRange.setSkewForCentre(5.0);

    NormalisableRange<float> outVolRange(-20.f, 12.f, 0.1, 1.0, false);

    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back(std::make_unique<AudioParameterFloat>
        ("gain", "Gain", nRange, 3.f));
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
    params.push_back(std::make_unique <AudioParameterFloat>("tsXgain", "TSXGain", 0.f, 10.0f, 0.f));
    params.push_back(std::make_unique <AudioParameterFloat>
        ("master", "Master", nRange, 5.f));
    params.push_back(std::make_unique<AudioParameterFloat>
        ("outVol", "Output Volume", outVolRange, 0.0));
    params.push_back(std::make_unique<AudioParameterBool>("hq", "HQ", false));    
    params.push_back(std::make_unique<AudioParameterBool>("renderHQ", "Render HQ", false));
    

    return { params.begin(), params.end() };  
    
}