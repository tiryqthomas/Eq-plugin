#include "EqCurveView.h"
#include "Theme.h"

EqCurveView::EqCurveView (EqProcessor& p) : processor (p)
{
    startTimerHz (30);
}

EqCurveView::~EqCurveView()
{
    stopTimer();
}

void EqCurveView::timerCallback()
{
    analyzer.update (processor.analyzerFifo);
    cachedSampleRate = processor.getSampleRate() > 0 ? processor.getSampleRate() : 44100.0;
    repaint();
}

// ---- coordinate mapping -------------------------------------------------------------

float EqCurveView::freqToX (float f) const
{
    const float norm = std::log (f / eq::kMinFreq) / std::log (eq::kMaxFreq / eq::kMinFreq);
    return norm * (float) getWidth();
}

float EqCurveView::xToFreq (float x) const
{
    const float norm = juce::jlimit (0.0f, 1.0f, x / (float) getWidth());
    return eq::kMinFreq * std::pow (eq::kMaxFreq / eq::kMinFreq, norm);
}

float EqCurveView::dbToY (float db) const
{
    return juce::jmap (db, kDisplayRangeDb, -kDisplayRangeDb, 0.0f, (float) getHeight());
}

float EqCurveView::yToDb (float y) const
{
    return juce::jmap (y, 0.0f, (float) getHeight(), kDisplayRangeDb, -kDisplayRangeDb);
}

float EqCurveView::spectrumDbToY (float db) const
{
    // Spectrum uses its own scale: 0 dB at the top, -90 dB at the bottom.
    return juce::jmap (juce::jlimit (-90.0f, 0.0f, db), 0.0f, -90.0f, 0.0f, (float) getHeight());
}

// ---- band helpers -------------------------------------------------------------------

juce::RangedAudioParameter* EqCurveView::param (int band, const char* name) const
{
    return processor.apvts.getParameter (eq::bandId (band, name));
}

juce::Point<float> EqCurveView::nodePosition (int band) const
{
    auto& bp = processor.bandParams[(size_t) band];
    return { freqToX (bp.freq->load()), dbToY (bp.gain->load()) };
}

int EqCurveView::hitTestNode (juce::Point<float> pos) const
{
    int best = -1;
    float bestDist = 12.0f;
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

// ---- painting -----------------------------------------------------------------------

void EqCurveView::paint (juce::Graphics& g)
{
    const auto w = (float) getWidth(), h = (float) getHeight();
    g.fillAll (theme::background);

    // Grid
    g.setFont (11.0f);
    for (float f : { 20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f })
    {
        const float x = freqToX (f);
        g.setColour (theme::grid);
        g.drawVerticalLine ((int) x, 0.0f, h);
        g.setColour (theme::gridLabel);
        auto label = f >= 1000.f ? juce::String (f / 1000.f, 0) + "k" : juce::String ((int) f);
        g.drawText (label, (int) x + 3, (int) h - 16, 40, 14, juce::Justification::left);
    }
    for (float db : { -18.f, -12.f, -6.f, 0.f, 6.f, 12.f, 18.f })
    {
        const float y = dbToY (db);
        g.setColour (std::abs (db) < 0.1f ? theme::grid.withMultipliedAlpha (2.0f) : theme::grid);
        g.drawHorizontalLine ((int) y, 0.0f, w);
        g.setColour (theme::gridLabel);
        g.drawText ((db > 0 ? "+" : "") + juce::String ((int) db), (int) w - 30, (int) y - 7, 26, 14,
                    juce::Justification::right);
    }

    // Spectrum (output of the EQ)
    juce::Path spec;
    spec.startNewSubPath (0, h);
    for (float x = 0; x <= w; x += 2.0f)
        spec.lineTo (x, spectrumDbToY (analyzer.getDbAtFrequency (xToFreq (x))));
    spec.lineTo (w, h);
    spec.closeSubPath();
    g.setColour (theme::spectrum.withMultipliedAlpha (0.5f));
    g.fillPath (spec);
    g.setColour (theme::spectrumLine.withMultipliedAlpha (0.6f));
    g.strokePath (spec, juce::PathStrokeType (1.0f));

    // Selected band's own shape, filled
    if (processor.bandParams[(size_t) selectedBand].on->load() > 0.5f)
    {
        juce::Path fill;
        fill.startNewSubPath (0, dbToY (0));
        for (float x = 0; x <= w; x += 2.0f)
            fill.lineTo (x, dbToY ((float) bandResponseDb (selectedBand, xToFreq (x))));
        fill.lineTo (w, dbToY (0));
        fill.closeSubPath();
        g.setColour (theme::band (selectedBand).withAlpha (0.18f));
        g.fillPath (fill);
    }

    // Summed EQ curve
    juce::Path curve;
    bool first = true;
    for (float x = 0; x <= w; x += 1.0f)
    {
        const double f = xToFreq (x);
        double db = 0.0;
        for (int i = 0; i < eq::kMaxBands; ++i)
            db += bandResponseDb (i, f);
        const float y = dbToY ((float) juce::jlimit ((double) -kDisplayRangeDb, (double) kDisplayRangeDb, db));
        if (first) { curve.startNewSubPath (x, y); first = false; } else curve.lineTo (x, y);
    }
    g.setColour (theme::curve);
    g.strokePath (curve, juce::PathStrokeType (2.0f));

    // Nodes
    for (int i = 0; i < eq::kMaxBands; ++i)
    {
        if (processor.bandParams[(size_t) i].on->load() < 0.5f)
            continue;
        const auto pos = nodePosition (i);
        const float r = (i == selectedBand || i == hoveredBand) ? 8.0f : 6.0f;
        g.setColour (theme::band (i));
        g.fillEllipse (pos.x - r, pos.y - r, r * 2, r * 2);
        if (i == selectedBand)
        {
            g.setColour (juce::Colours::white);
            g.drawEllipse (pos.x - r - 2, pos.y - r - 2, (r + 2) * 2, (r + 2) * 2, 1.5f);
        }
    }
}

// ---- mouse --------------------------------------------------------------------------

void EqCurveView::mouseDown (const juce::MouseEvent& e)
{
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
    if (! e.mods.isShiftDown()) // hold shift to change frequency only
        gn->setValueNotifyingHost (gn->convertTo0to1 (juce::jlimit (-eq::kMaxGainDb, eq::kMaxGainDb, yToDb (e.position.y))));
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
        setMouseCursor (h >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
    }
}

void EqCurveView::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int hit = hitTestNode (e.position);
    if (hit >= 0)
    {
        auto* on = param (hit, "on");
        on->setValueNotifyingHost (0.0f);
        return;
    }

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
        set ("gain", juce::jlimit (-eq::kMaxGainDb, eq::kMaxGainDb, yToDb (e.position.y)));
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
