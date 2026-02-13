#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>

class AudioStreamVSTAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  AudioStreamVSTAudioProcessorEditor(AudioStreamVSTAudioProcessor &);
  ~AudioStreamVSTAudioProcessorEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private:
  AudioStreamVSTAudioProcessor &audioProcessor;

  juce::Label serverLabel{"serverLabel", "Server Address"};
  juce::TextEditor serverEditor;

  juce::Label volumeLabel{"volumeLabel", "Volume"};
  juce::Slider volumeSlider;

  juce::TextButton connectButton{"Connect"};
  juce::TextButton disconnectButton{"Disconnect"};

  juce::TextEditor logEditor;
  void log(const juce::String &message);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
      AudioStreamVSTAudioProcessorEditor)
};
