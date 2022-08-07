/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PowerAmp.h"
#include "PreAmp.h"
#include "ts9.h"

//==============================================================================
/**
*/
class STRXAudioProcessor  : public juce::AudioProcessor
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

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    void updateFilters();

    float updateParameters();

    void updateCrossover();

    juce::AudioProcessorValueTreeState apvts;

private:

    double lastSampleRate;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    juce::NormalisableRange<float> nRange;
    juce::NormalisableRange<float> outVolRange;

    std::atomic<float>* inputGain = nullptr;
    std::atomic<float>* outGain = nullptr;
    std::atomic<float>* tsXGain = nullptr;
    std::atomic<float>* bright = nullptr;

    juce::dsp::Gain<float> outVol;

    juce::dsp::LinkwitzRileyFilter<double> lowBand, hiBand;

    TS9 ts9;

    PreAmp preamp;

    juce::dsp::IIR::Filter<double> highPass;
    juce::dsp::IIR::Filter<double> bandPass;
    juce::dsp::IIR::Filter<double> lowPass;

    juce::dsp::IIR::Filter<double> bass;
    juce::dsp::IIR::Filter<double> mid;
    juce::dsp::IIR::Filter<double> treble;
    juce::dsp::IIR::Filter<double> presence;
    juce::dsp::IIR::Filter<double> brightShelf;

    ClassBValvePair poweramp;

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
