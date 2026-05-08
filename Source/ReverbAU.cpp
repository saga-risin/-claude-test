#include "ReverbAU.h"

#include <AudioUnit/AudioUnit.h>
#include <CoreFoundation/CoreFoundation.h>
#include <algorithm>

// ── Factory entry point ───────────────────────────────────────────────────────

AUDIOCOMPONENT_ENTRY(AUBaseFactory, ReverbAU)

// ── Constructor ───────────────────────────────────────────────────────────────

ReverbAU::ReverbAU(AudioUnit component)
    : AUEffectBase(component, false) {
    // Reverb defaults
    SetParameter(kParam_RoomSize, 0.5f);
    SetParameter(kParam_Damping,  0.5f);
    SetParameter(kParam_WetDry,   0.5f);
    SetParameter(kParam_PreDelay, 20.0f);

    // EQ defaults (flat — no colouring out of the box)
    SetParameter(kParam_EQ1Freq,  300.f);
    SetParameter(kParam_EQ1Gain,  0.f);
    SetParameter(kParam_EQ2Freq,  500.f);
    SetParameter(kParam_EQ2Gain,  0.f);
    SetParameter(kParam_EQ2Q,     1.4f);
    SetParameter(kParam_EQ3Freq,  3000.f);
    SetParameter(kParam_EQ3Gain,  0.f);
    SetParameter(kParam_EQ3Q,     1.4f);
    SetParameter(kParam_EQ4Freq,  8000.f);
    SetParameter(kParam_EQ4Gain,  0.f);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

OSStatus ReverbAU::Initialize() {
    OSStatus err = AUEffectBase::Initialize();
    if (err != noErr) return err;

    const double sr = GetSampleRate();
    reverb_.prepare(sr);
    eq_.prepare(sr);
    syncParams();

    // Pre-allocate wet buffers to max expected block size.
    wetL_.assign(4096, 0.f);
    wetR_.assign(4096, 0.f);

    return noErr;
}

void ReverbAU::Cleanup() {
    reverb_.reset();
    eq_.reset();
    AUEffectBase::Cleanup();
}

// ── Parameters ────────────────────────────────────────────────────────────────

OSStatus ReverbAU::GetParameterInfo(AudioUnitScope          inScope,
                                    AudioUnitParameterID    inID,
                                    AudioUnitParameterInfo& o) {
    if (inScope != kAudioUnitScope_Global) return kAudioUnitErr_InvalidScope;

    o.flags = kAudioUnitParameterFlag_IsReadable | kAudioUnitParameterFlag_IsWritable;

    switch (inID) {
    // ── Reverb ────────────────────────────────────────────────────────────
    case kParam_RoomSize:
        AUBase::FillInParameterName(o, CFSTR("Room Size"), false);
        o.unit = kAudioUnitParameterUnit_Generic;
        o.minValue = 0.f;  o.maxValue = 1.f;   o.defaultValue = 0.5f;
        break;

    case kParam_Damping:
        AUBase::FillInParameterName(o, CFSTR("Damping"), false);
        o.unit = kAudioUnitParameterUnit_Generic;
        o.minValue = 0.f;  o.maxValue = 1.f;   o.defaultValue = 0.5f;
        break;

    case kParam_WetDry:
        AUBase::FillInParameterName(o, CFSTR("Wet/Dry Mix"), false);
        o.unit = kAudioUnitParameterUnit_Generic;
        o.minValue = 0.f;  o.maxValue = 1.f;   o.defaultValue = 0.5f;
        break;

    case kParam_PreDelay:
        AUBase::FillInParameterName(o, CFSTR("Pre-Delay (ms)"), false);
        o.unit = kAudioUnitParameterUnit_Milliseconds;
        o.minValue = 0.f;  o.maxValue = 100.f; o.defaultValue = 20.f;
        break;

    // ── EQ Band 1 — Low Shelf ─────────────────────────────────────────────
    case kParam_EQ1Freq:
        AUBase::FillInParameterName(o, CFSTR("EQ1 Freq (Hz)"), false);
        o.unit = kAudioUnitParameterUnit_Hertz;
        o.minValue = 20.f; o.maxValue = 1000.f; o.defaultValue = 300.f;
        break;

    case kParam_EQ1Gain:
        AUBase::FillInParameterName(o, CFSTR("EQ1 Gain (dB)"), false);
        o.unit = kAudioUnitParameterUnit_Decibels;
        o.minValue = -18.f; o.maxValue = 18.f; o.defaultValue = 0.f;
        break;

    // ── EQ Band 2 — Peaking ───────────────────────────────────────────────
    case kParam_EQ2Freq:
        AUBase::FillInParameterName(o, CFSTR("EQ2 Freq (Hz)"), false);
        o.unit = kAudioUnitParameterUnit_Hertz;
        o.minValue = 100.f; o.maxValue = 5000.f; o.defaultValue = 500.f;
        break;

    case kParam_EQ2Gain:
        AUBase::FillInParameterName(o, CFSTR("EQ2 Gain (dB)"), false);
        o.unit = kAudioUnitParameterUnit_Decibels;
        o.minValue = -18.f; o.maxValue = 18.f; o.defaultValue = 0.f;
        break;

    case kParam_EQ2Q:
        AUBase::FillInParameterName(o, CFSTR("EQ2 Q"), false);
        o.unit = kAudioUnitParameterUnit_Generic;
        o.minValue = 0.1f; o.maxValue = 10.f; o.defaultValue = 1.4f;
        break;

    // ── EQ Band 3 — Peaking ───────────────────────────────────────────────
    case kParam_EQ3Freq:
        AUBase::FillInParameterName(o, CFSTR("EQ3 Freq (Hz)"), false);
        o.unit = kAudioUnitParameterUnit_Hertz;
        o.minValue = 500.f; o.maxValue = 16000.f; o.defaultValue = 3000.f;
        break;

    case kParam_EQ3Gain:
        AUBase::FillInParameterName(o, CFSTR("EQ3 Gain (dB)"), false);
        o.unit = kAudioUnitParameterUnit_Decibels;
        o.minValue = -18.f; o.maxValue = 18.f; o.defaultValue = 0.f;
        break;

    case kParam_EQ3Q:
        AUBase::FillInParameterName(o, CFSTR("EQ3 Q"), false);
        o.unit = kAudioUnitParameterUnit_Generic;
        o.minValue = 0.1f; o.maxValue = 10.f; o.defaultValue = 1.4f;
        break;

    // ── EQ Band 4 — High Shelf ────────────────────────────────────────────
    case kParam_EQ4Freq:
        AUBase::FillInParameterName(o, CFSTR("EQ4 Freq (Hz)"), false);
        o.unit = kAudioUnitParameterUnit_Hertz;
        o.minValue = 2000.f; o.maxValue = 20000.f; o.defaultValue = 8000.f;
        break;

    case kParam_EQ4Gain:
        AUBase::FillInParameterName(o, CFSTR("EQ4 Gain (dB)"), false);
        o.unit = kAudioUnitParameterUnit_Decibels;
        o.minValue = -18.f; o.maxValue = 18.f; o.defaultValue = 0.f;
        break;

    default:
        return kAudioUnitErr_InvalidParameter;
    }

    return noErr;
}

// ── Audio processing ──────────────────────────────────────────────────────────

Float64 ReverbAU::GetTailTime() {
    return reverb_.tailSeconds();
}

void ReverbAU::syncParams() {
    reverb_.setRoomSize  (GetParameter(kParam_RoomSize));
    reverb_.setDamping   (GetParameter(kParam_Damping));
    reverb_.setPreDelayMs(GetParameter(kParam_PreDelay));

    eq_.setBand1LowShelf (GetParameter(kParam_EQ1Freq), GetParameter(kParam_EQ1Gain));
    eq_.setBand2Peaking  (GetParameter(kParam_EQ2Freq), GetParameter(kParam_EQ2Gain),
                          GetParameter(kParam_EQ2Q));
    eq_.setBand3Peaking  (GetParameter(kParam_EQ3Freq), GetParameter(kParam_EQ3Gain),
                          GetParameter(kParam_EQ3Q));
    eq_.setBand4HighShelf(GetParameter(kParam_EQ4Freq), GetParameter(kParam_EQ4Gain));
}

OSStatus ReverbAU::ProcessBufferLists(AudioUnitRenderActionFlags& ioFlags,
                                      const AudioBufferList&      inBufs,
                                      AudioBufferList&            outBufs,
                                      UInt32                      inFrames) {
    syncParams();

    if (inBufs.mNumberBuffers < 2 || outBufs.mNumberBuffers < 2)
        return kAudioUnitErr_FormatNotSupported;

    const float* inL  = static_cast<const float*>(inBufs.mBuffers[0].mData);
    const float* inR  = static_cast<const float*>(inBufs.mBuffers[1].mData);
    float*       outL = static_cast<float*>(outBufs.mBuffers[0].mData);
    float*       outR = static_cast<float*>(outBufs.mBuffers[1].mData);

    const int n = static_cast<int>(inFrames);

    // Grow wet buffers if block size has increased.
    if (n > static_cast<int>(wetL_.size())) {
        wetL_.resize(n, 0.f);
        wetR_.resize(n, 0.f);
    }

    // 1. Generate wet-only reverb signal.
    reverb_.process(inL, inR, wetL_.data(), wetR_.data(), n);

    // 2. Shape reverb tail with 4-band EQ (wet signal only).
    eq_.process(wetL_.data(), wetR_.data(), n);

    // 3. Wet/dry mix.
    const float wet = GetParameter(kParam_WetDry);
    const float dry = 1.f - wet;
    for (int i = 0; i < n; ++i) {
        outL[i] = dry * inL[i] + wet * wetL_[i];
        outR[i] = dry * inR[i] + wet * wetR_[i];
    }

    ioFlags &= ~kAudioUnitRenderAction_OutputIsSilence;
    return noErr;
}
