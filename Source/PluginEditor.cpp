#include "PluginEditor.h"

// ============================================================================
// SpectrumDisplay
// ============================================================================

SpectrumDisplay::SpectrumDisplay()
{
    setOpaque (true);
}

float SpectrumDisplay::freqToX (float freq, float width) const noexcept
{
    return width * std::log10 (freq / kMinHz) / std::log10 (kMaxHz / kMinHz);
}

float SpectrumDisplay::dbToY (float db, float height) const noexcept
{
    db = juce::jlimit (kMinDb, kMaxDb, db);
    return height * (1.0f - (db - kMinDb) / (kMaxDb - kMinDb));
}

void SpectrumDisplay::update (const float* data, int size,
                               const ResonancePeak* peaks, int numPeaks,
                               double sampleRate)
{
    spectrum.assign (data, data + size);
    detectedPeaks.assign (peaks, peaks + numPeaks);
    specSize = size;
    sr = sampleRate;
    repaint();
}

void SpectrumDisplay::paint (juce::Graphics& g)
{
    const float w = static_cast<float> (getWidth());
    const float h = static_cast<float> (getHeight());

    g.fillAll (juce::Colour (0xff0f0f1a));
    drawGrid   (g, w, h);
    drawSpectrum (g, w, h);
    drawPeaks  (g, w, h);
}

void SpectrumDisplay::drawGrid (juce::Graphics& g, float w, float h)
{
    g.setFont (juce::Font (10.0f));

    // Frequency grid lines
    const float freqLines[] = { 50.f, 100.f, 200.f, 500.f,
                                 1000.f, 2000.f, 5000.f, 10000.f };
    for (float f : freqLines)
    {
        float x = freqToX (f, w);
        g.setColour (juce::Colour (0xff2a2a44));
        g.drawVerticalLine (static_cast<int> (x), 0.0f, h - 16.0f);

        g.setColour (juce::Colour (0xff5555aa));
        juce::String label = (f >= 1000.f)
            ? juce::String (static_cast<int> (f / 1000.f)) + "k"
            : juce::String (static_cast<int> (f));
        g.drawText (label, x - 14.0f, h - 16.0f, 28.0f, 14.0f,
                    juce::Justification::centred);
    }

    // dB grid lines
    const float dbLines[] = { -20.f, -40.f, -60.f, -80.f };
    for (float db : dbLines)
    {
        float y = dbToY (db, h - 16.0f);
        g.setColour (juce::Colour (0xff2a2a44));
        g.drawHorizontalLine (static_cast<int> (y), 0.0f, w);

        g.setColour (juce::Colour (0xff5555aa));
        g.drawText (juce::String (static_cast<int> (db)) + "dB",
                    2.0f, y - 7.0f, 36.0f, 14.0f,
                    juce::Justification::left);
    }
}

void SpectrumDisplay::drawSpectrum (juce::Graphics& g, float w, float h)
{
    if (spectrum.empty() || specSize < 2)
        return;

    const float displayH = h - 16.0f;
    const float freqPerBin = static_cast<float> (sr) / (2.0f * static_cast<float> (specSize - 1));

    juce::Path path;
    bool started = false;

    for (int i = 1; i < specSize; ++i)
    {
        float freq = static_cast<float> (i) * freqPerBin;
        if (freq < kMinHz || freq > kMaxHz)
            continue;

        float x = freqToX (freq, w);
        float y = dbToY (spectrum[i], displayH);
        y = juce::jlimit (0.0f, displayH, y);

        if (!started)
        {
            path.startNewSubPath (x, displayH);
            path.lineTo (x, y);
            started = true;
        }
        else
        {
            path.lineTo (x, y);
        }
    }

    if (!started) return;

    path.lineTo (w, displayH);
    path.closeSubPath();

    // Gradient fill
    juce::ColourGradient gradient (juce::Colour (0x5500dd55), 0, 0,
                                    juce::Colour (0x1100aa33), 0, displayH, false);
    g.setGradientFill (gradient);
    g.fillPath (path);

    g.setColour (juce::Colour (0xff00ee55));
    g.strokePath (path, juce::PathStrokeType (1.5f));
}

