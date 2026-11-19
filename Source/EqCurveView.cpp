#include "EqCurveView.h"
#include "Theme.h"

namespace
{
    constexpr float kSpectrumTopDb = -10.0f, kSpectrumBottomDb = -120.0f;

    juce::String formatFreq (float f)
    {
        return f >= 1000.0f ? juce::String (f / 1000.0f, 2) + " kHz" : juce::String (f, 1) + " Hz";
    }

    // Small bell-shape glyph used in the tooltip and the band panel.
    void drawBellGlyph (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
    {
        juce::Path p;
        p.startNewSubPath (r.getX(), r.getBottom());
        p.cubicTo (r.getX() + r.getWidth() * 0.35f, r.getBottom(), r.getCentreX() - r.getWidth() * 0.15f, r.getY(),
                   r.getCentreX(), r.getY());
        p.cubicTo (r.getCentreX() + r.getWidth() * 0.15f, r.getY(), r.getRight() - r.getWidth() * 0.35f, r.getBottom(),
                   r.getRight(), r.getBottom());
        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawPowerGlyph (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
    {
        juce::Path p;
        const auto ctr = r.getCentre();
        const float rad = r.getWidth() * 0.42f;
        p.addCentredArc (ctr.x, ctr.y, rad, rad, 0.0f, 0.75f, juce::MathConstants<float>::twoPi - 0.75f, true);
        p.startNewSubPath (ctr.x, r.getY());
        p.lineTo (ctr.x, ctr.y);
        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

EqCurveView::EqCurveView (EqProcessor& p) : processor (p)
{
    startTimerHz (60);
}

EqCurveView::~EqCurveView()
{
    stopTimer();
}

void EqCurveView::resized() {}

void EqCurveView::timerCallback()
{
    analyzer.update (processor.analyzerFifo);
    cachedSampleRate = processor.getSampleRate() > 0 ? processor.getSampleRate() : 44100.0;

    const bool mono = processor.getTotalNumOutputChannels() == 1;
    for (size_t ch = 0; ch < 2; ++ch)
    {
        const float peak = processor.outputPeak[mono ? 0 : ch].exchange (0.0f);
        meterLevel[ch] = peak > meterLevel[ch] ? peak : meterLevel[ch] * 0.92f;
        if (peak >= meterHold[ch]) { meterHold[ch] = peak; holdFrames[ch] = 90; }
        else if (--holdFrames[ch] <= 0) meterHold[ch] *= 0.95f;
    }
    if (mono)
        processor.outputPeak[1].store (0.0f);

    repaint();
}

// ---- coordinate mapping -------------------------------------------------------------

juce::Rectangle<float> EqCurveView::plotArea() const
{
    return getLocalBounds().toFloat().withTrimmedRight ((float) kMeterWidth + 34.0f);
}

float EqCurveView::freqToX (float f) const
{
    const auto a = plotArea();
    const float norm = std::log (f / eq::kMinFreq) / std::log (eq::kMaxFreq / eq::kMinFreq);
    return a.getX() + norm * a.getWidth();
}

float EqCurveView::xToFreq (float x) const
{
    const auto a = plotArea();
    const float norm = juce::jlimit (0.0f, 1.0f, (x - a.getX()) / a.getWidth());
    return eq::kMinFreq * std::pow (eq::kMaxFreq / eq::kMinFreq, norm);
}

// The EQ scale keeps 0 dB a little above centre, as the bottom of the view is busier.
float EqCurveView::dbToY (float db) const
{
    const auto a = plotArea();
    const float zeroY = a.getY() + a.getHeight() * 0.42f;
    const float halfSpan = a.getHeight() * 0.36f;
    return zeroY - db / displayRangeDb * halfSpan;
}

float EqCurveView::yToDb (float y) const
{
    const auto a = plotArea();
    const float zeroY = a.getY() + a.getHeight() * 0.42f;
    const float halfSpan = a.getHeight() * 0.36f;
    return (zeroY - y) / halfSpan * displayRangeDb;
}

float EqCurveView::spectrumDbToY (float db) const
{
    const auto a = plotArea();
    return juce::jmap (juce::jlimit (kSpectrumBottomDb, kSpectrumTopDb, db),
                       kSpectrumTopDb, kSpectrumBottomDb, a.getY() + 24.0f, a.getBottom());
}

// ---- band helpers -------------------------------------------------------------------

juce::RangedAudioParameter* EqCurveView::param (int band, const char* name) const
{
    return processor.apvts.getParameter (eq::bandId (band, name));
}

juce::Point<float> EqCurveView::nodePosition (int band) const
{
    auto& bp = processor.bandParams[(size_t) band];
    const float gain = juce::jlimit (-displayRangeDb, displayRangeDb, bp.gain->load());
    return { freqToX (bp.freq->load()), dbToY (gain) };
}

int EqCurveView::hitTestNode (juce::Point<float> pos) const
{
    int best = -1;
    float bestDist = 13.0f;
    for (int i = 0; i < eq::kMaxBands; ++i)
    {
        if (processor.bandParams[(size_t) i].on->load() < 0.5f)
            continue;
        const float d = nodePosition (i).getDistanceFrom (pos);
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}

double EqCurveView::bandResponseDb (int band, double freq) const
{
    auto& bp = processor.bandParams[(size_t) band];
    if (bp.on->load() < 0.5f || std::abs (bp.gain->load()) < 0.001f)
        return 0.0;
    auto c = EqProcessor::makeBell (cachedSampleRate, bp.freq->load(), bp.gain->load(), bp.q->load());
    return juce::Decibels::gainToDecibels (c->getMagnitudeForFrequency (freq, cachedSampleRate));
}

void EqCurveView::setSelectedBand (int b)
{
    selectedBand = b;
    repaint();
}

juce::Rectangle<float> EqCurveView::rangeChipBounds() const
{
    const auto a = plotArea();
    return { a.getRight() - 4.0f, a.getY() + 6.0f, 40.0f, 18.0f };
}

// ---- painting -----------------------------------------------------------------------

void EqCurveView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (theme::backgroundTop, 0, bounds.getY(),
                                             theme::background, 0, bounds.getBottom(), false));
    g.fillRect (bounds);

    paintGrid (g);
    paintSpectrum (g);
    paintBands (g);
    paintNodes (g);
    paintMeters (g);

    const int tip = draggingBand >= 0 ? draggingBand
                  : hoveredBand >= 0  ? hoveredBand
                  : tooltipPinned     ? selectedBand : -1;
    if (tip >= 0 && processor.bandParams[(size_t) tip].on->load() > 0.5f)
        paintTooltip (g, tip);
}

void EqCurveView::paintGrid (juce::Graphics& g)
{
    const auto a = plotArea();

    g.setFont (juce::FontOptions (11.0f));
    for (float f : { 20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f })
    {
        const float x = freqToX (f);
        g.setColour (theme::grid);
        g.drawVerticalLine ((int) x, a.getY(), a.getBottom());
        g.setColour (theme::gridLabel);
        auto label = f >= 1000.f ? juce::String ((int) (f / 1000.f)) + "k" : juce::String ((int) f);
        const int lx = juce::jlimit ((int) a.getX() + 2, (int) a.getRight() - 40, (int) x - 20);
        g.drawText (label, lx, (int) a.getBottom() - 18, 40, 14,
                    lx > (int) x - 20 ? juce::Justification::centredLeft : juce::Justification::centred);
    }
    // Minor frequency lines
    g.setColour (theme::grid.withMultipliedAlpha (0.5f));
    for (float decade : { 10.f, 100.f, 1000.f, 10000.f })
        for (int m = 2; m <= 9; ++m)
            if (m != 2 && m != 5 && decade * (float) m >= eq::kMinFreq && decade * (float) m <= eq::kMaxFreq)
                g.drawVerticalLine ((int) freqToX (decade * (float) m), a.getY(), a.getBottom());

    // EQ dB lines (gold labels) on the right of the plot
    const float step = displayRangeDb / 4.0f;
    for (int k = -4; k <= 4; ++k)
    {
        const float db = step * (float) k;
        const float y = dbToY (db);
        g.setColour (k == 0 ? theme::grid.withMultipliedAlpha (2.2f) : theme::grid);
        g.drawHorizontalLine ((int) y, a.getX(), a.getRight());
        if (k == 4) continue; // leave room for the range chip
        g.setColour (theme::dbLabel.withAlpha (k == 0 ? 0.9f : 0.75f));
        auto txt = k == 0 ? juce::String ("0") : (db > 0 ? "+" : "") + juce::String (db, step < 2.0f ? 1 : 0);
        g.drawText (txt, (int) a.getRight() + 2, (int) y - 7, 30, 14, juce::Justification::centredLeft);
    }

    // Range chip ("12 dB")
    const auto chip = rangeChipBounds();
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (chip, 4.0f);
    g.setColour (theme::dbLabel);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (juce::String ((int) displayRangeDb) + " dB", chip, juce::Justification::centred);
}

void EqCurveView::paintSpectrum (juce::Graphics& g)
{
    const auto a = plotArea();
    juce::Path line;
    bool first = true;
    for (float x = a.getX(); x <= a.getRight(); x += 1.5f)
    {
        const float y = spectrumDbToY (analyzer.getDbAtFrequency (xToFreq (x)));
        if (first) { line.startNewSubPath (x, y); first = false; } else line.lineTo (x, y);
    }

    juce::Path fill (line);
    fill.lineTo (a.getRight(), a.getBottom());
    fill.lineTo (a.getX(), a.getBottom());
    fill.closeSubPath();

    g.setGradientFill (juce::ColourGradient (theme::spectrumFill, 0, a.getY() + a.getHeight() * 0.3f,
                                             theme::spectrumFill.withAlpha (0.04f), 0, a.getBottom(), false));
    g.fillPath (fill);
    g.setColour (theme::spectrumLine);
    g.strokePath (line, juce::PathStrokeType (1.0f));
}

void EqCurveView::paintBands (juce::Graphics& g)
{
    const auto a = plotArea();
    const float zeroY = dbToY (0.0f);
    const float clampTop = a.getY() + 2.0f, clampBottom = a.getBottom() - 2.0f;

    // Each band's own shape: a coloured fill between 0 dB and its curve.
    for (int i = 0; i < eq::kMaxBands; ++i)
    {
        auto& bp = processor.bandParams[(size_t) i];
        if (bp.on->load() < 0.5f)
            continue;

        juce::Path shape;
        shape.startNewSubPath (a.getX(), zeroY);
        for (float x = a.getX(); x <= a.getRight(); x += 2.0f)
            shape.lineTo (x, juce::jlimit (clampTop, clampBottom, dbToY ((float) bandResponseDb (i, xToFreq (x)))));
        shape.lineTo (a.getRight(), zeroY);

        const auto colour = theme::band (i);
        const bool selected = i == selectedBand;
        const float peakY = nodePosition (i).y;

        juce::Path filled (shape);
        filled.closeSubPath();
        g.setGradientFill (juce::ColourGradient (colour.withAlpha (selected ? 0.65f : 0.32f), 0, peakY,
                                                 colour.withAlpha (selected ? 0.18f : 0.06f), 0, zeroY, false));
        g.fillPath (filled);

        g.setColour (colour.withAlpha (selected ? 0.95f : 0.7f));
        g.strokePath (shape, juce::PathStrokeType (selected ? 1.6f : 1.2f));
    }

    // Summed EQ curve in gold, with a soft glow underneath.
    juce::Path curve;
    bool first = true;
    for (float x = a.getX(); x <= a.getRight(); x += 1.0f)
    {
        const double f = xToFreq (x);
        double db = 0.0;
        for (int i = 0; i < eq::kMaxBands; ++i)
            db += bandResponseDb (i, f);
        const float y = juce::jlimit (clampTop, clampBottom, dbToY ((float) db));
        if (first) { curve.startNewSubPath (x, y); first = false; } else curve.lineTo (x, y);
    }
    g.setColour (theme::curve.withAlpha (0.18f));
    g.strokePath (curve, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (theme::curve);
    g.strokePath (curve, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void EqCurveView::paintNodes (juce::Graphics& g)
{
    for (int i = 0; i < eq::kMaxBands; ++i)
    {
        if (processor.bandParams[(size_t) i].on->load() < 0.5f)
            continue;

        const auto pos = nodePosition (i);
        const auto colour = theme::band (i);
        const bool big = i == selectedBand || i == hoveredBand || i == draggingBand;
        const float r = big ? 8.5f : 6.5f;

        if (big)
        {
            g.setColour (colour.withAlpha (0.25f));
            g.fillEllipse (pos.x - r - 5, pos.y - r - 5, (r + 5) * 2, (r + 5) * 2);
        }
        g.setColour (juce::Colours::white.withAlpha (big ? 0.95f : 0.8f));
        g.fillEllipse (pos.x - r - 2, pos.y - r - 2, (r + 2) * 2, (r + 2) * 2);
        g.setGradientFill (juce::ColourGradient (colour.brighter (0.4f), pos.x, pos.y - r,
                                                 colour.darker (0.2f), pos.x, pos.y + r, false));
        g.fillEllipse (pos.x - r, pos.y - r, r * 2, r * 2);

        // Little up/down arrows on the selected node's 0 dB anchor hint at gain dragging.
        if (i == selectedBand)
        {
            const float ax = pos.x, ay = dbToY (0.0f);
            if (std::abs (ay - pos.y) > 18.0f)
            {
                juce::Path up, down;
                up.addTriangle (ax - 4, ay - 3, ax + 4, ay - 3, ax, ay - 9);
                down.addTriangle (ax - 4, ay + 3, ax + 4, ay + 3, ax, ay + 9);
                g.setColour (juce::Colours::white.withAlpha (0.45f));
                g.fillPath (up);
                g.fillPath (down);
            }
        }
    }
}

void EqCurveView::paintTooltip (juce::Graphics& g, int band)
{
    auto& bp = processor.bandParams[(size_t) band];
    const auto node = nodePosition (band);

    const float w = 150.0f, h = 66.0f;
    auto box = juce::Rectangle<float> (node.x - w * 0.5f, node.y - h - 20.0f, w, h);
    bool below = false;
    if (box.getY() < 4.0f) { box.setY (node.y + 20.0f); below = true; }
    box.setX (juce::jlimit (4.0f, plotArea().getRight() - w - 4.0f, box.getX()));

    juce::Path bubble;
    bubble.addRoundedRectangle (box, 9.0f);
    const float tipX = juce::jlimit (box.getX() + 12.0f, box.getRight() - 12.0f, node.x);
    if (below)
        bubble.addTriangle (tipX - 7, box.getY(), tipX + 7, box.getY(), tipX, box.getY() - 7);
    else
        bubble.addTriangle (tipX - 7, box.getBottom(), tipX + 7, box.getBottom(), tipX, box.getBottom() + 7);

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillPath (bubble, juce::AffineTransform::translation (0, 3));
    g.setColour (theme::panel.withAlpha (0.96f));
    g.fillPath (bubble);
    g.setColour (theme::panelEdge);
    g.strokePath (bubble, juce::PathStrokeType (1.0f));

    auto inner = box.reduced (10.0f, 6.0f);
    drawPowerGlyph (g, { inner.getX(), inner.getY() + 3.0f, 13.0f, 13.0f }, theme::dbLabel);
    drawBellGlyph (g, { inner.getX(), inner.getBottom() - 14.0f, 15.0f, 9.0f }, theme::textDim);

    const float gain = bp.gain->load();
    auto textArea = inner.withTrimmedLeft (20.0f);
    const float lineH = textArea.getHeight() / 3.0f;
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (13.5f, juce::Font::bold));
    g.drawText (formatFreq (bp.freq->load()), textArea.removeFromTop (lineH), juce::Justification::centred);
    g.drawText ((gain > 0 ? "+" : "") + juce::String (gain, 2) + " dB", textArea.removeFromTop (lineH),
                juce::Justification::centred);
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.drawText ("Q: " + juce::String (bp.q->load(), 3), textArea, juce::Justification::centred);
}

void EqCurveView::paintMeters (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().removeFromRight ((float) kMeterWidth).reduced (4.0f, 8.0f);
    const float barW = (area.getWidth() - 4.0f) / 2.0f;

    auto levelToY = [&area] (float lin)
    {
        const float db = juce::Decibels::gainToDecibels (lin, -60.0f);
        return juce::jmap (db, -60.0f, 0.0f, area.getBottom(), area.getY());
    };

    for (size_t ch = 0; ch < 2; ++ch)
    {
        auto bar = juce::Rectangle<float> (area.getX() + (float) ch * (barW + 4.0f), area.getY(), barW, area.getHeight());
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (bar, 2.0f);

        const float top = levelToY (meterLevel[ch]);
        auto lit = bar.withTop (top);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd8e04a), 0, bar.getY(),
                                                 juce::Colour (0xff3f9a46), 0, bar.getBottom(), false));
        g.fillRoundedRectangle (lit, 2.0f);

        g.setColour (meterHold[ch] >= 1.0f ? juce::Colour (0xffef5a5a) : theme::dbLabel);
        g.fillRect (bar.getX(), levelToY (meterHold[ch]) - 1.0f, bar.getWidth(), 2.0f);
    }
}

// ---- mouse --------------------------------------------------------------------------

void EqCurveView::mouseDown (const juce::MouseEvent& e)
{
    if (rangeChipBounds().contains (e.position))
    {
        displayRangeDb = displayRangeDb < 7.0f ? 12.0f : displayRangeDb < 13.0f ? 24.0f : 6.0f;
        repaint();
        return;
    }

    draggingBand = hitTestNode (e.position);
    if (draggingBand >= 0)
    {
        setSelectedBand (draggingBand);
        if (onBandSelected) onBandSelected (draggingBand);
        param (draggingBand, "freq")->beginChangeGesture();
        param (draggingBand, "gain")->beginChangeGesture();
    }
}

void EqCurveView::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingBand < 0)
        return;
    auto* f = param (draggingBand, "freq");
    auto* gn = param (draggingBand, "gain");
    f->setValueNotifyingHost (f->convertTo0to1 (xToFreq (e.position.x)));
    if (! e.mods.isShiftDown())
        gn->setValueNotifyingHost (gn->convertTo0to1 (juce::jlimit (-displayRangeDb, displayRangeDb, yToDb (e.position.y))));
}

void EqCurveView::mouseUp (const juce::MouseEvent&)
{
    if (draggingBand >= 0)
    {
        param (draggingBand, "freq")->endChangeGesture();
        param (draggingBand, "gain")->endChangeGesture();
    }
    draggingBand = -1;
}

void EqCurveView::mouseMove (const juce::MouseEvent& e)
{
    const int h = hitTestNode (e.position);
    if (h != hoveredBand)
    {
        hoveredBand = h;
        setMouseCursor (h >= 0 ? juce::MouseCursor::DraggingHandCursor
                               : rangeChipBounds().contains (e.position) ? juce::MouseCursor::PointingHandCursor
                                                                         : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void EqCurveView::mouseExit (const juce::MouseEvent&)
{
    hoveredBand = -1;
    repaint();
}

void EqCurveView::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int hit = hitTestNode (e.position);
    if (hit >= 0)
    {
        param (hit, "on")->setValueNotifyingHost (0.0f);
        hoveredBand = -1;
        return;
    }
    if (! plotArea().contains (e.position))
        return;

    for (int i = 0; i < eq::kMaxBands; ++i)
    {
        if (processor.bandParams[(size_t) i].on->load() > 0.5f)
            continue;
        auto set = [this, i] (const char* name, float value)
        {
            auto* p = param (i, name);
            p->setValueNotifyingHost (p->convertTo0to1 (value));
        };
        set ("freq", xToFreq (e.position.x));
        set ("gain", juce::jlimit (-displayRangeDb, displayRangeDb, yToDb (e.position.y)));
        set ("q", 1.0f);
        set ("on", 1.0f);
        setSelectedBand (i);
        if (onBandSelected) onBandSelected (i);
        return;
    }
}

void EqCurveView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    int band = hitTestNode (e.position);
    if (band < 0)
        band = selectedBand;
    auto* q = param (band, "q");
    const float current = q->convertFrom0to1 (q->getValue());
    const float next = juce::jlimit (eq::kMinQ, eq::kMaxQ, current * std::pow (1.15f, wheel.deltaY * 4.0f));
    q->setValueNotifyingHost (q->convertTo0to1 (next));
}
