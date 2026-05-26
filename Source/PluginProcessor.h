#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>
#include <algorithm>

struct ResonancePeak
{
    float frequency    { 0.0f };
    float magnitudeDb  { -100.0f };
};

class ResonanceDetectorProcessor  : public juce::AudioProcessor
{
public:
    static constexpr int fftOrder = 12;
    static constexpr int fftSize  = 1 << fftOrder;  // 4096
    static constexpr int maxPeaks = 8;

    ResonanceDetectorProcessor();
    ~ResonanceDetectorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                    { return true; }

    const juce::String getName() const override        { return JucePlugin_Name; }
    bool acceptsMidi() const override                  { return false; }
    bool producesMidi() const override                 { return false; }
    bool isMidiEffect() const override                 { return false; }
    double getTailLengthSeconds() const override       { return 0.0; }

    int  getNumPrograms() override                     { return 1; }
    int  getCurrentProgram() override                  { return 0; }
    void setCurrentProgram (int) override              {}
    const juce::String getProgramName (int) override   { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- Thread-safe data shared with the editor ---
    std::atomic<bool>  newDataAvailable { false };
    std::array<float, fftSize / 2 + 1> displaySpectrum {};
    std::array<ResonancePeak, maxPeaks> displayPeaks {};
    std::atomic<int>   displayNumPeaks { 0 };

    double currentSampleRate { 44100.0 };

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::dsp::FFT                          forwardFFT;
    juce::dsp::WindowingFunction<float>     hannWindow;

    std::array<float, fftSize>      fifo {};
    std::array<float, fftSize * 2>  fftWorkBuf {};
    std::array<float, fftSize / 2 + 1> smoothedSpectrum {};

    int fifoIndex { 0 };

    void runFFT();
    void detectPeaks (float thresholdDb, int maxCount);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonanceDetectorProcessor)
};
