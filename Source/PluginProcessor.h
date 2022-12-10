/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

/*input * (max-min) + min*/
static float cookParams(float valueToCook, float minValue, float maxValue) 
{
    return valueToCook * (maxValue - minValue) + minValue;
}

#include "STR-X.hpp"

#if NDEBUG
    #define USE_SIMD 1
#endif

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
    void processBlock (AudioBuffer<double>&, MidiBuffer&) override;
    void processDoubleBuffer(AudioBuffer<double> &);

    bool supportsDoublePrecisionProcessing() const override { return true; }

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
    
    AudioProcessorValueTreeState apvts;

    std::vector<std::unique_ptr<dsp::Oversampling<double>>> oversample;

    int lastUIWidth, lastUIHeight;

private:

    int osIndex = 0;
    double lastSampleRate = 0.0;
    double lastDownSampleRate = 0.0;
    int numSamples = 0;

    bool isOversampled = false;

    AudioProcessorValueTreeState::ParameterLayout createParameters();

    NormalisableRange<float> nRange, outVolRange;

    std::atomic<float>* hq, *renderHQ, *outVol_dB;

    AudioBuffer<double> doubleBuffer;
#if USE_SIMD
    AmpProcessor<vec> amp;
#else
    AmpProcessor<double> amp;
#endif

    strix::SIMD<double, dsp::AudioBlock<double>, strix::AudioBlock<vec>> simd;

    std::queue<String> msgs;
    std::mutex mutex;
    std::atomic<bool> newMessages = false;

    void handleMessage()
    {
        int num = msgs.size();
        for (size_t i = 0; i < num; ++i)
        {
            auto &msg = msgs.front();
            if (msg == "renderHQ" || msg == "hq")
            {
                updateOversample();

                dsp::ProcessSpec newSpec;
                newSpec.sampleRate = lastSampleRate;
                newSpec.maximumBlockSize = numSamples * oversample[osIndex]->getOversamplingFactor();
                newSpec.numChannels = getTotalNumInputChannels();

                amp.prepare(newSpec);

                simd.setInterleavedBlockSize(newSpec.numChannels, newSpec.maximumBlockSize);
            }
            else if (msg == "legacyTone")
                amp.eq.updateAllFilters();
            else if (msg == "mode")
                amp.preAmp.needCrossoverUpdate = true;

            msgs.pop();
        }

        newMessages = false;
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (STRXAudioProcessor)
};
