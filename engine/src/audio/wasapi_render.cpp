/**
 * WindowsAiMic - WASAPI Render Implementation
 *
 * WASAPI audio rendering to the virtual speaker device.
 */

#include "wasapi_render.h"
#include <algorithm>
#include <cmath>
#include <iostream>

#ifdef _WIN32
#include <Audioclient.h>
#include <avrt.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <propidl.h> // For PROPERTYKEY
// Manually define PKEY_Device_FriendlyName to avoid build issues
DEFINE_PROPERTYKEY(PKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80,
                   0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);
#pragma comment(lib, "avrt.lib")
#endif

namespace WindowsAiMic {

WasapiRender::WasapiRender()
    : ringBuffer_(48000 * 2) // 2 seconds at 48kHz mono
{}

WasapiRender::~WasapiRender() {
  stop();
  cleanup();
}

void WasapiRender::cleanup() {
#ifdef _WIN32
  if (waveFormat_) {
    CoTaskMemFree(waveFormat_);
    waveFormat_ = nullptr;
  }
  if (renderClient_) {
    renderClient_->Release();
    renderClient_ = nullptr;
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
  initialized_ = false;
}

bool WasapiRender::initialize(const std::wstring &deviceId) {
  cleanup();
  return initializeDevice(deviceId);
}

bool WasapiRender::initializeDevice(const std::wstring &deviceId) {
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

  // Get render device (default if not specified)
  if (deviceId.empty()) {
    hr =
        deviceEnumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &device_);
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

  std::cout << "Render format: " << sampleRate_ << " Hz, " << channels_
            << " channels, " << bitsPerSample_ << " bits" << std::endl;

  // Create event for audio callbacks
  audioEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (!audioEvent_) {
    std::cerr << "Failed to create audio event" << std::endl;
    return false;
  }

  // Initialize audio client in shared mode
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

  // Get buffer size
  hr = audioClient_->GetBufferSize(&bufferFrameCount_);
  if (FAILED(hr)) {
    std::cerr << "Failed to get buffer size: 0x" << std::hex << hr << std::endl;
    return false;
  }

  // Get render client
  hr = audioClient_->GetService(__uuidof(IAudioRenderClient),
                                reinterpret_cast<void **>(&renderClient_));
  if (FAILED(hr)) {
    std::cerr << "Failed to get render client: 0x" << std::hex << hr
              << std::endl;
    return false;
  }

  initialized_ = true;

  // Reset ring buffer
  std::lock_guard<std::mutex> lock(bufferMutex_);
  writePos_ = 0;
  readPos_ = 0;

  return true;
#else
  return false;
#endif
}

void WasapiRender::start() {
  if (!initialized_.load() || running_.load()) {
    return;
  }

#ifdef _WIN32
  HRESULT hr = audioClient_->Start();
  if (FAILED(hr)) {
    std::cerr << "Failed to start audio render: 0x" << std::hex << hr
              << std::endl;
    return;
  }
#endif

  running_ = true;
  renderThread_ = std::thread(&WasapiRender::renderThread, this);
}

void WasapiRender::stop() {
  if (!running_.load()) {
    return;
  }

  running_ = false;

#ifdef _WIN32
  // Signal event to wake up thread
  if (audioEvent_) {
    SetEvent(audioEvent_);
  }
#endif

  bufferCv_.notify_all();

  if (renderThread_.joinable()) {
    renderThread_.join();
  }

#ifdef _WIN32
  if (audioClient_) {
    audioClient_->Stop();
  }
#endif
}

void WasapiRender::write(const float *buffer, size_t frames) {
  if (!initialized_.load()) {
    return;
  }

  std::lock_guard<std::mutex> lock(bufferMutex_);

  for (size_t i = 0; i < frames; ++i) {
    ringBuffer_[writePos_] = buffer[i];
    writePos_ = (writePos_ + 1) % ringBuffer_.size();

    // Overwrite oldest data if buffer full
    if (writePos_ == readPos_) {
      readPos_ = (readPos_ + 1) % ringBuffer_.size();
    }
  }

  bufferCv_.notify_one();
}

void WasapiRender::renderThread() {
#ifdef _WIN32
  // Boost thread priority for real-time audio
  DWORD taskIndex = 0;
  HANDLE hTask = AvSetMmThreadCharacteristics(L"Pro Audio", &taskIndex);
  if (!hTask) {
    std::cerr << "Warning: Failed to set thread characteristics" << std::endl;
  }

  while (running_.load()) {
    // Wait for audio buffer ready
    DWORD result = WaitForSingleObject(audioEvent_, 100);

    if (!running_.load()) {
      break;
    }

    if (result != WAIT_OBJECT_0) {
      continue;
    }

    // Get buffer padding (how much is still queued)
    UINT32 padding = 0;
    HRESULT hr = audioClient_->GetCurrentPadding(&padding);
    if (FAILED(hr)) {
      continue;
    }

    // Calculate available space
    UINT32 framesAvailable = bufferFrameCount_ - padding;
    if (framesAvailable == 0) {
      continue;
    }

    // Get render buffer
    BYTE *data = nullptr;
    hr = renderClient_->GetBuffer(framesAvailable, &data);
    if (FAILED(hr)) {
      continue;
    }

    // Read from ring buffer and convert/copy to render buffer
    size_t framesRead = 0;
    float *floatData = reinterpret_cast<float *>(data);

    {
      std::lock_guard<std::mutex> lock(bufferMutex_);

      while (framesRead < framesAvailable && readPos_ != writePos_) {
        float sample = ringBuffer_[readPos_];
        readPos_ = (readPos_ + 1) % ringBuffer_.size();

        // If output is stereo, duplicate mono to both channels
        if (channels_ == 2) {
          floatData[framesRead * 2] = sample;
          floatData[framesRead * 2 + 1] = sample;
        } else {
          floatData[framesRead] = sample;
        }

        ++framesRead;
      }
    }

    // Fill remaining with silence if needed
    for (size_t i = framesRead; i < framesAvailable; ++i) {
      if (channels_ == 2) {
        floatData[i * 2] = 0.0f;
        floatData[i * 2 + 1] = 0.0f;
      } else {
        floatData[i] = 0.0f;
      }
    }

    // Release buffer
    hr = renderClient_->ReleaseBuffer(framesAvailable, 0);
    if (FAILED(hr)) {
      std::cerr << "Failed to release buffer: 0x" << std::hex << hr
                << std::endl;
    }
  }

  if (hTask) {
    AvRevertMmThreadCharacteristics(hTask);
  }
#endif
}

std::vector<std::pair<std::string, std::wstring>>
WasapiRender::enumerateDevices() {
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

  hr =
      enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);
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