void SpectrumDisplay::drawPeaks (juce::Graphics& g, float w, float h)
{
    const float displayH = h - 16.0f;

    for (int i = 0; i < static_cast<int> (detectedPeaks.size()); ++i)
    {
        const auto& peak = detectedPeaks[i];
        if (peak.frequency < kMinHz || peak.frequency > kMaxHz)
            continue;

        float x = freqToX (peak.frequency, w);
        float y = dbToY (peak.magnitudeDb, displayH);
        y = juce::jlimit (2.0f, displayH - 2.0f, y);

        // Vertical indicator line
        g.setColour (juce::Colour (0x88ff4444));
        g.drawLine (x, y, x, displayH, 1.0f);

        // Triangle marker
        juce::Path marker;
        marker.addTriangle (x - 5.0f, y - 10.0f,
                            x + 5.0f, y - 10.0f,
                            x,        y);
        g.setColour (juce::Colour (0xffff4444));
        g.fillPath (marker);

        // Frequency label
        juce::String label;
        if (peak.frequency >= 1000.0f)
            label = juce::String (peak.frequency / 1000.0f, 2) + "kHz";
        else
            label = juce::String (static_cast<int> (peak.frequency)) + "Hz";

        g.setFont (juce::Font (10.0f, juce::Font::bold));
        g.setColour (juce::Colours::white);

        // Keep label inside bounds
        float labelX = juce::jlimit (0.0f, w - 50.0f, x - 25.0f);
        g.drawText (label, labelX, y - 24.0f, 50.0f, 12.0f,
                    juce::Justification::centred);
    }
}

// ============================================================================
// PeakListDisplay
// ============================================================================

void PeakListDisplay::update (const ResonancePeak* p, int n)
{
    peaks.assign (p, p + n);
    repaint();
}

void PeakListDisplay::paint (juce::Graphics& g)
{
    const float w = static_cast<float> (getWidth());
    const float h = static_cast<float> (getHeight());

    g.fillAll (juce::Colour (0xff12121f));
    g.setColour (juce::Colour (0xff333366));
    g.drawRect (getLocalBounds(), 1);

    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff8888cc));
    g.drawText ("DETECTED RESONANCES", 6, 4, static_cast<int> (w) - 12, 14,
                juce::Justification::left);

    if (peaks.empty())
    {
        g.setFont (juce::Font (11.0f));
        g.setColour (juce::Colour (0xff555577));
        g.drawText ("No resonances above threshold",
                    6, 20, static_cast<int> (w) - 12, static_cast<int> (h) - 20,
                    juce::Justification::centredLeft);
        return;
    }

    const float rowH = (h - 22.0f) / static_cast<float> (juce::jmax (1, static_cast<int> (peaks.size())));
    const float usedH = juce::jmin (rowH, 22.0f);

    for (int i = 0; i < static_cast<int> (peaks.size()); ++i)
    {
        const auto& pk = peaks[static_cast<size_t> (i)];
        float rowY = 22.0f + i * usedH;

        // Rank number
        g.setColour (juce::Colour (0xffff4444));
        g.setFont (juce::Font (10.0f, juce::Font::bold));
        g.drawText ("#" + juce::String (i + 1), 6, rowY, 20, usedH,
                    juce::Justification::centredLeft);

        // Frequency
        juce::String freqStr;
        if (pk.frequency >= 1000.0f)
            freqStr = juce::String (pk.frequency / 1000.0f, 3) + " kHz";
        else
            freqStr = juce::String (static_cast<int> (pk.frequency)) + " Hz";

        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (11.0f));
        g.drawText (freqStr, 30, rowY, 90, usedH,
                    juce::Justification::centredLeft);

        // Magnitude
        g.setColour (juce::Colour (0xff00dd66));
        g.drawText (juce::String (pk.magnitudeDb, 1) + " dB",
                    124, rowY, 70, usedH,
                    juce::Justification::centredLeft);
    }
}

