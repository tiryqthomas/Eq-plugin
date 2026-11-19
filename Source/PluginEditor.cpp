#include "PluginEditor.h"
#include "Theme.h"

// ---- look and feel ------------------------------------------------------------------

EqLookAndFeel::EqLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, theme::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::focusedOutlineColourId, theme::textDim);
    setColour (juce::Label::textColourId, theme::textDim);
    setColour (juce::ToggleButton::textColourId, theme::text);
    setColour (juce::ToggleButton::tickColourId, theme::text);
}

void EqLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                      float startAngle, float endAngle, juce::Slider& s)
{
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0, startAngle, endAngle, true);
    g.setColour (theme::grid.withMultipliedAlpha (3.0f));
    g.strokePath (track, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, radius, radius, 0, startAngle, angle, true);
    g.setColour (accent);
    g.strokePath (value, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto dot = centre.getPointOnCircumference (radius * 0.65f, angle);
    g.setColour (theme::text);
    g.fillEllipse (dot.x - 2.5f, dot.y - 2.5f, 5.0f, 5.0f);
}

// ---- band bar -----------------------------------------------------------------------

BandBar::BandBar (EqProcessor& p) : processor (p)
{
    auto setupKnob = [this] (juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (s);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::FontOptions (11.0f));
        addAndMakeVisible (l);
    };
    setupKnob (freq, freqLabel, "FREQ");
    setupKnob (gain, gainLabel, "GAIN");
    setupKnob (q, qLabel, "Q");
    setupKnob (output, outputLabel, "OUTPUT");
    output.setColour (juce::Slider::rotarySliderFillColourId, theme::text);

    bandLabel.setJustificationType (juce::Justification::centredLeft);
    bandLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    addAndMakeVisible (bandLabel);
    addAndMakeVisible (onButton);

    outputAtt = std::make_unique<SliderAttachment> (processor.apvts, "output", output);
    setBand (0);
}

void BandBar::setBand (int b)
{
    band = b;
    // Drop old attachments before making new ones so they don't fight over the sliders.
    freqAtt.reset(); gainAtt.reset(); qAtt.reset(); onAtt.reset();
    freqAtt = std::make_unique<SliderAttachment> (processor.apvts, eq::bandId (b, "freq"), freq);
    gainAtt = std::make_unique<SliderAttachment> (processor.apvts, eq::bandId (b, "gain"), gain);
    qAtt    = std::make_unique<SliderAttachment> (processor.apvts, eq::bandId (b, "q"), q);
    onAtt   = std::make_unique<ButtonAttachment> (processor.apvts, eq::bandId (b, "on"), onButton);

    for (auto* s : { &freq, &gain, &q })
        s->setColour (juce::Slider::rotarySliderFillColourId, theme::band (b));
    bandLabel.setColour (juce::Label::textColourId, theme::band (b));
    bandLabel.setText ("Band " + juce::String (b + 1) + "  Bell", juce::dontSendNotification);
    repaint();
}

void BandBar::paint (juce::Graphics& g)
{
    g.setColour (theme::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f);
}

void BandBar::resized()
{
    auto r = getLocalBounds().reduced (12, 6);
    auto left = r.removeFromLeft (120);
    bandLabel.setBounds (left.removeFromTop (left.getHeight() / 2));
    onButton.setBounds (left.removeFromTop (24));

    auto outArea = r.removeFromRight (90);
    auto place = [] (juce::Rectangle<int> area, juce::Slider& s, juce::Label& l)
    {
        l.setBounds (area.removeFromTop (14));
        s.setBounds (area);
    };
    place (outArea, output, outputLabel);

    const int knobW = 90;
    auto knobs = r.withSizeKeepingCentre (knobW * 3, r.getHeight());
    place (knobs.removeFromLeft (knobW), freq, freqLabel);
    place (knobs.removeFromLeft (knobW), gain, gainLabel);
    place (knobs.removeFromLeft (knobW), q, qLabel);
}

// ---- editor -------------------------------------------------------------------------

EqEditor::EqEditor (EqProcessor& p)
    : AudioProcessorEditor (&p), curveView (p), bandBar (p)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (curveView);
    addAndMakeVisible (bandBar);
    curveView.onBandSelected = [this] (int b) { bandBar.setBand (b); };

    setResizable (true, true);
    setResizeLimits (640, 380, 1800, 1100);
    setSize (900, 520);
}

EqEditor::~EqEditor()
{
    setLookAndFeel (nullptr);
}

void EqEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (JucePlugin_Name, 14, 0, 200, 30, juce::Justification::centredLeft);
}

void EqEditor::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (30);
    bandBar.setBounds (r.removeFromBottom (110).reduced (12, 8));
    curveView.setBounds (r);
}
