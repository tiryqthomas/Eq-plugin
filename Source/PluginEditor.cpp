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
    setColour (juce::Label::textColourId, theme::text);
    setColour (juce::TextButton::textColourOffId, theme::textDim);
    setColour (juce::TextButton::textColourOnId, theme::text);
}

void EqLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                      float startAngle, float endAngle, juce::Slider& s)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat();

    if (s.getProperties().contains ("valueText"))
    {
        g.setColour (s.isMouseOverOrDragging() ? theme::text : theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (s.getTextFromValue (s.getValue()), bounds, juce::Justification::centred);
        return;
    }

    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 3.0f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const bool bipolar = s.getProperties().contains ("bipolar");
    const float fromAngle = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;

    // Track and value arc
    juce::Path track;
    track.addCentredArc (c.x, c.y, radius, radius, 0, startAngle, endAngle, true);
    g.setColour (theme::knobTrack);
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (c.x, c.y, radius, radius, 0, juce::jmin (fromAngle, angle), juce::jmax (fromAngle, angle), true);
    g.setColour (theme::curve);
    g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Body
    const float bodyR = radius - 6.0f;
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (c.x - bodyR, c.y - bodyR + 2.0f, bodyR * 2, bodyR * 2);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff35323d), c.x, c.y - bodyR,
                                             theme::knobBody, c.x, c.y + bodyR, false));
    g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2, 1.0f);

    // Pointer
    const auto p1 = c.getPointOnCircumference (bodyR * 0.35f, angle);
    const auto p2 = c.getPointOnCircumference (bodyR * 0.9f, angle);
    g.setColour (theme::text);
    g.drawLine ({ p1, p2 }, 2.2f);
}

void EqLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    if (over || down)
    {
        g.setColour (juce::Colours::white.withAlpha (down ? 0.12f : 0.06f));
        g.fillRoundedRectangle (b.getLocalBounds().toFloat(), 4.0f);
    }
}

void EqLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool over, bool)
{
    g.setColour (over ? theme::text : theme::textDim);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

void PowerButton::paintButton (juce::Graphics& g, bool highlighted, bool)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    const auto c = r.getCentre();
    const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.42f;
    juce::Path p;
    p.addCentredArc (c.x, c.y, rad, rad, 0.0f, 0.75f, juce::MathConstants<float>::twoPi - 0.75f, true);
    p.startNewSubPath (c.x, c.y - rad - 1.0f);
    p.lineTo (c.x, c.y - rad * 0.2f);
    auto colour = getToggleState() ? theme::curve : theme::textDim;
    g.setColour (highlighted ? colour.brighter (0.3f) : colour);
    g.strokePath (p, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// ---- band panel ---------------------------------------------------------------------

BandPanel::BandPanel (EqProcessor& p) : processor (p)
{
    auto setupKnob = [this] (juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        s.setMouseDragSensitivity (220);
        s.onDragStart = [this] { if (onKnobDrag) onKnobDrag (true); };
        s.onDragEnd   = [this] { if (onKnobDrag) onKnobDrag (false); };
        addAndMakeVisible (s);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::FontOptions (11.5f, juce::Font::bold));
        l.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (l);
    };
    setupKnob (freq, freqLabel, "FREQ");
    setupKnob (gain, gainLabel, "GAIN");
    setupKnob (q, qLabel, "Q");
    gain.getProperties().set ("bipolar", true);

    addAndMakeVisible (power);
    addAndMakeVisible (prevBand);
    addAndMakeVisible (nextBand);
    prevBand.onClick = [this] { stepBand (-1); };
    nextBand.onClick = [this] { stepBand (+1); };

    setBand (0);
}

void BandPanel::stepBand (int direction)
{
    // Move to the next band that is turned on; fall back to the next slot.
    for (int k = 1; k <= eq::kMaxBands; ++k)
    {
        const int b = (band + direction * k + eq::kMaxBands * 2) % eq::kMaxBands;
        if (processor.bandParams[(size_t) b].on->load() > 0.5f)
        {
            if (onBandChange) onBandChange (b);
            return;
        }
    }
}

void BandPanel::setBand (int b)
{
    band = b;
    freqAtt.reset(); gainAtt.reset(); qAtt.reset(); powerAtt.reset();
    freqAtt  = std::make_unique<SliderAttachment> (processor.apvts, eq::bandId (b, "freq"), freq);
    gainAtt  = std::make_unique<SliderAttachment> (processor.apvts, eq::bandId (b, "gain"), gain);
    qAtt     = std::make_unique<SliderAttachment> (processor.apvts, eq::bandId (b, "q"), q);
    powerAtt = std::make_unique<ButtonAttachment> (processor.apvts, eq::bandId (b, "on"), power);
    repaint();
}

void BandPanel::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (1.0f);

    // Panel body with a soft drop shadow
    juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 18, { 0, 6 }).drawForRectangle (g, getLocalBounds().reduced (8));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xf2302d38), 0, r.getY(), theme::panel, 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 12.0f);
    g.setColour (theme::panelEdge);
    g.drawRoundedRectangle (r, 12.0f, 1.0f);

    // Accent stripe in the band colour along the top edge
    g.setColour (theme::band (band).withAlpha (0.8f));
    g.fillRoundedRectangle (r.getCentreX() - 22.0f, r.getY() + 5.0f, 44.0f, 3.0f, 1.5f);

    // Filter type pill
    auto pill = typePill.toFloat();
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRoundedRectangle (pill, 6.0f);
    {
        juce::Path bell;
        auto gr = juce::Rectangle<float> (pill.getX() + 10.0f, pill.getCentreY() - 4.0f, 14.0f, 8.0f);
        bell.startNewSubPath (gr.getX(), gr.getBottom());
        bell.quadraticTo (gr.getCentreX(), gr.getY() - 6.0f, gr.getRight(), gr.getBottom());
        g.setColour (theme::text);
        g.strokePath (bell, juce::PathStrokeType (1.4f));
    }
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (12.5f));
    g.drawText ("Bell", pill.withTrimmedLeft (30.0f), juce::Justification::centredLeft);

    // Stereo pill
    auto sp = stereoPill.toFloat();
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRoundedRectangle (sp, 6.0f);
    g.setColour (theme::textDim);
    g.drawEllipse (sp.getX() + 9.0f, sp.getCentreY() - 4.5f, 9.0f, 9.0f, 1.2f);
    g.drawEllipse (sp.getX() + 14.0f, sp.getCentreY() - 4.5f, 9.0f, 9.0f, 1.2f);
    g.setColour (theme::dbLabel);
    g.fillEllipse (sp.getX() + 30.0f, sp.getCentreY() - 2.0f, 4.0f, 4.0f);
    g.setColour (theme::text);
    g.drawText ("Stereo", sp.withTrimmedLeft (38.0f), juce::Justification::centredLeft);

    // Band number between the arrows
    g.setColour (theme::band (band));
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.drawText (juce::String (band + 1), bandNumberArea, juce::Justification::centred);
}

void BandPanel::resized()
{
    auto r = getLocalBounds().reduced (12, 10);

    power.setBounds (r.getX(), r.getY(), 22, 22);

    auto left = r.removeFromLeft (100);
    typePill = left.withSizeKeepingCentre (96, 26).translated (0, 4);

    auto right = r.removeFromRight (100);
    stereoPill = right.withSizeKeepingCentre (96, 26).translated (0, 4);
    auto nav = juce::Rectangle<int> (right.getX() + 18, right.getY() - 2, 64, 20);
    prevBand.setBounds (nav.removeFromLeft (20));
    nextBand.setBounds (nav.removeFromRight (20));
    bandNumberArea = nav;

    auto knobs = r.reduced (6, 0);
    const int bigW = juce::jmin (100, knobs.getHeight());
    const int smallW = juce::roundToInt (bigW * 0.78f);
    auto centre = knobs.withSizeKeepingCentre (bigW, knobs.getHeight());
    auto placeKnob = [] (juce::Rectangle<int> area, int size, juce::Slider& s, juce::Label& l, int yOffset)
    {
        auto labelArea = area.removeFromBottom (16);
        s.setBounds (area.withSizeKeepingCentre (size, size).translated (0, yOffset));
        l.setBounds (labelArea);
    };
    placeKnob (centre, bigW - 16, gain, gainLabel, 0);
    placeKnob ({ centre.getX() - smallW - 8, knobs.getY(), smallW + 8, knobs.getHeight() }, smallW - 12, freq, freqLabel, 4);
    placeKnob (knobs.withLeft (centre.getRight()).withWidth (smallW + 8), smallW - 12, q, qLabel, 4);
}

