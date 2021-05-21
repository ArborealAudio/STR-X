/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "STR-X.h"

//==============================================================================
/**
*/
class STRXAudioProcessor  : public AudioProcessor,
                            public AudioProcessorValueTreeState::Listener
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

    void parameterChanged (const String& parameterID, float newValue) override;
    
    void updateOversample();
    
    void updateFilters();

    float updateParameters();

    AudioProcessorValueTreeState apvts;

    std::unique_ptr<dsp::Oversampling<float>> oversample;

    /*AudioProcessorValueTreeState::Listener* hqListener;
    AudioProcessorValueTreeState::Listener* renderListener;*/

    
private:

    double lastSampleRate;

    AudioProcessorValueTreeState::ParameterLayout createParameters();

    NormalisableRange<float> nRange, outVolRange;

    std::atomic<float>* hq = nullptr;
    std::atomic<float>* renderHQ = nullptr;
    dsp::Gain<float> outVol;

    AmpProcessor amp;


    void boundValue(float& value, float minValue, float maxValue)
    {
        value = fmin(value, maxValue);
        value = fmax(value, minValue);
    }

    float cookParams(float valueToCook, float minValue, float maxValue)
    {
        boundValue(valueToCook, 0.0, 1.0);

        return valueToCook * (maxValue - minValue) + minValue;
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (STRXAudioProcessor)
};
