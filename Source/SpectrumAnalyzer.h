#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Lock-free hand-off of mono-summed audio from the audio thread to the UI.
class SpectrumFifo
{
public:
    static constexpr int kFftOrder = 12;            // 4096-point FFT
    static constexpr int kFftSize = 1 << kFftOrder;

    void prepare (double sr) { sampleRate.store (sr); }
    void push (const juce::AudioBuffer<float>& buffer, int numChannels);

    // Called from the UI thread. Returns false if not enough new audio arrived.
    bool pull (float* dest, int numSamples);
    double getSampleRate() const { return sampleRate.load(); }

private:
    static constexpr int kCapacity = kFftSize * 4;
    juce::AbstractFifo fifo { kCapacity };
    std::array<float, kCapacity> data {};
    std::atomic<double> sampleRate { 44100.0 };
};

// UI-side analyzer: windowed FFT with smoothing, mapped to dB per FFT bin.
class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer();

    // Pulls new audio and updates the magnitude spectrum. Returns true if it changed.
    bool update (SpectrumFifo& fifo);

    // Smoothed level in dB at a given frequency (interpolated between bins).
    float getDbAtFrequency (float freq) const;

private:
    juce::dsp::FFT fft { SpectrumFifo::kFftOrder };
    juce::dsp::WindowingFunction<float> window { SpectrumFifo::kFftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> history, fftData, smoothedDb;
    double sampleRate = 44100.0;
};
