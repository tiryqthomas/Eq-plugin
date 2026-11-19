#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "SpectrumAnalyzer.h"

// The main display: frequency grid, live spectrum, per-band fills, summed EQ curve,
// draggable band nodes with a value tooltip, and output meters on the right edge.
//   drag a node        -> frequency + gain (shift: frequency only)
//   scroll on a node   -> Q
//   double-click node  -> turn band off
//   double-click empty -> turn on a free band at that spot
//   click the "12 dB"  -> cycle display range (6 / 12 / 24 dB)
class EqCurveView : public juce::Component, private juce::Timer
{
public:
    explicit EqCurveView (EqProcessor&);
    ~EqCurveView() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    std::function<void (int)> onBandSelected;
    int getSelectedBand() const { return selectedBand; }
    void setSelectedBand (int);

    // Keeps the selected band's tooltip visible (used while a panel knob is dragged).
    void setTooltipPinned (bool pinned) { tooltipPinned = pinned; repaint(); }

    // Area at the bottom that the floating band panel covers (nodes stay above it visually).
    static constexpr int kMeterWidth = 34;

private:
    void timerCallback() override;

    juce::Rectangle<float> plotArea() const;
    float freqToX (float f) const;
    float xToFreq (float x) const;
    float dbToY (float db) const;
    float yToDb (float y) const;
    float spectrumDbToY (float db) const;

    juce::Point<float> nodePosition (int band) const;
    int hitTestNode (juce::Point<float>) const;
    double bandResponseDb (int band, double freq) const;
    juce::RangedAudioParameter* param (int band, const char* name) const;

    void paintGrid (juce::Graphics&);
    void paintSpectrum (juce::Graphics&);
    void paintBands (juce::Graphics&);
    void paintNodes (juce::Graphics&);
    void paintTooltip (juce::Graphics&, int band);
    void paintMeters (juce::Graphics&);
    juce::Rectangle<float> rangeChipBounds() const;

    EqProcessor& processor;
    SpectrumAnalyzer analyzer;

    int selectedBand = 0, hoveredBand = -1, draggingBand = -1;
    bool tooltipPinned = false;
    double cachedSampleRate = 44100.0;
    float displayRangeDb = 12.0f;
    std::array<float, 2> meterLevel { 0.0f, 0.0f }; // smoothed, linear
    std::array<float, 2> meterHold { 0.0f, 0.0f };
    std::array<int, 2> holdFrames { 0, 0 };
};
