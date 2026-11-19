cmake_minimum_required(VERSION 3.22)

# Change these two lines to rename the plugin.
set(PLUGIN_NAME "ArcEQ")
set(PLUGIN_DISPLAY_NAME "Arc EQ")

project(${PLUGIN_NAME} VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build universal-capable, but default to Apple Silicon only (fast builds on a MacBook Air).
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# JUCE: use a local checkout if JUCE_DIR is given, otherwise download it.
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()

juce_add_plugin(${PLUGIN_NAME}
    COMPANY_NAME "Ti"
    PRODUCT_NAME "${PLUGIN_DISPLAY_NAME}"
    BUNDLE_ID "com.ti.arceq"
    PLUGIN_MANUFACTURER_CODE Tity
    PLUGIN_CODE Arq1
    FORMATS ${PLUGIN_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx EQ
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(${PLUGIN_NAME} PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SpectrumAnalyzer.cpp
    Source/EqCurveView.cpp)

target_compile_definitions(${PLUGIN_NAME} PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(${PLUGIN_NAME}
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
