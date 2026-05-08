# FDN Reverb — Logic Pro Audio Unit Plugin

Algorithmic reverb using an 8-line **Feedback Delay Network (FDN)** with a
Householder feedback matrix.  Targets Logic Pro (and any AU v2-compatible host)
on macOS.

## Parameters

### Reverb

| Parameter    | Range       | Default | Description                              |
|------------- |------------ |---------|------------------------------------------|
| Room Size    | 0.0 – 1.0   | 0.5     | RT60 from ~0.3 s (small) to ~8 s (large)|
| Damping      | 0.0 – 1.0   | 0.5     | High-frequency absorption                |
| Wet/Dry Mix  | 0.0 – 1.0   | 0.5     | 0 = dry only, 1 = wet only               |
| Pre-Delay    | 0 – 100 ms  | 20 ms   | Time before reverb tail begins           |

### 4-Band EQ (applied to wet/reverb signal only)

| Parameter      | Range            | Default  | Typical vocal use                        |
|--------------- |----------------- |----------|------------------------------------------|
| EQ1 Freq       | 20 – 1 000 Hz    | 300 Hz   | Low Shelf — cut low-end rumble           |
| EQ1 Gain       | -18 – +18 dB     | 0 dB     |                                          |
| EQ2 Freq       | 100 – 5 000 Hz   | 500 Hz   | Peak — reduce muddiness (~300–600 Hz)    |
| EQ2 Gain       | -18 – +18 dB     | 0 dB     |                                          |
| EQ2 Q          | 0.1 – 10         | 1.4      |                                          |
| EQ3 Freq       | 500 – 16 000 Hz  | 3 000 Hz | Peak — add presence/intelligibility      |
| EQ3 Gain       | -18 – +18 dB     | 0 dB     |                                          |
| EQ3 Q          | 0.1 – 10         | 1.4      |                                          |
| EQ4 Freq       | 2 000 – 20 000 Hz| 8 000 Hz | High Shelf — control air & brightness    |
| EQ4 Gain       | -18 – +18 dB     | 0 dB     |                                          |

**Starting point for vocals:** EQ1 low-shelf cut around 200–300 Hz (−4 to −6 dB)
removes low-end boxiness; EQ3 presence peak around 2–4 kHz adds intelligibility
without making the reverb harsh; EQ4 high-shelf rolloff above 10 kHz tames
excessive brightness on bright/sibilant voices.

## Algorithm

```
Input (stereo) ──▶ mono sum ──▶ Pre-delay ──▶ ┬──▶ delay[0] ──▶ LP ──▶ ─┐
                                               │    delay[1] ──▶ LP ──▶  │
                                               │    ...                   │
                                               │    delay[7] ──▶ LP ──▶ ─┤
                                               └─────────────────────────┘
                                                    Householder mix
Output L/R (interleaved even/odd lines for stereo spread)
```

- **8 delay lines** with lengths based on Schroeder primes, scaled to sample rate.
- **Householder reflection** (`y_i = x_i − (2/N)·Σx`) for optimal diffusion.
- **One-pole lowpass** per line simulates air/material absorption.
- **Gain per line** set by `g = 10^(−3·delay/RT60)` where RT60 is derived from Room Size.

## Building (macOS + Xcode)

### Prerequisites

```bash
xcode-select --install
brew install cmake
```

The Apple **AU Public SDK** source ships inside Xcode:

```
/Applications/Xcode.app/Contents/Developer/Extras/CoreAudio/
```

### Build & install

```bash
cmake -B build -G Xcode \
    -DAU_SDK_PATH="/Applications/Xcode.app/Contents/Developer/Extras/CoreAudio"
cmake --build build --config Release
cmake --install build --config Release
```

This installs `ReverbAU.component` to `~/Library/Audio/Plug-Ins/Components/`.

### Refresh AU cache in Logic Pro

```bash
auval -a    # verify the component is found
```

Then: Logic Pro → Preferences → Plug-in Manager → "Reset & Rescan Selection".

## Project structure

```
.
├── Source/
│   ├── FDNReverb.h / .cpp   — DSP engine (platform-independent)
│   └── ReverbAU.h  / .cpp   — Audio Unit wrapper
├── Resources/
│   └── ReverbAU.r            — Component registration resource
├── Info.plist                — Bundle metadata
└── CMakeLists.txt            — Build configuration
```

## Customisation tips

| Goal | Where |
|------|-------|
| Change RT60 range | `FDNReverb::setRoomSize()` exponent |
| More diffusion | Increase `NUM_LINES` to 16 and add all-pass pre-filters |
| Stereo width | Adjust odd/even line routing in `FDNReverb::process()` |
| GUI | Add an `AUCocoaUIView` target linked to the parameter IDs |
