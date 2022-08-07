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
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "Parameters", createParameters())
#endif
{
    inputGain = apvts.getRawParameterValue("gain");
    outGain = apvts.getRawParameterValue("master");
    tsXGain = apvts.getRawParameterValue("tsXgain");
    bright = apvts.getRawParameterValue("bright");
}

STRXAudioProcessor::~STRXAudioProcessor()
{
}

//==============================================================================
const juce::String STRXAudioProcessor::getName() const
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

const juce::String STRXAudioProcessor::getProgramName (int index)
{
    return {};
}

void STRXAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void STRXAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    lastSampleRate = sampleRate;
    
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = getTotalNumOutputChannels();
    
    ts9.prepare(spec);
    highPass.prepare(spec);
    bandPass.prepare(spec);
    lowPass.prepare(spec);
    lowBand.prepare(spec);
    hiBand.prepare(spec);
    poweramp.dcRemoval.prepare(spec);
    preamp.dcRemoval.prepare(spec);
    preamp.lowShelf.prepare(spec);
    preamp.inputHPF.prepare(spec);
    bass.prepare(spec);
    mid.prepare(spec);
    treble.prepare(spec);
    presence.prepare(spec);
    brightShelf.prepare(spec);
    outVol.prepare(spec);

    lowBand.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    hiBand.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

    highPass.coefficients = (juce::dsp::IIR::Coefficients<double>::makeFirstOrderHighPass(sampleRate, 750.f));
    bandPass.coefficients = (juce::dsp::IIR::Coefficients<double>::makeBandPass(sampleRate, 80.f));
    lowPass.coefficients = (juce::dsp::IIR::Coefficients<double>::makeFirstOrderLowPass(sampleRate, 10000.f));
    brightShelf.coefficients = (juce::dsp::IIR::Coefficients<double>::makeHighShelf(sampleRate, 2500.0, 0.707, 2.0));

    poweramp.dcRemoval.coefficients = (juce::dsp::IIR::Coefficients<double>::makeHighPass(sampleRate, 10.0));
    preamp.dcRemoval.coefficients = (juce::dsp::IIR::Coefficients<double>::makeHighPass(sampleRate, 10.0));
    preamp.lowShelf.coefficients = (juce::dsp::IIR::Coefficients<double>::makeLowShelf(sampleRate, 185.0, 1.8, 0.5));
    preamp.inputHPF.coefficients = (juce::dsp::IIR::Coefficients<double>::makeHighPass(sampleRate, 65.0));

    highPass.reset();
    bandPass.reset();
    lowPass.reset();
    lowBand.reset();
    hiBand.reset();
    poweramp.dcRemoval.reset();
    preamp.dcRemoval.reset();
    preamp.inputHPF.reset();
    preamp.lowShelf.reset();
    bass.reset();
    mid.reset();
    treble.reset();
    presence.reset();
    brightShelf.reset();
    outVol.reset();
}

void STRXAudioProcessor::releaseResources()
{
    
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
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
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
    
    *bass.coefficients = *(juce::dsp::IIR::Coefficients<double>::makeLowShelf(lastSampleRate, 150.f, 0.606f,
        bassCook));
    *mid.coefficients = *(juce::dsp::IIR::Coefficients<double>::makePeakFilter(lastSampleRate, 600.f, 0.5f,
        midCook));
    *treble.coefficients = *(juce::dsp::IIR::Coefficients<double>::makeHighShelf(lastSampleRate, 1500.f, 0.3f,
        trebleCook));
    *presence.coefficients = *(juce::dsp::IIR::Coefficients<double>::makePeakFilter(lastSampleRate, 4000.f, 0.6f,
        presenceCook));
}

void STRXAudioProcessor::updateCrossover()
{
    float mode = *apvts.getRawParameterValue("mode");
    
    if (mode == 0) {
        lowBand.setCutoffFrequency(100.0);
        hiBand.setCutoffFrequency(100.0);
    }
    else if (mode == 1) {
        lowBand.setCutoffFrequency(250.0);
        hiBand.setCutoffFrequency(250.0);
    }
    else {
        lowBand.setCutoffFrequency(400.0);
        hiBand.setCutoffFrequency(400.0);
    }
    
}

float STRXAudioProcessor::updateParameters()
{
    float outVol_dB = *apvts.getRawParameterValue("outVol");

    return outVol_dB;
}

void STRXAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    ts9.updateFilters();
    updateCrossover();
    updateFilters();
    float outVol_dB = updateParameters();

    float** channelData = buffer.getArrayOfWritePointers();
    for(int i = 0; i < buffer.getNumSamples(); ++i)
    {        
        float x = 0.5f * (channelData[0][i] + channelData[1][i]);              
        
        x = ts9.processAudioSample(x, *tsXGain);

        x = 8 * (*inputGain * x);
        double xnL = lowBand.processSample(0, x);
        double xnH = hiBand.processSample(0, x);

        float k = *inputGain / 3.0;

        double yn = preamp.processAudioSample(xnH, xnL, k);

        double xHPF = highPass.processSample(yn);
        double xBPF = bandPass.processSample(yn);
        yn = xHPF + xBPF;
        double xB = bass.processSample(yn);
        double xM = mid.processSample(xB);
        double xT = treble.processSample(xM);
        double xP = presence.processSample(xT);
        if (*bright == 0) {
            yn = lowPass.processSample(xP);
        }
        else {
            yn = brightShelf.processSample(xP);
        }
        
        yn *= (*outGain / 5);
        
        yn = poweramp.processAudioSample(yn);

        yn /= 3;

        outVol.setGainDecibels(outVol_dB);
        yn = outVol.processSample(yn);

        channelData[0][i] = yn;
        channelData[1][i] = yn;
    }
}

//==============================================================================
bool STRXAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* STRXAudioProcessor::createEditor()
{
    return new STRXAudioProcessorEditor (*this);
}

//==============================================================================
void STRXAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void STRXAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new STRXAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout STRXAudioProcessor::createParameters()
{
    juce::NormalisableRange<float> nRange(1.f, 10.f, 0.1, 1.0, true);
    nRange.setSkewForCentre(5.0);

    juce::NormalisableRange<float> outVolRange(-20.f, 12.f, 0.1, 1.0, false);

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>
        ("gain", "Gain", nRange, 3.f));
    params.push_back(std::make_unique <juce::AudioParameterChoice>
        ("mode", "Mode", juce::StringArray{ "Thick", "Normal", "Open" }, 1));
    params.push_back(std::make_unique <juce::AudioParameterFloat>
        ("bass", "Bass", nRange, 5.f));
    params.push_back(std::make_unique <juce::AudioParameterFloat>
        ("mid", "Mid", nRange, 5.f));
    params.push_back(std::make_unique <juce::AudioParameterFloat>
        ("treble", "Treble", nRange, 5.f));
    params.push_back(std::make_unique <juce::AudioParameterFloat>
        ("presence", "Presence", nRange, 5.f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("bright", "Bright", false));
    params.push_back(std::make_unique <juce::AudioParameterFloat>("tsXgain", "TSXGain", 0.f, 10.0f, 0.f));
    params.push_back(std::make_unique <juce::AudioParameterFloat>
        ("master", "Master", nRange, 5.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>
        ("outVol", "Output Volume", outVolRange, 0.0));

    return { params.begin(), params.end() };  
    
}
