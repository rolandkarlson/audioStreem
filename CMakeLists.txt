cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for Mac OS X" FORCE)

project(AudioStreamVST VERSION 0.1.0)

# Import JUCE
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(juce)

# Create the plugin target
juce_add_plugin(AudioStreamVST
    COMPANY_NAME "Antigravity"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    
    # Formats to build
    FORMATS VST3 AU Standalone
    
    # Plugin characteristics
    PRODUCT_NAME "AudioStreamVST"
    PLUGIN_NAME "AudioStreamVST"
    plugin_code "AudS"
    plugin_manufacturer_code "Anti"
)

# Source files
target_sources(AudioStreamVST PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/NetworkManager.cpp
    Source/NetworkManager.h
    Source/AudioBufferQueue.h
)

# Generate JuceHeader.h for convenience
juce_generate_juce_header(AudioStreamVST)

# Link requirements
target_compile_definitions(AudioStreamVST PUBLIC
    JUCE_WEB_BROWSER_WORKAROUND=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(AudioStreamVST PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

if (APPLE)
    add_custom_command(TARGET AudioStreamVST_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "$<TARGET_FILE_DIR:AudioStreamVST_VST3>/../.."
        "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/AudioStreamVST.vst3"
        COMMENT "Copying VST3 to ~/Library/Audio/Plug-Ins/VST3"
    )
endif()
