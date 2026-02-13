#pragma once
#include <JuceHeader.h>

class AudioBufferQueue {
public:
  AudioBufferQueue() {
    fifo.setTotalSize(bufferSize);
    buffer.setSize(1,
                   bufferSize); // Mono for simplicity in transmission initially
  }

  void push(const float *data, int numSamples) {
    int start1, size1, start2, size2;
    fifo.prepareToWrite(numSamples, start1, size1, start2, size2);

    if (size1 > 0)
      buffer.copyFrom(0, start1, data, size1);
    if (size2 > 0)
      buffer.copyFrom(0, start2, data + size1, size2);

    fifo.finishedWrite(size1 + size2);
  }

  void pop(float *outputData, int numSamples) {
    int start1, size1, start2, size2;
    fifo.prepareToRead(numSamples, start1, size1, start2, size2);

    if (size1 > 0) {
      juce::FloatVectorOperations::copy(
          outputData, buffer.getReadPointer(0, start1), size1);
    }
    if (size2 > 0) {
      juce::FloatVectorOperations::copy(
          outputData + size1, buffer.getReadPointer(0, start2), size2);
    }

    fifo.finishedRead(size1 + size2);
  }

  int getNumReady() const { return fifo.getNumReady(); }
  int getFreeSpace() const { return fifo.getFreeSpace(); }

private:
  static constexpr int bufferSize = 48000 * 2; // 2 seconds buffer
  juce::AbstractFifo fifo{bufferSize};
  juce::AudioBuffer<float> buffer;
};
