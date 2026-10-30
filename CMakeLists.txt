cmake_minimum_required(VERSION 3.22)

project(VocaMaxSpace2 VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/MP)
endif()

# ---------------------------------------------------------------------------
# JUCE: a local checkout wins, otherwise CMake fetches the pinned release.
# ---------------------------------------------------------------------------
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)

    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.10
        GIT_SHALLOW TRUE)

    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# Plugin
# ---------------------------------------------------------------------------
juce_add_plugin(VocaMaxSpace2
    COMPANY_NAME "Mark33s Beats Lab"
    BUNDLE_ID "com.mark33sbeatslab.vocamaxspace2"
    PLUGIN_MANUFACTURER_CODE Mark
    PLUGIN_CODE VMS2
    PRODUCT_NAME "VocaMax Space 2"
    DESCRIPTION "Vocal space, clarity and tone control for any DAW"
    VERSION "2.0.0"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx
    FORMATS VST3 Standalone)

target_sources(VocaMaxSpace2 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditorCompat.cpp)

target_include_directories(VocaMaxSpace2 PRIVATE Source)

target_compile_definitions(VocaMaxSpace2 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(VocaMaxSpace2
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if (WIN32)
    set_target_properties(VocaMaxSpace2 PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# ---------------------------------------------------------------------------
# Headless DSP checks (no JUCE required)
# ---------------------------------------------------------------------------
option(VOCAMAX_BUILD_DSP_TESTS "Build the headless VocaMax Space 2 DSP checks" OFF)

if (VOCAMAX_BUILD_DSP_TESTS)
    enable_testing()

    add_executable(VocaMaxDSPTests tests/dsp_test.cpp)
    target_include_directories(VocaMaxDSPTests PRIVATE Source)
    set_target_properties(VocaMaxDSPTests PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON)

    add_test(NAME VocaMaxDSPTests COMMAND VocaMaxDSPTests)
endif()
