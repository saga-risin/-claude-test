cmake_minimum_required(VERSION 3.21)
project(ReverbAU VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")

# ── Locate AU SDK (included in macOS SDK) ─────────────────────────────────────
# Apple's AU Public SDK is distributed as source in the Audio Unit examples package.
# Download from: https://developer.apple.com/library/archive/samplecode/AudioUnitEffectExample/
# Set AU_SDK_PATH to the extracted "PublicUtility" / "AUPublic" tree.
#
# Alternatively, install via: brew install --cask xcode  (SDK ships with Xcode)
set(AU_SDK_PATH "" CACHE PATH "Path to Apple AU Public SDK (AUPublic parent directory)")

if(NOT AU_SDK_PATH)
    # Try a common location inside the Xcode bundle.
    set(AU_SDK_PATH
        "/Applications/Xcode.app/Contents/Developer/Extras/CoreAudio")
endif()

# ── Plugin bundle ─────────────────────────────────────────────────────────────
add_library(ReverbAU MODULE
    Source/FDNReverb.cpp
    Source/BiquadEQ.cpp
    Source/ReverbAU.cpp
)

target_include_directories(ReverbAU PRIVATE
    Source
    "${AU_SDK_PATH}/AudioUnits/AUPublic/AUBase"
    "${AU_SDK_PATH}/AudioUnits/AUPublic/Utility"
    "${AU_SDK_PATH}/AudioUnits/AUPublic/OtherBases"
    "${AU_SDK_PATH}/PublicUtility"
)

target_compile_options(ReverbAU PRIVATE
    -Wall -Wextra -Wpedantic
    -O2
    # Silence deprecation noise from the AU SDK itself.
    -Wno-deprecated-declarations
)

target_link_libraries(ReverbAU PRIVATE
    "-framework AudioUnit"
    "-framework AudioToolbox"
    "-framework CoreFoundation"
)

# Bundle packaging
set_target_properties(ReverbAU PROPERTIES
    BUNDLE              TRUE
    BUNDLE_EXTENSION    "component"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist"
    OUTPUT_NAME         "ReverbAU"
)

# ── Install to user AU folder ──────────────────────────────────────────────────
install(TARGETS ReverbAU
    DESTINATION "$ENV{HOME}/Library/Audio/Plug-Ins/Components"
)
