cmake_minimum_required(VERSION 3.22)
project(ResonanceDetector VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE automatically if not found locally
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(ResonanceDetector
    COMPANY_NAME              "YourCompany"
    COMPANY_WEBSITE           "https://example.com"
    PLUGIN_MANUFACTURER_CODE  Yocm
    PLUGIN_CODE               Rsd1
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   TRUE
    FORMATS                   $<IF:$<PLATFORM_ID:Darwin>,AU VST3,VST3>
    PRODUCT_NAME              "Resonance Detector"
    BUNDLE_ID                 "com.yourcompany.resonancedetector"
)

juce_generate_juce_header(ResonanceDetector)

target_sources(ResonanceDetector
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(ResonanceDetector
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(ResonanceDetector
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
