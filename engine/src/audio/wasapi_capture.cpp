/**
 * WindowsAiMic - WASAPI Capture Implementation
 *
 * Event-driven WASAPI audio capture implementation.
 */

#include "wasapi_capture.h"
#include <algorithm>
#include <cmath>
#include <iostream>

#ifdef _WIN32
#include <Audioclient.h>
#include <avrt.h>
#include <functiondiscoverykeys_devpkey.h>
#include <mmdeviceapi.h>
#pragma comment(lib, "avrt.lib")
#endif

namespace WindowsAiMic {

WasapiCapture::WasapiCapture() = default;

WasapiCapture::~WasapiCapture() {
  stop();
  cleanup();
}

void WasapiCapture::cleanup() {
#ifdef _WIN32
  if (waveFormat_) {
    CoTaskMemFree(waveFormat_);
    waveFormat_ = nullptr;
  }
  if (captureClient_) {
    captureClient_->Release();
    captureClient_ = nullptr;
  }
  if (audioClient_) {
    audioClient_->Release();
    audioClient_ = nullptr;
  }
  if (device_) {
    device_->Release();
    device_ = nullptr;
  }
  if (deviceEnumerator_) {
    deviceEnumerator_->Release();
    deviceEnumerator_ = nullptr;
  }
  if (audioEvent_) {
    CloseHandle(audioEvent_);
    audioEvent_ = nullptr;
  }
#endif
}

bool WasapiCapture::initialize(const std::wstring &deviceId) {
  cleanup();
  return initializeDevice(deviceId);
}

bool WasapiCapture::initializeDevice(const std::wstring &deviceId) {
#ifdef _WIN32
  HRESULT hr;

  // Create device enumerator
  hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                        __uuidof(IMMDeviceEnumerator),
                        reinterpret_cast<void **>(&deviceEnumerator_));
  if (FAILED(hr)) {
    std::cerr << "Failed to create device enumerator: 0x" << std::hex << hr
              << std::endl;
    return false;
  }

  // Get capture device
  if (deviceId.empty()) {
    // Use default capture device
    hr = deviceEnumerator_->GetDefaultAudioEndpoint(eCapture, eConsole,
                                                    &device_);
  } else {
    hr = deviceEnumerator_->GetDevice(deviceId.c_str(), &device_);
  }

  if (FAILED(hr)) {
    std::cerr << "Failed to get audio device: 0x" << std::hex << hr
              << std::endl;
    return false;
  }

  // Activate audio client
  hr = device_->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                         reinterpret_cast<void **>(&audioClient_));
  if (FAILED(hr)) {
    std::cerr << "Failed to activate audio client: 0x" << std::hex << hr
              << std::endl;
    return false;
  }

  // Get mix format
  hr = audioClient_->GetMixFormat(&waveFormat_);
  if (FAILED(hr)) {
    std::cerr << "Failed to get mix format: 0x" << std::hex << hr << std::endl;
    return false;
  }

  sampleRate_ = waveFormat_->nSamplesPerSec;
  channels_ = waveFormat_->nChannels;
  bitsPerSample_ = waveFormat_->wBitsPerSample;

  std::cout << "Capture format: " << sampleRate_ << " Hz, " << channels_
            << " channels, " << bitsPerSample_ << " bits" << std::endl;

  // Create event for audio callbacks
  audioEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (!audioEvent_) {
    std::cerr << "Failed to create audio event" << std::endl;
    return false;
  }

  // Initialize audio client in shared mode with event callback
  // Use a 20ms buffer for low latency
  REFERENCE_TIME bufferDuration = 200000; // 20ms in 100ns units

  hr = audioClient_->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                bufferDuration, 0, waveFormat_, nullptr);
  if (FAILED(hr)) {
    std::cerr << "Failed to initialize audio client: 0x" << std::hex << hr
              << std::endl;
    return false;
  }

  // Set event handle
  hr = audioClient_->SetEventHandle(audioEvent_);
  if (FAILED(hr)) {
    std::cerr << "Failed to set event handle: 0x" << std::hex << hr
              << std::endl;
    return false;
  }

  // Get capture client
  hr = audioClient_->GetService(__uuidof(IAudioCaptureClient),
                                reinterpret_cast<void **>(&captureClient_));
  if (FAILED(hr)) {
    std::cerr << "Failed to get capture client: 0x" << std::hex << hr
              << std::endl;
    return false;
  }

  // Pre-allocate conversion buffer
  UINT32 bufferSize;
  hr = audioClient_->GetBufferSize(&bufferSize);
  if (SUCCEEDED(hr)) {
    conversionBuffer_.resize(bufferSize * channels_);
  }

  return true;
#else
  return false;
#endif
}

void WasapiCapture::start() {
  if (capturing_.load()) {
    return;
  }

#ifdef _WIN32
  HRESULT hr = audioClient_->Start();
  if (FAILED(hr)) {
    std::cerr << "Failed to start audio capture: 0x" << std::hex << hr
              << std::endl;
    return;
  }
#endif

  capturing_ = true;
  captureThread_ = std::thread(&WasapiCapture::captureThread, this);
}

void WasapiCapture::stop() {
  if (!capturing_.load()) {
    return;
  }

  capturing_ = false;

#ifdef _WIN32
  // Signal event to wake up thread
  if (audioEvent_) {
    SetEvent(audioEvent_);
  }
#endif

  if (captureThread_.joinable()) {
    captureThread_.join();
  }

#ifdef _WIN32
  if (audioClient_) {
    audioClient_->Stop();
  }
#endif
}

