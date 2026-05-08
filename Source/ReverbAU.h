#pragma once

#include <AUPublic/AUBase/AUEffectBase.h>
#include <vector>
#include "FDNReverb.h"
#include "BiquadEQ.h"

// ── Parameter IDs ─────────────────────────────────────────────────────────────
enum ReverbParam : AudioUnitParameterID {
    // Reverb
    kParam_RoomSize  = 0,
    kParam_Damping   = 1,
    kParam_WetDry    = 2,
    kParam_PreDelay  = 3,

    // EQ — applied to wet signal only
    // Band 1: Low Shelf  (low-end rumble control, e.g. ~200-400 Hz)
    kParam_EQ1Freq   = 4,   // 20 – 1 000 Hz,     default 300 Hz
    kParam_EQ1Gain   = 5,   // -18 – +18 dB,       default 0 dB

    // Band 2: Peaking    (mud reduction, e.g. ~300-800 Hz)
    kParam_EQ2Freq   = 6,   // 100 – 5 000 Hz,     default 500 Hz
    kParam_EQ2Gain   = 7,   // -18 – +18 dB,       default 0 dB
    kParam_EQ2Q      = 8,   // 0.1 – 10,           default 1.4

    // Band 3: Peaking    (presence, e.g. 2 – 5 kHz)
    kParam_EQ3Freq   = 9,   // 500 – 16 000 Hz,    default 3 000 Hz
    kParam_EQ3Gain   = 10,  // -18 – +18 dB,       default 0 dB
    kParam_EQ3Q      = 11,  // 0.1 – 10,           default 1.4

    // Band 4: High Shelf (air / brightness, e.g. ~8-12 kHz)
    kParam_EQ4Freq   = 12,  // 2 000 – 20 000 Hz,  default 8 000 Hz
    kParam_EQ4Gain   = 13,  // -18 – +18 dB,       default 0 dB

    kNumParams       = 14
};

static constexpr OSType kReverbAU_Manufacturer = 'Demo';
static constexpr OSType kReverbAU_SubType      = 'Rvb1';
static constexpr OSType kReverbAU_Type         = kAudioUnitType_Effect;

class ReverbAU : public AUEffectBase {
public:
    explicit ReverbAU(AudioUnit component);

    OSStatus Initialize() override;
    void     Cleanup()    override;

    OSStatus GetParameterInfo(AudioUnitScope          inScope,
                              AudioUnitParameterID    inID,
                              AudioUnitParameterInfo& outInfo) override;

    OSStatus ProcessBufferLists(AudioUnitRenderActionFlags& ioFlags,
                                const AudioBufferList&      inBufs,
                                AudioBufferList&            outBufs,
                                UInt32                      inFrames) override;

    bool    SupportsTail()  override { return true; }
    Float64 GetTailTime()   override;
    OSStatus Version()      override { return 0x00010000; }

    AUKernelBase* NewKernel() override { return nullptr; }

private:
    FDNReverb reverb_;
    BiquadEQ  eq_;

    // Temporary buffers for wet signal (resized in Initialize).
    std::vector<float> wetL_, wetR_;

    void syncParams();
};
