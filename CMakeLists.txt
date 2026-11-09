cmake_minimum_required(VERSION 3.22)

project(LadderSynth VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Universal binary on macOS, so it runs on Apple silicon and Intel Macs.
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# ---------------------------------------------------------------- engine tests
# The sound engine is plain C++, so its tests build without JUCE.
option(LADDER_BUILD_TESTS "Build the headless engine tests" ON)
if(LADDER_BUILD_TESTS)
    enable_testing()
    add_executable(EngineTests tests/EngineTests.cpp Source/dsp/SynthEngine.cpp)
    add_test(NAME EngineTests COMMAND EngineTests)
endif()

option(LADDER_BUILD_PLUGIN "Build the plugin (needs JUCE)" ON)
if(NOT LADDER_BUILD_PLUGIN)
    return()
endif()

# ---------------------------------------------------------------- JUCE
# Uses a local JUCE checkout if JUCE_DIR is given, otherwise downloads it.
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional)")
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------- plugin
if(APPLE)
    set(LADDER_FORMATS AU VST3 Standalone)
else()
    set(LADDER_FORMATS VST3 Standalone)
endif()

juce_add_plugin(LadderSynth
    PRODUCT_NAME "Ladder Synth"
    COMPANY_NAME "Jarglew"
    BUNDLE_ID "com.jarglew.laddersynth"
    PLUGIN_MANUFACTURER_CODE Jrgl
    PLUGIN_CODE Ldr1
    FORMATS ${LADDER_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(LadderSynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/Presets.cpp
    Source/dsp/SynthEngine.cpp)

target_compile_definitions(LadderSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(LadderSynth
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------- plugin smoke test
# Drives the real processor and editor without a host. Off by default.
option(LADDER_BUILD_PLUGIN_TESTS "Build the plugin smoke test" OFF)
if(LADDER_BUILD_PLUGIN_TESTS)
    juce_add_console_app(PluginSmokeTest PRODUCT_NAME "PluginSmokeTest")
    # Compiles the plugin's own sources into a plain app, so no plugin wrapper is involved.
    target_sources(PluginSmokeTest PRIVATE
        tests/PluginSmokeTest.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Parameters.cpp
        Source/Presets.cpp
        Source/dsp/SynthEngine.cpp)
    target_compile_definitions(PluginSmokeTest PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JucePlugin_Name="Ladder Synth")
    target_link_libraries(PluginSmokeTest PRIVATE
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
    add_test(NAME PluginSmokeTest COMMAND PluginSmokeTest)
endif()
