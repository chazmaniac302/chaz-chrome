cmake_minimum_required(VERSION 3.22)

# Static MSVC runtime (/MT): the .vst3 does not need the VC++ redistributable on the FL Studio machine.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(ChazChrome VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(CHAZ_BUILD_TESTS  "Build the headless engine tests" ON)
option(CHAZ_BUILD_PLUGIN "Build the JUCE plugin (VST3 + Standalone)" ON)
option(CHAZ_CROSS_TESTS  "Also build test executables when cross-compiling (run them with wine)" OFF)

# ------------------------------------------------------------------------------
# Engine (plain C++, no JUCE)
# ------------------------------------------------------------------------------
add_library(ChazEngine STATIC Source/Engine/ChromeEngine.cpp)
target_include_directories(ChazEngine PUBLIC Source/Engine)
set_target_properties(ChazEngine PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    add_compile_options(/utf-8)
    target_compile_options(ChazEngine PRIVATE /W4 /O2 /fp:fast)
else()
    target_compile_options(ChazEngine PRIVATE -Wall -Wextra -O2)
endif()

if(CHAZ_BUILD_TESTS AND (CHAZ_CROSS_TESTS OR NOT CMAKE_CROSSCOMPILING))
    enable_testing()
    add_executable(EngineTests Tests/EngineTests.cpp)
    target_link_libraries(EngineTests PRIVATE ChazEngine)
    add_test(NAME ChazChromeEngineTests COMMAND EngineTests ${CMAKE_BINARY_DIR}/test-output)
endif()

if(NOT CHAZ_BUILD_PLUGIN)
    return()
endif()

# ------------------------------------------------------------------------------
# JUCE: local checkout (-DCHAZ_JUCE_DIR=...) or fetched from GitHub
# ------------------------------------------------------------------------------
set(CHAZ_JUCE_DIR "" CACHE PATH "Path to a JUCE checkout (leave empty to download JUCE ${CHAZ_JUCE_TAG})")
set(CHAZ_JUCE_TAG "8.0.16-security" CACHE STRING "JUCE git tag to download")
if(CHAZ_JUCE_DIR)
    add_subdirectory(${CHAZ_JUCE_DIR} JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${CHAZ_JUCE_TAG}
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(CMAKE_CROSSCOMPILING)
    set(CHAZ_VST3_MANIFEST FALSE)
else()
    set(CHAZ_VST3_MANIFEST TRUE)
endif()

# Instrument: MIDI in -> melodic trap / hip-hop lead
juce_add_plugin(ChazChrome
    PRODUCT_NAME               "Chaz Chrome"
    COMPANY_NAME               "Chazmaniac"
    COMPANY_WEBSITE            ""
    BUNDLE_ID                  "com.chazmaniac.chazchrome"
    PLUGIN_MANUFACTURER_CODE   Chzm
    PLUGIN_CODE                ChCr
    FORMATS                    VST3 Standalone
    IS_SYNTH                   TRUE
    NEEDS_MIDI_INPUT           TRUE
    NEEDS_MIDI_OUTPUT          FALSE
    IS_MIDI_EFFECT             FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD    FALSE
    VST3_CATEGORIES            Instrument Synth
    VST3_AUTO_MANIFEST         ${CHAZ_VST3_MANIFEST}
    NEEDS_CURL                 FALSE
    NEEDS_WEB_BROWSER          FALSE)

target_sources(ChazChrome PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Presets.cpp)

target_compile_definitions(ChazChrome PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_MP3AUDIOFORMAT=0
    JUCE_USE_FLAC=0
    JUCE_USE_OGGVORBIS=0
    JUCE_USE_WINDOWS_MEDIA_FORMAT=0
    JUCE_USE_WEBP=0
    JUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE=0)

target_link_libraries(ChazChrome
    PRIVATE
        ChazEngine
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(CHAZ_BUILD_TESTS AND (CHAZ_CROSS_TESTS OR NOT CMAKE_CROSSCOMPILING))
    add_executable(PluginTests Tests/PluginTests.cpp)
    target_link_libraries(PluginTests PRIVATE ChazChrome ChazEngine)
    target_include_directories(PluginTests PRIVATE $<TARGET_PROPERTY:ChazChrome,INCLUDE_DIRECTORIES>)
    target_compile_definitions(PluginTests PRIVATE $<TARGET_PROPERTY:ChazChrome,COMPILE_DEFINITIONS>)
    add_test(NAME ChazChromePluginTests COMMAND PluginTests ${CMAKE_BINARY_DIR}/test-output)
endif()
