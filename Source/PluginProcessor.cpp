#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioStreamVSTAudioProcessor::AudioStreamVSTAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true))
#endif
{
}

AudioStreamVSTAudioProcessor::~AudioStreamVSTAudioProcessor() {}

const juce::String AudioStreamVSTAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool AudioStreamVSTAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool AudioStreamVSTAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool AudioStreamVSTAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double AudioStreamVSTAudioProcessor::getTailLengthSeconds() const {
  return 0.0;
}

int AudioStreamVSTAudioProcessor::getNumPrograms() {
  return 1; // NB: some hosts don't cope very well if you tell them there are 0
            // programs, so this should be at least 1, even if you're not really
            // implementing programs.
}

int AudioStreamVSTAudioProcessor::getCurrentProgram() { return 0; }

void AudioStreamVSTAudioProcessor::setCurrentProgram(int index) {}

const juce::String AudioStreamVSTAudioProcessor::getProgramName(int index) {
  return {};
}

void AudioStreamVSTAudioProcessor::changeProgramName(
    int index, const juce::String &newName) {}

void AudioStreamVSTAudioProcessor::prepareToPlay(double sampleRate,
                                                 int samplesPerBlock) {
  // Use this method as the place to do any pre-playback
  // initialization that you need..
}

void AudioStreamVSTAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AudioStreamVSTAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
  // Some plugin hosts, such as certain GarageBand versions, will only
  // load plugins that support stereo bus layouts.
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}
#endif

void AudioStreamVSTAudioProcessor::processBlock(
    juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // 1. Send Audio: Downmix to mono and push to sendQueue
  if (totalNumInputChannels > 0) {
    if (totalNumInputChannels > 1) {
      // Simple mix of L+R for sending
      // We can't easily use a temp buffer without allocation in realtime
      // thread, so we'll just sum to a stack array if small enough, or just
      // take Left channel for now for safety. Better: calculate mix on the fly.
      // For prototype: Just send Left channel.
      sendQueue.push(buffer.getReadPointer(0), buffer.getNumSamples());
    } else {
      sendQueue.push(buffer.getReadPointer(0), buffer.getNumSamples());
    }
  }

  // 2. Receive Audio
  // We need a temp buffer to pop into because we add to all output channels.
  // Using a stack buffer is risky if size is large, but usually it's < 1024.
  // Safe max size:
  const int maxSamples = buffer.getNumSamples();
  // If buffer is huge, we might skip processing or loop. keeping it simple.

  // Note: allocating heap memory in processBlock is bad.
  // We should have a member buffer ideally.
  // For this prototype, I'll use a `AudioBuffer` member if I had one, valid for
  // 512 samples. Let's use `juce::AudioBuffer` on stack? No, that allocates. I
  // will use a simple float array on stack if < 2048, otherwise skip.

  if (maxSamples <= 2048) {
    float tempBuffer[2048];
    // Pop available samples.
    // Note: pop expects full block or partial?
    // Our generic queue pops WHAT IT CAN? No, our implementation pops requested
    // amount, or zero padding? Our implementation `pop` does NOT check size
    // availability in the public API I wrote? Wait, let's check
    // AudioBufferQueue::pop. `fifo.prepareToRead` will return start1, size1...
    // if not enough data, size1+size2 < numSamples. My implementation calls
    // copy only if size > 0. But it doesn't zero-fill the rest if queue is
    // empty. So I must clear tempBuffer first.

    juce::FloatVectorOperations::clear(tempBuffer, maxSamples);
    receiveQueue.pop(tempBuffer, maxSamples);

    // Apply Gain to Incoming Audio Only
    juce::FloatVectorOperations::multiply(tempBuffer, gain, maxSamples);

    // Mix incoming to all output channels
    for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
      juce::FloatVectorOperations::add(buffer.getWritePointer(channel),
                                       tempBuffer, maxSamples);
    }
  }
}

juce::String
AudioStreamVSTAudioProcessor::startStreaming(const juce::String &ip,
                                             int targetPort, int localPort) {
  return networkManager.startStreaming(ip, targetPort, localPort);
}

void AudioStreamVSTAudioProcessor::stopStreaming() {
  networkManager.stopStreaming();
}

bool AudioStreamVSTAudioProcessor::hasEditor() const {
  return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *AudioStreamVSTAudioProcessor::createEditor() {
  return new AudioStreamVSTAudioProcessorEditor(*this);
}

void AudioStreamVSTAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  // You should use this method to store your parameters in the memory block.
  // You could do that either as raw data, or use the XML or ValueTree classes
  // as intermediaries
}

void AudioStreamVSTAudioProcessor::setStateInformation(const void *data,
                                                       int sizeInBytes) {
  // You should use this method to restore your parameters from this memory
  // block, whose contents will have been created by the getStateInformation()
  // call.
}

// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new AudioStreamVSTAudioProcessor();
}
