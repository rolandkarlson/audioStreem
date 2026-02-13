#include "PluginEditor.h"
#include "PluginProcessor.h"

AudioStreamVSTAudioProcessorEditor::AudioStreamVSTAudioProcessorEditor(
    AudioStreamVSTAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
  setSize(400, 200);

  addAndMakeVisible(serverLabel);
  addAndMakeVisible(serverEditor);
  serverEditor.setText("sample-expires.gl.at.ply.gg:31171");

  addAndMakeVisible(volumeLabel);
  addAndMakeVisible(volumeSlider);
  volumeSlider.setRange(0.0, 2.0, 0.01);
  volumeSlider.setValue(audioProcessor.gain);
  volumeSlider.onValueChange = [this] {
    audioProcessor.gain = (float)volumeSlider.getValue();
  };

  addAndMakeVisible(connectButton);
  addAndMakeVisible(disconnectButton);

  addAndMakeVisible(logEditor);
  logEditor.setMultiLine(true);
  logEditor.setReadOnly(true);
  logEditor.setScrollbarsShown(true);
  logEditor.setCaretVisible(false);
  log("Ready.");

  connectButton.onClick = [this] {
    juce::String text = serverEditor.getText();

    // Parse Server Address (URL:Port)
    juce::String targetIP = "127.0.0.1";
    int port = 5000;

    int colIndex = text.lastIndexOfChar(':');
    if (colIndex > 0) {
      targetIP = text.substring(0, colIndex);
      port = text.substring(colIndex + 1).getIntValue();
    } else {
      targetIP = text;
    }

    log("Connecting to " + targetIP + ":" + juce::String(port) + "...");

    // Omnidirectional: Send to User:Port, Listen on Local:Port
    juce::String result = audioProcessor.startStreaming(targetIP, port, port);

    if (result.isEmpty()) {
      log("Success! Listening on port " + juce::String(port));
    } else {
      log("Error: " + result);
    }
  };

  disconnectButton.onClick = [this] {
    audioProcessor.stopStreaming();
    log("Disconnected.");
  };
}

AudioStreamVSTAudioProcessorEditor::~AudioStreamVSTAudioProcessorEditor() {}

void AudioStreamVSTAudioProcessorEditor::log(const juce::String &message) {
  logEditor.moveCaretToEnd();
  logEditor.insertTextAtCaret(message + "\n");
}

void AudioStreamVSTAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  g.setColour(juce::Colours::white);
  g.setFont(15.0f);
  g.drawFittedText("Audio Stream VST (Omni)",
                   getLocalBounds().removeFromTop(30),
                   juce::Justification::centred, 1);
}

void AudioStreamVSTAudioProcessorEditor::resized() {
  auto area = getLocalBounds().reduced(20);
  area.removeFromTop(30); // Title

  auto row = area.removeFromTop(30);
  serverLabel.setBounds(row.removeFromLeft(100));
  serverEditor.setBounds(row);

  area.removeFromTop(10);
  row = area.removeFromTop(30);
  volumeLabel.setBounds(row.removeFromLeft(100));
  volumeSlider.setBounds(row);

  area.removeFromTop(20);
  connectButton.setBounds(area.removeFromTop(30));
  area.removeFromTop(10);
  disconnectButton.setBounds(area.removeFromTop(30));

  area.removeFromTop(20);
  logEditor.setBounds(area);
}
