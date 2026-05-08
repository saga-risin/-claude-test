#include "ReverbAU.h"

#include <AudioUnit/AudioUnit.h>
#include <CoreFoundation/CoreFoundation.h>

// ── Factory entry point ───────────────────────────────────────────────────────

AUDIOCOMPONENT_ENTRY(AUBaseFactory, ReverbAU)

// ── Constructor ───────────────────────────────────────────────────────────────

ReverbAU::ReverbAU(AudioUnit component)
    : AUEffectBase(component, false /* not in-place only */) {
    // Register default parameter values.
    SetParameter(kParam_RoomSize, 0.5f);
    SetParameter(kParam_Damping,  0.5f);
    SetParameter(kParam_WetDry,   0.5f);
    SetParameter(kParam_PreDelay, 20.0f);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

OSStatus ReverbAU::Initialize() {
    OSStatus err = AUEffectBase::Initialize();
    if (err != noErr) return err;

    reverb_.prepare(GetSampleRate());
    syncParams();
    return noErr;
}

void ReverbAU::Cleanup() {
    reverb_.reset();
    AUEffectBase::Cleanup();
}

// ── Parameters ────────────────────────────────────────────────────────────────

OSStatus ReverbAU::GetParameterInfo(AudioUnitScope         inScope,
                                    AudioUnitParameterID   inID,
                                    AudioUnitParameterInfo& outInfo) {
    if (inScope != kAudioUnitScope_Global) return kAudioUnitErr_InvalidScope;

    outInfo.flags = kAudioUnitParameterFlag_IsReadable |
                    kAudioUnitParameterFlag_IsWritable;

    switch (inID) {
    case kParam_RoomSize:
        AUBase::FillInParameterName(outInfo, CFSTR("Room Size"), false);
        outInfo.unit         = kAudioUnitParameterUnit_Generic;
        outInfo.minValue     = 0.0f;
        outInfo.maxValue     = 1.0f;
        outInfo.defaultValue = 0.5f;
        break;

    case kParam_Damping:
        AUBase::FillInParameterName(outInfo, CFSTR("Damping"), false);
        outInfo.unit         = kAudioUnitParameterUnit_Generic;
        outInfo.minValue     = 0.0f;
        outInfo.maxValue     = 1.0f;
        outInfo.defaultValue = 0.5f;
        break;

    case kParam_WetDry:
        AUBase::FillInParameterName(outInfo, CFSTR("Wet/Dry Mix"), false);
        outInfo.unit         = kAudioUnitParameterUnit_Generic;
        outInfo.minValue     = 0.0f;
        outInfo.maxValue     = 1.0f;
        outInfo.defaultValue = 0.5f;
        break;

    case kParam_PreDelay:
        AUBase::FillInParameterName(outInfo, CFSTR("Pre-Delay (ms)"), false);
        outInfo.unit         = kAudioUnitParameterUnit_Milliseconds;
        outInfo.minValue     = 0.0f;
        outInfo.maxValue     = 100.0f;
        outInfo.defaultValue = 20.0f;
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
    reverb_.setRoomSize(GetParameter(kParam_RoomSize));
    reverb_.setDamping(GetParameter(kParam_Damping));
    reverb_.setWet(GetParameter(kParam_WetDry));
    reverb_.setPreDelayMs(GetParameter(kParam_PreDelay));
}

OSStatus ReverbAU::ProcessBufferLists(AudioUnitRenderActionFlags& ioFlags,
                                      const AudioBufferList&      inBufs,
                                      AudioBufferList&            outBufs,
                                      UInt32                      inFrames) {
    // Sync parameters every block (parameter changes are cheap).
    syncParams();

    // Require stereo (2 buffers of float32).
    if (inBufs.mNumberBuffers < 2 || outBufs.mNumberBuffers < 2)
        return kAudioUnitErr_FormatNotSupported;

    const float* inL  = static_cast<const float*>(inBufs.mBuffers[0].mData);
    const float* inR  = static_cast<const float*>(inBufs.mBuffers[1].mData);
    float*       outL = static_cast<float*>(outBufs.mBuffers[0].mData);
    float*       outR = static_cast<float*>(outBufs.mBuffers[1].mData);

    reverb_.process(inL, inR, outL, outR, static_cast<int>(inFrames));

    ioFlags &= ~kAudioUnitRenderAction_OutputIsSilence;
    return noErr;
}
