#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "EqCurveView.h"

// Simple look: rotary knobs drawn as a thin arc with a dot.
class EqLookAndFeel : public juce::LookAndFeel_V4
{
public:
    EqLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

// Bottom bar showing the selected band's controls.
class BandBar : public juce::Component
{
public:
    explicit BandBar (EqProcessor&);
    void setBand (int band);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    EqProcessor& processor;
    int band = 0;

    juce::ToggleButton onButton { "On" };
    juce::Slider freq, gain, q, output;
    juce::Label freqLabel, gainLabel, qLabel, outputLabel, bandLabel;

    std::unique_ptr<SliderAttachment> freqAtt, gainAtt, qAtt, outputAtt;
    std::unique_ptr<ButtonAttachment> onAtt;
};

class EqEditor : public juce::AudioProcessorEditor
{
public:
    explicit EqEditor (EqProcessor&);
    ~EqEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    EqLookAndFeel lookAndFeel;
    EqCurveView curveView;
    BandBar bandBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqEditor)
};
