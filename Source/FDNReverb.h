#pragma once

#include <array>
#include <vector>
#include <cmath>
#include <cstring>

// Feedback Delay Network (FDN) reverb engine.
// 8 delay lines with Householder feedback matrix and per-line damping filters.
class FDNReverb {
public:
    static constexpr int NUM_LINES = 8;
    static constexpr int MAX_PRE_DELAY_MS = 100;

    FDNReverb() = default;

    void prepare(double sampleRate);
    void reset();

    void setRoomSize(float v);   // 0.0 – 1.0  → RT60 ~0.5s – 8s
    void setDamping(float v);    // 0.0 – 1.0  → high-freq absorption
    void setPreDelayMs(float v); // 0 – 100 ms

    // Outputs wet-only reverb signal. Dry/wet mix is handled by the caller.
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, int numSamples);

    // Approximate tail length in seconds for AU tail-time reporting.
    float tailSeconds() const { return rt60_; }

private:
    struct DelayLine {
        std::vector<float> buf;
        int writePos  = 0;
        int length    = 0;
        float lpState = 0.0f; // one-pole lowpass state

        void init(int maxLen);
        void setLength(int len);
        float read() const;
        void write(float v);
    };

    double sampleRate_ = 44100.0;

    std::array<DelayLine, NUM_LINES> lines_;
    std::array<float, NUM_LINES>     gains_{};

    // Pre-delay (mono, applied before FDN)
    std::vector<float> preBuf_;
    int preWrite_  = 0;
    int preLength_ = 0;

    float rt60_      = 2.0f;
    float dampCoeff_ = 0.5f;  // one-pole coefficient

    static constexpr int BASE_DELAYS_[NUM_LINES] = {
        1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617
    };

    void updateGains();
    void updatePreDelay(float ms);

    // Householder mixing: y_i = x_i - (2/N) * sum(x)
    static void householderMix(std::array<float, NUM_LINES>& v);
};
