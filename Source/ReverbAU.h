#pragma once

#include <AUPublic/AUBase/AUEffectBase.h>
#include "FDNReverb.h"

// Parameter IDs
enum ReverbParam : AudioUnitParameterID {
    kParam_RoomSize  = 0,
    kParam_Damping   = 1,
    kParam_WetDry    = 2,
    kParam_PreDelay  = 3,
    kNumParams       = 4
};

static constexpr OSType kReverbAU_Manufacturer = 'Demo';
static constexpr OSType kReverbAU_SubType      = 'Rvb1';
static constexpr OSType kReverbAU_Type         = kAudioUnitType_Effect;

class ReverbAU : public AUEffectBase {
public:
    explicit ReverbAU(AudioUnit component);

    // AUEffectBase overrides
    OSStatus Initialize() override;
    void     Cleanup()    override;

    OSStatus GetParameterInfo(AudioUnitScope            inScope,
                              AudioUnitParameterID      inID,
                              AudioUnitParameterInfo&   outInfo) override;

    // Process stereo buffers directly (FDN needs cross-channel access).
    OSStatus ProcessBufferLists(AudioUnitRenderActionFlags& ioFlags,
                                const AudioBufferList&      inBufs,
                                AudioBufferList&            outBufs,
                                UInt32                      inFrames) override;

    bool    SupportsTail()  override { return true; }
    Float64 GetTailTime()   override;

    OSStatus Version() override { return 0x00010000; }

    // AUEffectBase requires this even if we don't use per-channel kernels.
    AUKernelBase* NewKernel() override { return nullptr; }

private:
    FDNReverb reverb_;

    void syncParams();
};
