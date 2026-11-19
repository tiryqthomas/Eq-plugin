#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout eq::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < kMaxBands; ++i)
    {
        auto prefix = "Band " + String (i + 1) + " ";

        layout.add (std::make_unique<AudioParameterBool> (ParameterID { bandId (i, "on"), 1 },
                                                          prefix + "On", i < kDefaultActiveBands));

        NormalisableRange<float> freqRange (kMinFreq, kMaxFreq);
        freqRange.setSkewForCentre (1000.0f);
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { bandId (i, "freq"), 1 },
                                                           prefix + "Freq", freqRange, defaultFreq (i),
                                                           AudioParameterFloatAttributes().withLabel ("Hz").withStringFromValueFunction (
                                                               [] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 2) + " kHz" : String (v, 0) + " Hz"; })));

        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { bandId (i, "gain"), 1 },
                                                           prefix + "Gain",
                                                           NormalisableRange<float> (-kMaxGainDb, kMaxGainDb, 0.01f), 0.0f,
                                                           AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (
                                                               [] (float v, int) { return (v > 0 ? "+" : "") + String (v, 1) + " dB"; })));

        NormalisableRange<float> qRange (kMinQ, kMaxQ);
        qRange.setSkewForCentre (1.0f);
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { bandId (i, "q"), 1 },
                                                           prefix + "Q", qRange, 1.0f,
                                                           AudioParameterFloatAttributes().withStringFromValueFunction (
                                                               [] (float v, int) { return String (v, 2); })));
    }

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Output",
                                                       NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f,
                                                       AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (
                                                           [] (float v, int) { return (v > 0 ? "+" : "") + String (v, 1) + " dB"; })));
    return layout;
}

EqProcessor::EqProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", eq::createLayout())
{
    for (int i = 0; i < eq::kMaxBands; ++i)
    {
        bandParams[(size_t) i] = { apvts.getRawParameterValue (eq::bandId (i, "on")),
                                   apvts.getRawParameterValue (eq::bandId (i, "freq")),
                                   apvts.getRawParameterValue (eq::bandId (i, "gain")),
                                   apvts.getRawParameterValue (eq::bandId (i, "q")) };
    }
    outputGain = apvts.getRawParameterValue ("output");
}

bool EqProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

juce::dsp::IIR::Coefficients<float>::Ptr EqProcessor::makeBell (double sampleRate, float freq, float gainDb, float q)
{
    auto f = juce::jlimit (eq::kMinFreq, (float) (sampleRate * 0.49), freq);
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, f, q,
                                                                juce::Decibels::decibelsToGain (gainDb));
}

void EqProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 1 };

    for (auto& b : bands)
    {
        for (auto& f : b.filters)
        {
            f.prepare (spec);
            f.reset();
        }
        b.lastFreq = -1; // force coefficient refresh
    }

    outputGainSmoothed.reset (sampleRate, 0.02);
    outputGainSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputGain->load()));
    analyzerFifo.prepare (sampleRate);
}

void EqProcessor::updateBand (int i)
{
    auto& p = bandParams[(size_t) i];
    auto& b = bands[(size_t) i];

    const bool on = p.on->load() > 0.5f;
    const float freq = p.freq->load(), gain = p.gain->load(), q = p.q->load();

    if (on && ! b.active)
        for (auto& f : b.filters) f.reset();

    // A bell at 0 dB does nothing, so skip it to save CPU.
    b.active = on && std::abs (gain) > 0.001f;

    if (b.active && ! (juce::exactlyEqual (freq, b.lastFreq) && juce::exactlyEqual (gain, b.lastGain) && juce::exactlyEqual (q, b.lastQ)))
    {
        auto coeffs = makeBell (currentSampleRate, freq, gain, q);
        for (auto& f : b.filters)
            *f.coefficients = *coeffs;
        b.lastFreq = freq; b.lastGain = gain; b.lastQ = q;
    }
}

void EqProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (getTotalNumInputChannels(), 2);
    for (int ch = numCh; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    for (int i = 0; i < eq::kMaxBands; ++i)
    {
        updateBand (i);
        auto& b = bands[(size_t) i];
        if (! b.active)
            continue;

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            auto& f = b.filters[(size_t) ch];
            for (int n = 0; n < buffer.getNumSamples(); ++n)
                data[n] = f.processSample (data[n]);
        }
    }

    outputGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (outputGain->load()));
    if (outputGainSmoothed.isSmoothing())
    {
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            auto g = outputGainSmoothed.getNextValue();
            for (int ch = 0; ch < numCh; ++ch)
                buffer.getWritePointer (ch)[n] *= g;
        }
    }
    else
    {
        buffer.applyGain (outputGainSmoothed.getTargetValue());
    }

    for (int ch = 0; ch < numCh; ++ch)
    {
        const float peak = buffer.getMagnitude (ch, 0, buffer.getNumSamples());
        auto& slot = outputPeak[(size_t) ch];
        if (peak > slot.load())
            slot.store (peak);
    }

    analyzerFifo.push (buffer, numCh);
}

void EqProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void EqProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* EqProcessor::createEditor()
{
    return new EqEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EqProcessor();
}
