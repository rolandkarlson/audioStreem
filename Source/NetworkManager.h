#pragma once
#include "AudioBufferQueue.h"
#include <JuceHeader.h>

struct AudioPacketHeader {
  int64_t sequenceNumber;
  int32_t sampleRate;
  int16_t channels;
  int32_t payloadSize; // Number of samples
};

class NetworkManager : public juce::Thread {
public:
  NetworkManager(AudioBufferQueue &sendQ, AudioBufferQueue &receiveQ);
  ~NetworkManager() override;

  void run() override;

  juce::String startStreaming(const juce::String &targetIP, int targetPort,
                              int localPort);
  void stopStreaming();

private:
  void sendPendingAudio();
  void checkIncomingPackets();

  AudioBufferQueue &sendQueue;
  AudioBufferQueue &receiveQueue;

  std::unique_ptr<juce::DatagramSocket> udpSocket;

  juce::String remoteIP;
  int txPort = 0;
  int rxPort = 0;

  std::atomic<bool> isStreaming{false};
  std::atomic<int64_t> sequenceCounter{0};

  // Intermediate buffers
  static constexpr int packetSampleCount = 512;
  std::vector<float> floatBuffer;
  std::vector<int16_t> pcmBuffer;
  std::vector<uint8_t> receiveBuffer;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkManager)
};
