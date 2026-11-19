#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Parameters.h"
#include "SpectrumAnalyzer.h"

class EqProcessor : public juce::AudioProcessor
{
public:
    EqProcessor();
    ~EqProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    SpectrumFifo analyzerFifo;
    std::array<std::atomic<float>, 2> outputPeak { 0.0f, 0.0f }; // read and reset by the UI meters

    struct BandParams
    {
        std::atomic<float>* on;
        std::atomic<float>* freq;
        std::atomic<float>* gain;
        std::atomic<float>* q;
    };
    std::array<BandParams, eq::kMaxBands> bandParams {};

    static juce::dsp::IIR::Coefficients<float>::Ptr makeBell (double sampleRate, float freq, float gainDb, float q);

private:
    using Filter = juce::dsp::IIR::Filter<float>;

    struct BandState
    {
        std::array<Filter, 2> filters; // one per channel
        float lastFreq = -1, lastGain = -100, lastQ = -1;
        bool active = false;
    };

    std::array<BandState, eq::kMaxBands> bands;
    std::atomic<float>* outputGain = nullptr;
    juce::SmoothedValue<float> outputGainSmoothed;
    double currentSampleRate = 44100.0;

    void updateBand (int i);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqProcessor)
};
