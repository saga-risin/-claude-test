#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// ---- Spectrum + Peak Overlay Display ----------------------------------------

class SpectrumDisplay  : public juce::Component
{
public:
    SpectrumDisplay();

    void paint (juce::Graphics& g) override;

    void update (const float* spectrumData, int spectrumSize,
                 const ResonancePeak* peaks, int numPeaks,
                 double sampleRate);

private:
    static constexpr float kMinHz  = 20.0f;
    static constexpr float kMaxHz  = 20000.0f;
    static constexpr float kMinDb  = -100.0f;
    static constexpr float kMaxDb  = 0.0f;

    std::vector<float>        spectrum;
    std::vector<ResonancePeak> detectedPeaks;
    int    specSize   { 0 };
    double sr         { 44100.0 };

    float freqToX (float freq, float width)  const noexcept;
    float dbToY   (float db,   float height) const noexcept;

    void drawGrid   (juce::Graphics& g, float w, float h);
    void drawSpectrum (juce::Graphics& g, float w, float h);
    void drawPeaks  (juce::Graphics& g, float w, float h);
};

// ---- Peak frequency list ----------------------------------------------------

class PeakListDisplay  : public juce::Component
{
public:
    void paint (juce::Graphics& g) override;
    void update (const ResonancePeak* peaks, int numPeaks);

private:
    std::vector<ResonancePeak> peaks;
};

// ---- Main editor window -----------------------------------------------------

class ResonanceDetectorEditor  : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    explicit ResonanceDetectorEditor (ResonanceDetectorProcessor&);
    ~ResonanceDetectorEditor() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ResonanceDetectorProcessor& proc;

    SpectrumDisplay  spectrumDisplay;
    PeakListDisplay  peakList;

    juce::Label  thresholdLabel, numPeaksLabel;
    juce::Slider thresholdSlider, numPeaksSlider;

    juce::AudioProcessorValueTreeState::SliderAttachment thresholdAttach, numPeaksAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonanceDetectorEditor)
};