// ============================================================================
// ResonanceDetectorEditor
// ============================================================================

ResonanceDetectorEditor::ResonanceDetectorEditor (ResonanceDetectorProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      thresholdAttach (p.apvts, "threshold", thresholdSlider),
      numPeaksAttach  (p.apvts, "numPeaks",  numPeaksSlider)
{
    setSize (660, 480);

    // Threshold slider
    thresholdLabel.setText ("Threshold (dB)", juce::dontSendNotification);
    thresholdLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8888cc));
    thresholdLabel.setFont (juce::Font (11.0f));
    addAndMakeVisible (thresholdLabel);

    thresholdSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    thresholdSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
    thresholdSlider.setColour (juce::Slider::thumbColourId,     juce::Colour (0xff00ee55));
    thresholdSlider.setColour (juce::Slider::trackColourId,     juce::Colour (0xff2a2a44));
    thresholdSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    thresholdSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff333366));
    addAndMakeVisible (thresholdSlider);

    // Num peaks slider
    numPeaksLabel.setText ("Max Peaks", juce::dontSendNotification);
    numPeaksLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8888cc));
    numPeaksLabel.setFont (juce::Font (11.0f));
    addAndMakeVisible (numPeaksLabel);

    numPeaksSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    numPeaksSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 40, 20);
    numPeaksSlider.setColour (juce::Slider::thumbColourId,     juce::Colour (0xff00ee55));
    numPeaksSlider.setColour (juce::Slider::trackColourId,     juce::Colour (0xff2a2a44));
    numPeaksSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    numPeaksSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff333366));
    addAndMakeVisible (numPeaksSlider);

    addAndMakeVisible (spectrumDisplay);
    addAndMakeVisible (peakList);

    startTimerHz (30);
}

ResonanceDetectorEditor::~ResonanceDetectorEditor()
{
    stopTimer();
}

void ResonanceDetectorEditor::timerCallback()
{
    if (proc.newDataAvailable.exchange (false))
    {
        const int n = proc.displayNumPeaks.load();

        spectrumDisplay.update (proc.displaySpectrum.data(),
                                static_cast<int> (proc.displaySpectrum.size()),
                                proc.displayPeaks.data(), n,
                                proc.currentSampleRate);

        peakList.update (proc.displayPeaks.data(), n);
    }
}

void ResonanceDetectorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0f0f1a));

    // Title
    g.setFont (juce::Font (14.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff00ee55));
    g.drawText ("RESONANCE FREQUENCY DETECTOR",
                10, 8, getWidth() - 20, 20,
                juce::Justification::centred);
}

void ResonanceDetectorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    const int titleH    = 32;
    const int ctrlH     = 28;
    const int ctrlGap   = 4;
    const int listH     = 120;
    const int margin    = 8;

    // Controls row
    const int ctrlY = titleH;
    const int halfW = (w - margin * 3) / 2;

    thresholdLabel.setBounds  (margin,          ctrlY,       80, ctrlH);
    thresholdSlider.setBounds (margin + 82,     ctrlY,       halfW - 82, ctrlH);
    numPeaksLabel.setBounds   (margin * 2 + halfW,     ctrlY, 70, ctrlH);
    numPeaksSlider.setBounds  (margin * 2 + halfW + 72, ctrlY, halfW - 72, ctrlH);

    // Spectrum display (bulk of the space)
    const int specY = ctrlY + ctrlH + ctrlGap;
    const int specH = h - specY - listH - margin * 2;
    spectrumDisplay.setBounds (margin, specY, w - margin * 2, specH);

    // Peak list below the spectrum
    const int listY = specY + specH + margin;
    peakList.setBounds (margin, listY, w - margin * 2, listH);
}
