/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <Arbor_modules.h>
#include "zig/processor.h"

//==============================================================================
/**
*/
class STRXAudioProcessor  : public AudioProcessor,
                            public AudioProcessorValueTreeState::Listener
                            // public clap_juce_extensions::clap_properties
{
public:
    //==============================================================================
    STRXAudioProcessor();
    ~STRXAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
    void processBlock (AudioBuffer<double>&, MidiBuffer&) override;
    void processDoubleBuffer(AudioBuffer<double> &);

    bool supportsDoublePrecisionProcessing() const override { return false; }

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void parameterChanged(const String& parameterID, float newValue) override;
    
    void updateOversample();

    // String getWrapperTypeString()
    // {
    //     if (wrapperType == wrapperType_Undefined && is_clap)
    //         return "CLAP";

    //     return juce::AudioProcessor::getWrapperTypeDescription(wrapperType);
    // }
    
    AudioProcessorValueTreeState apvts;

    int lastUIWidth, lastUIHeight;

private:

    AudioProcessorValueTreeState::ParameterLayout createParameters();

    // NormalisableRange<float> nRange, outVolRange;

    Processor *proc;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (STRXAudioProcessor)
};
