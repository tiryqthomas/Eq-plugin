#include "SpectrumAnalyzer.h"

void SpectrumFifo::push (const juce::AudioBuffer<float>& buffer, int numChannels)
{
    const int n = buffer.getNumSamples();
    if (numChannels <= 0 || n <= 0)
        return;

    int start1, size1, start2, size2;
    fifo.prepareToWrite (n, start1, size1, start2, size2);

    const float scale = 1.0f / (float) numChannels;
    auto write = [&] (int destStart, int count, int srcOffset)
    {
        for (int i = 0; i < count; ++i)
        {
            float s = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                s += buffer.getReadPointer (ch)[srcOffset + i];
            data[(size_t) (destStart + i)] = s * scale;
        }
    };

    write (start1, size1, 0);
    write (start2, size2, size1);
    fifo.finishedWrite (size1 + size2); // if the UI is slow, excess audio is simply dropped
}

bool SpectrumFifo::pull (float* dest, int numSamples)
{
    if (fifo.getNumReady() < numSamples)
        return false;

    int start1, size1, start2, size2;
    fifo.prepareToRead (numSamples, start1, size1, start2, size2);
    std::copy_n (data.begin() + start1, size1, dest);
    std::copy_n (data.begin() + start2, size2, dest + size1);
    fifo.finishedRead (size1 + size2);
    return true;
}

SpectrumAnalyzer::SpectrumAnalyzer()
    : history ((size_t) SpectrumFifo::kFftSize, 0.0f),
      fftData ((size_t) SpectrumFifo::kFftSize * 2, 0.0f),
      smoothedDb ((size_t) SpectrumFifo::kFftSize / 2 + 1, -120.0f)
{
}

bool SpectrumAnalyzer::update (SpectrumFifo& fifo)
{
    constexpr int hop = SpectrumFifo::kFftSize / 4;
    std::array<float, hop> incoming {};
    bool changed = false;

    while (fifo.pull (incoming.data(), hop))
    {
        // Slide the analysis window forward by one hop.
        std::move (history.begin() + hop, history.end(), history.begin());
        std::copy (incoming.begin(), incoming.end(), history.end() - hop);
        changed = true;
    }

    if (! changed)
        return false;

    sampleRate = fifo.getSampleRate();
    std::fill (fftData.begin(), fftData.end(), 0.0f);
    std::copy (history.begin(), history.end(), fftData.begin());
    window.multiplyWithWindowingTable (fftData.data(), (size_t) SpectrumFifo::kFftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

    // Normalise so a full-scale sine reads about 0 dB (Hann window has 0.5 coherent gain).
    const float norm = 2.0f / ((float) SpectrumFifo::kFftSize * 0.5f);
    for (size_t i = 0; i < smoothedDb.size(); ++i)
    {
        const float db = juce::Decibels::gainToDecibels (fftData[i] * norm, -120.0f);
        auto& s = smoothedDb[i];
        s = db > s ? db : s + (db - s) * 0.25f; // fast attack, slower release
    }
    return true;
}

float SpectrumAnalyzer::getDbAtFrequency (float freq) const
{
    const float binWidth = (float) sampleRate / (float) SpectrumFifo::kFftSize;
    const float pos = freq / binWidth;
    const int i = juce::jlimit (0, (int) smoothedDb.size() - 2, (int) pos);
    const float frac = juce::jlimit (0.0f, 1.0f, pos - (float) i);
    return smoothedDb[(size_t) i] + (smoothedDb[(size_t) i + 1] - smoothedDb[(size_t) i]) * frac;
}
