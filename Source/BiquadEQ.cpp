#include "BiquadEQ.h"

#include <cstring>

static constexpr float kPi = 3.14159265358979323846f;

// ── BiquadFilter ─────────────────────────────────────────────────────────────

void BiquadFilter::setCoeffs(float b0_, float b1_, float b2_,
                              float a0_, float a1_, float a2_) {
    b0 = b0_ / a0_;
    b1 = b1_ / a0_;
    b2 = b2_ / a0_;
    a1 = a1_ / a0_;
    a2 = a2_ / a0_;
}

// Audio EQ Cookbook (R. Bristow-Johnson) — Low Shelf, S = 1 (max slope).
void BiquadFilter::setLowShelf(float hz, float dBgain, float sr) {
    const float A  = std::pow(10.f, dBgain / 40.f);
    const float w0 = 2.f * kPi * hz / sr;
    const float cw = std::cos(w0);
    const float sw = std::sin(w0);
    const float alpha = sw / 2.f * std::sqrt((A + 1.f / A) + 2.f); // S = 1
    const float twoSqAa = 2.f * std::sqrt(A) * alpha;

    setCoeffs(
        A * ((A + 1.f) - (A - 1.f) * cw + twoSqAa),
        2.f * A * ((A - 1.f) - (A + 1.f) * cw),
        A * ((A + 1.f) - (A - 1.f) * cw - twoSqAa),
        (A + 1.f) + (A - 1.f) * cw + twoSqAa,
       -2.f * ((A - 1.f) + (A + 1.f) * cw),
        (A + 1.f) + (A - 1.f) * cw - twoSqAa
    );
}

void BiquadFilter::setHighShelf(float hz, float dBgain, float sr) {
    const float A  = std::pow(10.f, dBgain / 40.f);
    const float w0 = 2.f * kPi * hz / sr;
    const float cw = std::cos(w0);
    const float sw = std::sin(w0);
    const float alpha = sw / 2.f * std::sqrt((A + 1.f / A) + 2.f); // S = 1
    const float twoSqAa = 2.f * std::sqrt(A) * alpha;

    setCoeffs(
        A * ((A + 1.f) + (A - 1.f) * cw + twoSqAa),
       -2.f * A * ((A - 1.f) + (A + 1.f) * cw),
        A * ((A + 1.f) + (A - 1.f) * cw - twoSqAa),
        (A + 1.f) - (A - 1.f) * cw + twoSqAa,
        2.f * ((A - 1.f) - (A + 1.f) * cw),
        (A + 1.f) - (A - 1.f) * cw - twoSqAa
    );
}

void BiquadFilter::setPeaking(float hz, float dBgain, float q, float sr) {
    const float A     = std::pow(10.f, dBgain / 40.f);
    const float w0    = 2.f * kPi * hz / sr;
    const float alpha = std::sin(w0) / (2.f * q);

    setCoeffs(
        1.f + alpha * A,
       -2.f * std::cos(w0),
        1.f - alpha * A,
        1.f + alpha / A,
       -2.f * std::cos(w0),
        1.f - alpha / A
    );
}

void BiquadFilter::resetState() {
    s1L = s2L = s1R = s2R = 0.f;
}

// ── BiquadEQ ─────────────────────────────────────────────────────────────────

void BiquadEQ::prepare(double sampleRate) {
    sr_ = sampleRate;
    reset();
    // Apply flat defaults so the filter states start valid.
    setBand1LowShelf (300.f,   0.f);
    setBand2Peaking  (500.f,   0.f, 1.4f);
    setBand3Peaking  (3000.f,  0.f, 1.4f);
    setBand4HighShelf(8000.f,  0.f);
}

void BiquadEQ::reset() {
    for (auto& b : band_) b.resetState();
}

void BiquadEQ::setBand1LowShelf(float hz, float dBgain) {
    band_[0].setLowShelf(hz, dBgain, static_cast<float>(sr_));
}

void BiquadEQ::setBand2Peaking(float hz, float dBgain, float q) {
    band_[1].setPeaking(hz, dBgain, q, static_cast<float>(sr_));
}

void BiquadEQ::setBand3Peaking(float hz, float dBgain, float q) {
    band_[2].setPeaking(hz, dBgain, q, static_cast<float>(sr_));
}

void BiquadEQ::setBand4HighShelf(float hz, float dBgain) {
    band_[3].setHighShelf(hz, dBgain, static_cast<float>(sr_));
}

void BiquadEQ::process(float* left, float* right, int numSamples) {
    for (int n = 0; n < numSamples; ++n) {
        float l = left[n], r = right[n];
        for (auto& b : band_) {
            l = b.tickL(l);
            r = b.tickR(r);
        }
        left[n]  = l;
        right[n] = r;
    }
}
