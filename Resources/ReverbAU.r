// Audio Unit component registration resource.
// Compiled with Rez on macOS.
#include <AudioUnit/AudioUnit.r>

resource 'aunm' (0) {
    kAudioUnitType_Effect,   // component type
    'Rvb1',                  // component subtype
    'Demo'                   // manufacturer
};

resource 'STR ' (kAudioUnitNameKey) {
    "FDN Reverb"
};

resource 'STR ' (kAudioUnitVersionKey) {
    "1.0.0"
};

resource 'STR ' (kAudioUnitDescriptionKey) {
    "Feedback Delay Network reverb with Room Size, Damping, Wet/Dry and Pre-Delay."
};

resource 'STR ' (kAudioUnitIconSmallKey) {
    ""
};
