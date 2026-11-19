#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "SpectrumAnalyzer.h"

// The main display: frequency grid, live spectrum, summed EQ curve and draggable band nodes.
//   drag a node        -> frequency + gain
//   scroll on a node   -> Q
//   double-click node  -> turn band off
//   double-click empty -> turn on the nearest free band at that spot
class EqCurveView : public juce::Component, private juce::Timer
{
public:
    explicit EqCurveView (EqProcessor&);
    ~EqCurveView() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    std::function<void (int)> onBandSelected;
    int getSelectedBand() const { return selectedBand; }
    void setSelectedBand (int);

private:
    void timerCallback() override;

    float freqToX (float f) const;
    float xToFreq (float x) const;
    float dbToY (float db) const;
    float yToDb (float y) const;
    float spectrumDbToY (float db) const;

    juce::Point<float> nodePosition (int band) const;
    int hitTestNode (juce::Point<float>) const;
    double bandResponseDb (int band, double freq) const;
    juce::RangedAudioParameter* param (int band, const char* name) const;

    EqProcessor& processor;
    SpectrumAnalyzer analyzer;

    int selectedBand = 0, hoveredBand = -1, draggingBand = -1;
    double cachedSampleRate = 44100.0;

    static constexpr float kDisplayRangeDb = 24.0f;
};
