#include "NetworkManager.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

NetworkManager::NetworkManager(AudioBufferQueue &sQ, AudioBufferQueue &rQ)
    : Thread("NetworkThread"), sendQueue(sQ), receiveQueue(rQ) {
  floatBuffer.resize(packetSampleCount);
  pcmBuffer.resize(packetSampleCount);
  // Header + data size (int16 = 2 bytes)
  receiveBuffer.resize(sizeof(AudioPacketHeader) + packetSampleCount * 2);
}

NetworkManager::~NetworkManager() { stopStreaming(); }

juce::String NetworkManager::startStreaming(const juce::String &targetIP,
                                            int sendPort, int listenPort) {
  stopStreaming();

  // Resolve hostname if needed
  juce::IPAddress ip(targetIP);
  if (!ip.isNull()) // If it's a valid IP string (not 0.0.0.0), use it directly
  {
    remoteIP = targetIP;
  } else // Otherwise try to resolve hostname
  {
    struct addrinfo hints = {};
    hints.ai_family = AF_INET; // IPv4 only for now
    hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo *res = nullptr;

    if (getaddrinfo(targetIP.toRawUTF8(), nullptr, &hints, &res) == 0) {
      char host[256];
      if (getnameinfo(res->ai_addr, res->ai_addrlen, host, sizeof(host),
                      nullptr, 0, NI_NUMERICHOST) == 0) {
        remoteIP = juce::String(host);
      } else {
        remoteIP = targetIP;
        // Fail soft, but maybe warn?
      }
      freeaddrinfo(res);
    } else {
      remoteIP = targetIP;
      // Fail soft
    }
  }

  txPort = sendPort;
  rxPort = listenPort;

  // Single Socket: Bind to local port.
  // This allows us to receive on this port AND send from this port.
  udpSocket = std::make_unique<juce::DatagramSocket>(false);

  if (udpSocket->bindToPort(rxPort)) {
    DBG("UDP Socket bound to port " << rxPort);
    isStreaming = true;
    startThread();
    return {}; // Empty string = Success
  } else {
    DBG("UDP Socket failed to bind to port " << rxPort);
    return "Failed to bind to port " + juce::String(rxPort);
  }
}

void NetworkManager::stopStreaming() {
  isStreaming = false;
  stopThread(2000);

  if (udpSocket)
    udpSocket->shutdown();

  udpSocket.reset();
}

void NetworkManager::run() {
  while (!threadShouldExit() && isStreaming) {
    sendPendingAudio();
    checkIncomingPackets();
  }
}

void NetworkManager::sendPendingAudio() {
  if (udpSocket && sendQueue.getNumReady() >= packetSampleCount) {
    // 1. Pop float audio
    sendQueue.pop(floatBuffer.data(), packetSampleCount);

    // 2. Convert to Int16
    auto *src = floatBuffer.data();
    auto *dst = pcmBuffer.data();
    for (int i = 0; i < packetSampleCount; ++i) {
      dst[i] = (int16_t)(juce::jlimit(-1.0f, 1.0f, src[i]) * 32767.0f);
    }

    // 3. Prepare Packet
    juce::MemoryBlock packetData;
    AudioPacketHeader header;
    header.sequenceNumber = sequenceCounter++;
    header.sampleRate = 44100;
    header.channels = 1;
    header.payloadSize = packetSampleCount;

    packetData.append(&header, sizeof(header));
    packetData.append(pcmBuffer.data(), packetSampleCount * sizeof(int16_t));

    // 4. Send (Explicit target)
    udpSocket->write(remoteIP, txPort, packetData.getData(),
                     (int)packetData.getSize());
  }
}

void NetworkManager::checkIncomingPackets() {
  if (!udpSocket)
    return;

  int bytesRead = udpSocket->waitUntilReady(true, 5); // Wait up to 5ms for READ
  if (bytesRead > 0) {
    int bytesReceived =
        udpSocket->read(receiveBuffer.data(), (int)receiveBuffer.size(), false);

    if (bytesReceived > (int)sizeof(AudioPacketHeader)) {
      AudioPacketHeader header;
      memcpy(&header, receiveBuffer.data(), sizeof(header));

      int sampleCount = header.payloadSize;
      int16_t *pcmData =
          reinterpret_cast<int16_t *>(receiveBuffer.data() + sizeof(header));

      if (sampleCount > 0 && sampleCount <= packetSampleCount) {
        // Convert back to float
        auto *dst = floatBuffer.data();
        const float inv32767 = 1.0f / 32767.0f;
        for (int i = 0; i < sampleCount; ++i) {
          dst[i] = (float)pcmData[i] * inv32767;
        }

        // Push to receive queue
        receiveQueue.push(floatBuffer.data(), sampleCount);
      }
    }
  }
}
