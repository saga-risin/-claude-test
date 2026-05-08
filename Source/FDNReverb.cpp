#include "FDNReverb.h"

#include <algorithm>
#include <cassert>

// ── DelayLine ────────────────────────────────────────────────────────────────

void FDNReverb::DelayLine::init(int maxLen) {
    buf.assign(maxLen, 0.0f);
    writePos  = 0;
    lpState   = 0.0f;
}

void FDNReverb::DelayLine::setLength(int len) {
    length = len;
}

float FDNReverb::DelayLine::read() const {
    int readPos = writePos - length;
    if (readPos < 0) readPos += static_cast<int>(buf.size());
    return buf[readPos];
}

void FDNReverb::DelayLine::write(float v) {
    buf[writePos] = v;
    if (++writePos >= static_cast<int>(buf.size())) writePos = 0;
}

// ── FDNReverb ────────────────────────────────────────────────────────────────

void FDNReverb::prepare(double sampleRate) {
    sampleRate_ = sampleRate;

    // Scale base delays from 44100 Hz reference.
    const float scale = static_cast<float>(sampleRate / 44100.0);

    for (int i = 0; i < NUM_LINES; ++i) {
        const int maxLen = static_cast<int>(BASE_DELAYS_[i] * scale * 4.0f) + 64;
        lines_[i].init(maxLen);
        lines_[i].setLength(static_cast<int>(BASE_DELAYS_[i] * scale));
    }

    // Pre-delay buffer: 100 ms max.
    const int maxPre = static_cast<int>(sampleRate * MAX_PRE_DELAY_MS / 1000.0) + 64;
    preBuf_.assign(maxPre, 0.0f);
    preWrite_ = 0;

    reset();
    updateGains();
    updatePreDelay(20.0f);
}

void FDNReverb::reset() {
    for (auto& l : lines_) {
        std::fill(l.buf.begin(), l.buf.end(), 0.0f);
        l.writePos = 0;
        l.lpState  = 0.0f;
    }
    std::fill(preBuf_.begin(), preBuf_.end(), 0.0f);
    preWrite_ = 0;
}

void FDNReverb::setRoomSize(float v) {
    // Map 0–1 to RT60 of 0.3 – 8 seconds (log scale).
    v = std::clamp(v, 0.0f, 1.0f);
    rt60_ = 0.3f * std::pow(8.0f / 0.3f, v);
    updateGains();
}

void FDNReverb::setDamping(float v) {
    v = std::clamp(v, 0.0f, 1.0f);
    // Low damping = bright (coeff near 1), high damping = dull (coeff near 0).
    dampCoeff_ = 1.0f - v * 0.85f;
}

void FDNReverb::setPreDelayMs(float v) {
    updatePreDelay(std::clamp(v, 0.0f, static_cast<float>(MAX_PRE_DELAY_MS)));
}

void FDNReverb::updateGains() {
    for (int i = 0; i < NUM_LINES; ++i) {
        const float delaySec = static_cast<float>(lines_[i].length) /
                               static_cast<float>(sampleRate_);
        // g = 10^(-3 * delay / RT60)
        gains_[i] = std::pow(10.0f, -3.0f * delaySec / rt60_);
    }
}

void FDNReverb::updatePreDelay(float ms) {
    preLength_ = static_cast<int>(ms * 0.001f * static_cast<float>(sampleRate_));
    preLength_ = std::max(0, std::min(preLength_,
                                      static_cast<int>(preBuf_.size()) - 1));
}

void FDNReverb::householderMix(std::array<float, NUM_LINES>& v) {
    float sum = 0.0f;
    for (float x : v) sum += x;
    const float factor = 2.0f / static_cast<float>(NUM_LINES) * sum;
    for (float& x : v) x -= factor;
}

void FDNReverb::process(const float* inL, const float* inR,
                        float* outL, float* outR, int numSamples) {
    for (int n = 0; n < numSamples; ++n) {
        // Mono sum into FDN (stereo spread added on output).
        const float mono = (inL[n] + inR[n]) * 0.5f;

        // Pre-delay.
        preBuf_[preWrite_] = mono;
        const int preRead = (preWrite_ - preLength_ + static_cast<int>(preBuf_.size()))
                            % static_cast<int>(preBuf_.size());
        const float preSig = (preLength_ > 0) ? preBuf_[preRead] : mono;
        if (++preWrite_ >= static_cast<int>(preBuf_.size())) preWrite_ = 0;

        // Read all delay line outputs.
        std::array<float, NUM_LINES> dlOut{};
        for (int i = 0; i < NUM_LINES; ++i)
            dlOut[i] = lines_[i].read();

        // Feedback mixing via Householder reflection.
        std::array<float, NUM_LINES> fb = dlOut;
        householderMix(fb);

        // Per-line: damping filter + gain, then write back with input injection.
        for (int i = 0; i < NUM_LINES; ++i) {
            // One-pole lowpass: y = coeff * y_prev + (1 - coeff) * x
            float& lp = lines_[i].lpState;
            lp = dampCoeff_ * lp + (1.0f - dampCoeff_) * fb[i];
            lines_[i].write(preSig + gains_[i] * lp);
        }

        // Sum outputs: odd lines to L, even lines to R for stereo spread.
        float wetL = 0.0f, wetR = 0.0f;
        for (int i = 0; i < NUM_LINES; ++i) {
            if (i & 1) wetL += dlOut[i];
            else        wetR += dlOut[i];
        }

        const float norm = 1.0f / (NUM_LINES / 2);
        outL[n] = wetL * norm;
        outR[n] = wetR * norm;
    }
}
