#include "PluginProcessor.h"
#include "PluginEditor.h"

ResonanceDetectorProcessor::ResonanceDetectorProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout()),
      forwardFFT (fftOrder),
      hannWindow (fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    fifo.fill (0.0f);
    fftWorkBuf.fill (0.0f);
    smoothedSpectrum.fill (-100.0f);
    displaySpectrum.fill (-100.0f);
    displayPeaks.fill ({});
}

juce::AudioProcessorValueTreeState::ParameterLayout
ResonanceDetectorProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "threshold", 1 },
        "Detection Threshold",
        juce::NormalisableRange<float> (-80.0f, -20.0f, 0.1f),
        -45.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "numPeaks", 1 },
        "Number of Peaks",
        1, maxPeaks, 5));

    return layout;
}

void ResonanceDetectorProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    fifoIndex = 0;
    fifo.fill (0.0f);
    smoothedSpectrum.fill (-100.0f);
}

bool ResonanceDetectorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void ResonanceDetectorProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    for (int s = 0; s < numSamples; ++s)
    {
        // Mix all channels to mono for analysis
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            mono += buffer.getReadPointer (ch)[s];
        if (numChannels > 1)
            mono /= static_cast<float> (numChannels);

        fifo[fifoIndex++] = mono;

        if (fifoIndex >= fftSize)
        {
            runFFT();
            fifoIndex = 0;
        }
    }
}

void ResonanceDetectorProcessor::runFFT()
{
    // Copy fifo and apply Hann window
    std::copy (fifo.begin(), fifo.end(), fftWorkBuf.begin());
    hannWindow.multiplyWithWindowingTable (fftWorkBuf.data(), fftSize);
    std::fill (fftWorkBuf.begin() + fftSize, fftWorkBuf.end(), 0.0f);

    // Forward FFT → fills first fftSize/2+1 bins with magnitudes
    forwardFFT.performFrequencyOnlyForwardTransform (fftWorkBuf.data());

    // Convert to dB with exponential smoothing
    constexpr float smoothing = 0.82f;
    constexpr float minDb     = -100.0f;

    for (int i = 0; i <= fftSize / 2; ++i)
    {
        float mag = fftWorkBuf[i] / static_cast<float> (fftSize);
        float db  = juce::Decibels::gainToDecibels (mag, minDb);
        smoothedSpectrum[i] = smoothing * smoothedSpectrum[i] + (1.0f - smoothing) * db;
    }

    auto* threshold = apvts.getRawParameterValue ("threshold");
    auto* numPeaks  = apvts.getRawParameterValue ("numPeaks");

    detectPeaks (threshold->load(), static_cast<int> (numPeaks->load()));

    // Publish to display buffers
    std::copy (smoothedSpectrum.begin(), smoothedSpectrum.end(), displaySpectrum.begin());
    newDataAvailable.store (true);
}

void ResonanceDetectorProcessor::detectPeaks (float thresholdDb, int maxCount)
{
    std::vector<ResonancePeak> found;
    found.reserve (32);

    const int halfSize = fftSize / 2;
    const float freqPerBin = static_cast<float> (currentSampleRate) / fftSize;

    // Skip DC (i=0) and very low bins; look for local maxima above threshold
    for (int i = 2; i < halfSize - 1; ++i)
    {
        const float db = smoothedSpectrum[i];
        if (db > thresholdDb
         && db > smoothedSpectrum[i - 1]
         && db > smoothedSpectrum[i + 1])
        {
            // Parabolic interpolation for sub-bin frequency accuracy
            float alpha = smoothedSpectrum[i - 1];
            float beta  = db;
            float gamma = smoothedSpectrum[i + 1];
            float offset = 0.5f * (alpha - gamma) / (alpha - 2.0f * beta + gamma);
            float freq = (static_cast<float> (i) + offset) * freqPerBin;
            found.push_back ({ freq, db });
        }
    }

    // Sort descending by magnitude
    std::sort (found.begin(), found.end(),
               [] (const ResonancePeak& a, const ResonancePeak& b)
               { return a.magnitudeDb > b.magnitudeDb; });

    // Suppress duplicate peaks within 100 Hz of a stronger one
    std::vector<ResonancePeak> deduped;
    for (const auto& p : found)
    {
        bool tooClose = false;
        for (const auto& kept : deduped)
        {
            if (std::abs (p.frequency - kept.frequency) < 100.0f)
            { tooClose = true; break; }
        }
        if (!tooClose)
            deduped.push_back (p);
        if (static_cast<int> (deduped.size()) >= maxCount)
            break;
    }

    const int count = static_cast<int> (deduped.size());
    displayNumPeaks.store (count);
    for (int i = 0; i < count; ++i)
        displayPeaks[i] = deduped[i];
    for (int i = count; i < maxPeaks; ++i)
        displayPeaks[i] = {};
}

// ---- State persistence -------------------------------------------------------

void ResonanceDetectorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ResonanceDetectorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// ---- Factory -----------------------------------------------------------------

juce::AudioProcessorEditor* ResonanceDetectorProcessor::createEditor()
{
    return new ResonanceDetectorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ResonanceDetectorProcessor();
}