// ---- editor -------------------------------------------------------------------------

EqEditor::EqEditor (EqProcessor& p)
    : AudioProcessorEditor (&p), processor (p), curveView (p), bandPanel (p)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (curveView);
    addAndMakeVisible (bandPanel);

    curveView.onBandSelected = [this] (int b) { bandPanel.setBand (b); repaint(); };
    bandPanel.onBandChange = [this] (int b) { bandPanel.setBand (b); curveView.setSelectedBand (b); repaint(); };
    bandPanel.onKnobDrag = [this] (bool dragging) { curveView.setTooltipPinned (dragging); };

    output.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    output.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    output.getProperties().set ("valueText", true);
    output.setMouseDragSensitivity (300);
    output.setTooltip ("Output gain");
    addAndMakeVisible (output);
    outputAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "output", output);

    setResizable (true, true);
    setResizeLimits (760, 440, 2000, 1200);
    setSize (1000, 600);
}

EqEditor::~EqEditor()
{
    setLookAndFeel (nullptr);
}

void EqEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::header);

    // Header with the wordmark
    auto header = getLocalBounds().removeFromTop (kHeaderH).toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1f1d25), 0, 0, theme::header, 0, header.getBottom(), false));
    g.fillRect (header);

    juce::Path arc;
    arc.addCentredArc (26.0f, header.getCentreY() + 5.0f, 9.0f, 9.0f, 0.0f, -1.3f, 1.3f, true);
    g.setColour (theme::curve);
    g.strokePath (arc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (theme::text);
    g.setFont (juce::FontOptions (17.0f, juce::Font::bold));
    g.drawText ("Arc", 42, 0, 40, kHeaderH, juce::Justification::centredLeft);
    g.setColour (theme::curve);
    g.drawText ("EQ", 74, 0, 40, kHeaderH, juce::Justification::centredLeft);

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("Default", getLocalBounds().removeFromTop (kHeaderH), juce::Justification::centred);

    // Status bar
    auto status = getLocalBounds().removeFromBottom (kStatusH);
    g.setColour (theme::header);
    g.fillRect (status);
    g.setColour (theme::panelEdge.withAlpha (0.1f));
    g.drawHorizontalLine (status.getY(), 0.0f, (float) getWidth());

    g.setFont (juce::FontOptions (12.0f));
    g.setColour (theme::textDim);
    g.drawText ("Zero Latency", status.withTrimmedLeft (16).withWidth (120), juce::Justification::centredLeft);

    const int b = curveView.getSelectedBand();
    auto mid = status.withSizeKeepingCentre (120, kStatusH);
    g.setColour (theme::band (b));
    g.fillEllipse ((float) mid.getX() + 30.0f, (float) mid.getCentreY() - 3.0f, 6.0f, 6.0f);
    g.setColour (theme::text);
    g.drawText ("Band " + juce::String (b + 1), mid.withTrimmedLeft (42), juce::Justification::centredLeft);

    g.setColour (theme::textDim);
    g.drawText ("Analyzer: Post", status.withTrimmedRight (110).withLeft (getWidth() - 260),
                juce::Justification::centredRight);
    g.drawText ("Out", status.withLeft (getWidth() - 100).withWidth (30), juce::Justification::centredRight);
}

void EqEditor::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (kHeaderH);
    auto status = r.removeFromBottom (kStatusH);
    output.setBounds (status.withLeft (getWidth() - 68).withWidth (56));
    curveView.setBounds (r);

    const int panelW = juce::jmin (480, r.getWidth() - 40), panelH = 132;
    bandPanel.setBounds (r.getCentreX() - panelW / 2 - EqCurveView::kMeterWidth / 2, r.getBottom() - panelH - 26,
                         panelW, panelH);
}