void WasapiCapture::setCallback(AudioCallback callback) {
  callback_ = std::move(callback);
}

void WasapiCapture::captureThread() {
#ifdef _WIN32
  // Boost thread priority for real-time audio
  DWORD taskIndex = 0;
  HANDLE hTask = AvSetMmThreadCharacteristics(L"Pro Audio", &taskIndex);
  if (!hTask) {
    std::cerr << "Warning: Failed to set thread characteristics" << std::endl;
  }

  while (capturing_.load()) {
    // Wait for audio data
    DWORD result = WaitForSingleObject(audioEvent_, 100);

    if (!capturing_.load()) {
      break;
    }

    if (result != WAIT_OBJECT_0) {
      continue;
    }

    // Get captured data
    UINT32 packetLength = 0;
    HRESULT hr = captureClient_->GetNextPacketSize(&packetLength);

    while (SUCCEEDED(hr) && packetLength > 0) {
      BYTE *data = nullptr;
      UINT32 numFrames = 0;
      DWORD flags = 0;

      hr = captureClient_->GetBuffer(&data, &numFrames, &flags, nullptr,
                                     nullptr);
      if (FAILED(hr)) {
        break;
      }

      if (callback_ && numFrames > 0) {
        // Convert to float if needed
        if (waveFormat_->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
            (waveFormat_->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             reinterpret_cast<WAVEFORMATEXTENSIBLE *>(waveFormat_)->SubFormat ==
                 KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
          // Already float
          if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
            std::fill(conversionBuffer_.begin(),
                      conversionBuffer_.begin() + numFrames * channels_, 0.0f);
            callback_(conversionBuffer_.data(), numFrames, sampleRate_,
                      channels_);
          } else {
            callback_(reinterpret_cast<float *>(data), numFrames, sampleRate_,
                      channels_);
          }
        } else if (bitsPerSample_ == 16) {
          // Convert 16-bit to float
          const int16_t *src = reinterpret_cast<const int16_t *>(data);
          size_t totalSamples = numFrames * channels_;

          if (conversionBuffer_.size() < totalSamples) {
            conversionBuffer_.resize(totalSamples);
          }

          if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
            std::fill(conversionBuffer_.begin(),
                      conversionBuffer_.begin() + totalSamples, 0.0f);
          } else {
            for (size_t i = 0; i < totalSamples; ++i) {
              conversionBuffer_[i] = static_cast<float>(src[i]) / 32768.0f;
            }
          }

          callback_(conversionBuffer_.data(), numFrames, sampleRate_,
                    channels_);
        } else if (bitsPerSample_ == 24) {
          // Convert 24-bit to float
          const uint8_t *src = data;
          size_t totalSamples = numFrames * channels_;

          if (conversionBuffer_.size() < totalSamples) {
            conversionBuffer_.resize(totalSamples);
          }

          if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
            std::fill(conversionBuffer_.begin(),
                      conversionBuffer_.begin() + totalSamples, 0.0f);
          } else {
            for (size_t i = 0; i < totalSamples; ++i) {
              int32_t sample = (src[i * 3] << 8) | (src[i * 3 + 1] << 16) |
                               (src[i * 3 + 2] << 24);
              conversionBuffer_[i] = static_cast<float>(sample) / 2147483648.0f;
            }
          }

          callback_(conversionBuffer_.data(), numFrames, sampleRate_,
                    channels_);
        }
      }

      captureClient_->ReleaseBuffer(numFrames);
      hr = captureClient_->GetNextPacketSize(&packetLength);
    }
  }

  if (hTask) {
    AvRevertMmThreadCharacteristics(hTask);
  }
#endif
}

std::vector<std::pair<std::string, std::wstring>>
WasapiCapture::enumerateDevices() {
  std::vector<std::pair<std::string, std::wstring>> devices;

#ifdef _WIN32
  IMMDeviceEnumerator *enumerator = nullptr;
  IMMDeviceCollection *collection = nullptr;

  HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                reinterpret_cast<void **>(&enumerator));

  if (FAILED(hr)) {
    return devices;
  }

  hr = enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE,
                                      &collection);
  if (FAILED(hr)) {
    enumerator->Release();
    return devices;
  }

  UINT count = 0;
  collection->GetCount(&count);

  for (UINT i = 0; i < count; ++i) {
    IMMDevice *device = nullptr;
    hr = collection->Item(i, &device);
    if (FAILED(hr)) {
      continue;
    }

    // Get device ID
    LPWSTR deviceId = nullptr;
    device->GetId(&deviceId);

    // Get friendly name
    IPropertyStore *props = nullptr;
    device->OpenPropertyStore(STGM_READ, &props);

    PROPVARIANT varName;
    PropVariantInit(&varName);
    props->GetValue(PKEY_Device_FriendlyName, &varName);

    // Convert to std::string
    int size = WideCharToMultiByte(CP_UTF8, 0, varName.pwszVal, -1, nullptr, 0,
                                   nullptr, nullptr);
    std::string name(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, varName.pwszVal, -1, &name[0], size,
                        nullptr, nullptr);

    devices.emplace_back(name, deviceId);

    PropVariantClear(&varName);
    CoTaskMemFree(deviceId);
    props->Release();
    device->Release();
  }

  collection->Release();
  enumerator->Release();
#endif

  return devices;
}

} // namespace WindowsAiMic
