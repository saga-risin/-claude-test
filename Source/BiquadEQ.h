#pragma once

#include <cmath>

// Direct Form II Transposed biquad filter (stereo, one instance per band).
struct BiquadFilter {
    float b0 = 1.f, b1 = 0.f, b2 = 0.f;
    float a1 = 0.f, a2 = 0.f; // normalised (a0 = 1)

    float s1L = 0.f, s2L = 0.f; // left  state
    float s1R = 0.f, s2R = 0.f; // right state

    void setLowShelf (float hz, float dBgain, float sr);
    void setHighShelf(float hz, float dBgain, float sr);
    void setPeaking  (float hz, float dBgain, float q, float sr);

    inline float tickL(float x) {
        float y = b0 * x + s1L;
        s1L = b1 * x - a1 * y + s2L;
        s2L = b2 * x - a2 * y;
        return y;
    }

    inline float tickR(float x) {
        float y = b0 * x + s1R;
        s1R = b1 * x - a1 * y + s2R;
        s2R = b2 * x - a2 * y;
        return y;
    }

    void resetState();

private:
    void setCoeffs(float b0, float b1, float b2,
                   float a0, float a1, float a2);
};

// 4-band EQ applied to the reverb wet signal.
//
//  Band 1 – Low Shelf  : 20 – 1 000 Hz  (controls low-end buildup)
//  Band 2 – Peaking    : 100 – 5 000 Hz (tame muddiness)
//  Band 3 – Peaking    : 500 – 16 000 Hz (add presence)
//  Band 4 – High Shelf : 2 000 – 20 000 Hz (air / brightness)
class BiquadEQ {
public:
    void prepare(double sampleRate);
    void reset();

    void setBand1LowShelf (float hz, float dBgain);
    void setBand2Peaking  (float hz, float dBgain, float q);
    void setBand3Peaking  (float hz, float dBgain, float q);
    void setBand4HighShelf(float hz, float dBgain);

    void process(float* left, float* right, int numSamples);

private:
    double       sr_ = 44100.0;
    BiquadFilter band_[4];
};
