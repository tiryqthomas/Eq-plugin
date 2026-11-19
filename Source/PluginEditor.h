#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "EqCurveView.h"

// Knobs: dark body, gold value arc, white pointer. Sliders tagged "valueText" draw as
// a draggable text value instead (used for Output in the status bar).
class EqLookAndFeel : public juce::LookAndFeel_V4
{
public:
    EqLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

// Round power toggle drawn as a glyph.
class PowerButton : public juce::ToggleButton
{
public:
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
};

// Floating panel for the selected band.
class BandPanel : public juce::Component
{
public:
    explicit BandPanel (EqProcessor&);
    void setBand (int band);
    int getBand() const { return band; }
    void paint (juce::Graphics&) override;
    void resized() override;

    std::function<void (int)> onBandChange;   // band arrows clicked
    std::function<void (bool)> onKnobDrag;     // a knob drag started / ended

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void stepBand (int direction);

    EqProcessor& processor;
    int band = 0;

    PowerButton power;
    juce::Slider freq, gain, q;
    juce::Label freqLabel, gainLabel, qLabel;
    juce::TextButton prevBand { "<" }, nextBand { ">" };
    juce::Rectangle<int> typePill, stereoPill, bandNumberArea;

    std::unique_ptr<SliderAttachment> freqAtt, gainAtt, qAtt;
    std::unique_ptr<ButtonAttachment> powerAtt;
};

class EqEditor : public juce::AudioProcessorEditor
{
public:
    explicit EqEditor (EqProcessor&);
    ~EqEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int kHeaderH = 36, kStatusH = 26;

    EqProcessor& processor;
    EqLookAndFeel lookAndFeel;
    EqCurveView curveView;
    BandPanel bandPanel;
    juce::Slider output;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqEditor)
};
