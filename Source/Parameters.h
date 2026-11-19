#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace eq
{
    // 10 band slots are reserved from day one so later phases don't change the
    // parameter layout (which would break saved DAW sessions).
    constexpr int kMaxBands = 10;
    constexpr int kDefaultActiveBands = 4;

    constexpr float kMinFreq = 20.0f;
    constexpr float kMaxFreq = 20000.0f;
    constexpr float kMaxGainDb = 24.0f;
    constexpr float kMinQ = 0.1f;
    constexpr float kMaxQ = 18.0f;

    inline juce::String bandId (int band, const char* name)
    {
        return "b" + juce::String (band + 1) + "_" + name;
    }

    inline float defaultFreq (int band)
    {
        static const float freqs[kMaxBands] = { 80, 250, 1000, 4000, 10000, 150, 500, 2000, 6000, 14000 };
        return freqs[band];
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
