/**
 * WindowsAiMic - Engine Implementation
 *
 * Main audio processing engine implementation.
 * Optimized for Intel Core Ultra processors.
 */

#include "engine.h"
#include "ai/openvino_processor.h"
#include "ai/rnnoise_processor.h"
#include "audio/resampler.h"
#include "audio/wasapi_capture.h"
#include "audio/wasapi_render.h"
#include "dsp/compressor.h"
#include "dsp/equalizer.h"
#include "dsp/expander.h"
#include "dsp/limiter.h"
#include "dsp/metering.h"
#include "ipc/pipe_server.h"
#include "platform/cpu_features.h"
#include "platform/simd_dsp.h"
#include "platform/thread_utils.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace WindowsAiMic {

Engine::Engine(ConfigManager &configManager)
    : configManager_(configManager), inputBuffer_(BUFFER_SIZE),
      outputBuffer_(BUFFER_SIZE), processingBuffer_(PROCESSING_BLOCK_SIZE) {}

Engine::~Engine() { stop(); }

bool Engine::initialize() {
  std::cout << "Initializing audio engine..." << std::endl;

  // Detect CPU capabilities (Intel Core Ultra optimizations)
  CPUFeatures::initialize();
  const auto &cpu = CPUFeatures::get();

  std::cout << "CPU Optimizations:" << std::endl;
  std::cout << "  - AVX2: " << (cpu.hasAVX2() ? "Yes" : "No") << std::endl;
  std::cout << "  - AVX-512: " << (cpu.hasAVX512() ? "Yes" : "No") << std::endl;
  std::cout << "  - NPU: " << (cpu.hasNPU() ? "Yes (Intel AI Boost)" : "No")
            << std::endl;
  if (cpu.isHybrid()) {
    std::cout << "  - Hybrid CPU: " << cpu.performanceCores() << " P-cores, "
              << cpu.efficiencyCores() << " E-cores" << std::endl;
  }

  // Initialize audio capture
  if (!initializeCapture()) {
    std::cerr << "Failed to initialize audio capture" << std::endl;
    return false;
  }

  // Initialize audio render
  if (!initializeRender()) {
    std::cerr << "Failed to initialize audio render" << std::endl;
    return false;
  }

  // Initialize processors
  if (!initializeProcessors()) {
    std::cerr << "Failed to initialize audio processors" << std::endl;
    return false;
  }

  // Initialize IPC for UI communication
  if (!initializeIPC()) {
    std::cerr << "Warning: Failed to initialize IPC server" << std::endl;
    // Continue anyway - UI communication is optional
  }

  return true;
}

bool Engine::initializeCapture() {
  capture_ = std::make_unique<WasapiCapture>();

  const auto &config = configManager_.getConfig();
  std::wstring inputDevice = config.devices.inputDevice;

  // Use default device if not specified
  if (inputDevice.empty()) {
    inputDevice = L""; // Empty string means default device in WASAPI
  }

  if (!capture_->initialize(inputDevice)) {
    return false;
  }

  // Set up callback for captured audio
  capture_->setCallback(
      [this](float *buffer, size_t frames, int sampleRate, int channels) {
        onAudioCaptured(buffer, frames, sampleRate, channels);
      });

  // Set up resampler if needed
  int captureSampleRate = capture_->getSampleRate();
  if (captureSampleRate != INTERNAL_SAMPLE_RATE) {
    inputResampler_ = std::make_unique<Resampler>();
    if (!inputResampler_->initialize(captureSampleRate, INTERNAL_SAMPLE_RATE,
                                     INTERNAL_CHANNELS)) {
      std::cerr << "Failed to initialize input resampler" << std::endl;
      return false;
    }
    std::cout << "Input resampler: " << captureSampleRate << " Hz -> "
              << INTERNAL_SAMPLE_RATE << " Hz" << std::endl;
  }

  return true;
}

bool Engine::initializeRender() {
  render_ = std::make_unique<WasapiRender>();

  const auto &config = configManager_.getConfig();
  std::wstring outputDevice = config.devices.outputDevice;

  // Auto-detect VB-Cable if no output device specified
  if (outputDevice.empty()) {
    std::cout << "Looking for virtual audio device..." << std::endl;
    auto devices = getOutputDevices();

    for (const auto &device : devices) {
      // Look for VB-Cable (CABLE Input is what we write to)
      if (device.first.find("CABLE Input") != std::string::npos ||
          device.first.find("VB-Audio") != std::string::npos) {
        outputDevice = device.second;
        std::cout << "  Found VB-Cable: " << device.first << std::endl;
        break;
      }
      // Also check for our custom driver
      if (device.first.find("Virtual Speaker") != std::string::npos ||
          device.first.find("WindowsAiMic") != std::string::npos) {
        outputDevice = device.second;
        std::cout << "  Found WindowsAiMic driver: " << device.first
                  << std::endl;
        break;
      }
    }

    if (outputDevice.empty()) {
      std::cerr << "No virtual audio device found!" << std::endl;
      std::cerr << "Please install VB-Cable from: https://vb-audio.com/Cable/"
                << std::endl;
      return false;
    }
  }

  if (!render_->initialize(outputDevice)) {
    return false;
  }

  // Set up output resampler if needed
  int renderSampleRate = render_->getSampleRate();
  if (renderSampleRate != INTERNAL_SAMPLE_RATE) {
    outputResampler_ = std::make_unique<Resampler>();
    if (!outputResampler_->initialize(INTERNAL_SAMPLE_RATE, renderSampleRate,
                                      INTERNAL_CHANNELS)) {
      std::cerr << "Failed to initialize output resampler" << std::endl;
      return false;
    }
    std::cout << "Output resampler: " << INTERNAL_SAMPLE_RATE << " Hz -> "
              << renderSampleRate << " Hz" << std::endl;
  }

  return true;
}

bool Engine::initializeProcessors() {
  const auto &config = configManager_.getConfig();

  // Initialize RNNoise
  rnnoise_ = std::make_unique<RNNoiseProcessor>();
  if (!rnnoise_->initialize()) {
    std::cerr << "Failed to initialize RNNoise" << std::endl;
    return false;
  }
  rnnoise_->setAttenuation(config.aiSettings.rnnoise.attenuation);
  std::cout << "RNNoise initialized" << std::endl;

#ifdef USE_DEEPFILTER
  // Initialize DeepFilterNet if enabled
  deepfilter_ = std::make_unique<DeepFilterProcessor>();
  if (!deepfilter_->initialize(config.aiSettings.deepfilter.modelPath)) {
    std::cerr << "Warning: Failed to initialize DeepFilterNet" << std::endl;
    deepfilter_.reset();
  }
#endif

  // Initialize DSP chain
  expander_ = std::make_unique<Expander>();
  expander_->setEnabled(config.expander.enabled);
  expander_->setThreshold(config.expander.threshold);
  expander_->setRatio(config.expander.ratio);
  expander_->setAttack(config.expander.attack);
  expander_->setRelease(config.expander.release);
  expander_->setHysteresis(config.expander.hysteresis);
  std::cout << "Expander initialized" << std::endl;

  compressor_ = std::make_unique<Compressor>();
  compressor_->setEnabled(config.compressor.enabled);
  compressor_->setThreshold(config.compressor.threshold);
  compressor_->setRatio(config.compressor.ratio);
  compressor_->setKnee(config.compressor.knee);
  compressor_->setAttack(config.compressor.attack);
  compressor_->setRelease(config.compressor.release);
  compressor_->setMakeupGain(config.compressor.makeupGain);
  std::cout << "Compressor initialized" << std::endl;

  limiter_ = std::make_unique<Limiter>();
  limiter_->setEnabled(config.limiter.enabled);
  limiter_->setCeiling(config.limiter.ceiling);
  limiter_->setRelease(config.limiter.release);
  limiter_->setLookahead(config.limiter.lookahead);
  std::cout << "Limiter initialized" << std::endl;

  equalizer_ = std::make_unique<Equalizer>();
  equalizer_->setEnabled(config.equalizer.enabled);
  equalizer_->setHighPass(config.equalizer.highPass.freq,
                          config.equalizer.highPass.q);
  equalizer_->setLowShelf(config.equalizer.lowShelf.freq,
                          config.equalizer.lowShelf.gain);
  equalizer_->setPresence(config.equalizer.presence.freq,
                          config.equalizer.presence.gain,
                          config.equalizer.presence.q);
  equalizer_->setHighShelf(config.equalizer.highShelf.freq,
                           config.equalizer.highShelf.gain);
  std::cout << "Equalizer initialized" << std::endl;

  // Initialize metering
  inputMetering_ = std::make_unique<Metering>();
  outputMetering_ = std::make_unique<Metering>();
  std::cout << "Metering initialized" << std::endl;

  return true;
}

bool Engine::initializeIPC() {
  pipeServer_ = std::make_unique<PipeServer>();

  // Set up config update callback
  pipeServer_->setConfigUpdateCallback([this](const Config &newConfig) {
    configManager_.applyConfig(newConfig);
    // Re-apply all settings
    applyPreset(newConfig.activePreset);
  });

  return pipeServer_->start();
}

void Engine::start() {
  if (running_.load()) {
    return;
  }

  running_ = true;

  // Start processing thread
  processingThread_ = std::thread(&Engine::processingThread, this);

  // Start audio capture
  capture_->start();

  // Start audio render
  render_->start();

  // Start IPC server
  if (pipeServer_) {
    pipeServer_->start();
  }

  std::lock_guard<std::mutex> lock(statusMutex_);
  status_.capturing = true;
  status_.rendering = true;
}

void Engine::stop() {
  if (!running_.load()) {
    return;
  }

  running_ = false;

  // Signal processing thread to wake up
  processingCv_.notify_all();

  // Stop audio I/O
  if (capture_) {
    capture_->stop();
  }
  if (render_) {
    render_->stop();
  }

  // Stop IPC
  if (pipeServer_) {
    pipeServer_->stop();
  }

  // Wait for processing thread
  if (processingThread_.joinable()) {
    processingThread_.join();
  }

  std::lock_guard<std::mutex> lock(statusMutex_);
  status_.capturing = false;
  status_.rendering = false;
}

void Engine::listAudioDevices() {
  std::cout << "\n=== Input Devices (Microphones) ===" << std::endl;
  auto inputs = getInputDevices();
  for (size_t i = 0; i < inputs.size(); ++i) {
    std::cout << "  [" << i << "] " << inputs[i].first << std::endl;
  }

  std::cout << "\n=== Output Devices (Speakers/Virtual) ===" << std::endl;
  auto outputs = getOutputDevices();
  for (size_t i = 0; i < outputs.size(); ++i) {
    std::cout << "  [" << i << "] " << outputs[i].first << std::endl;
  }
  std::cout << std::endl;
}

std::vector<std::pair<std::string, std::wstring>> Engine::getInputDevices() {
  WasapiCapture temp;
  return temp.enumerateDevices();
}

std::vector<std::pair<std::string, std::wstring>> Engine::getOutputDevices() {
  WasapiRender temp;
  return temp.enumerateDevices();
}

void Engine::onAudioCaptured(float *buffer, size_t frames, int sampleRate,
                             int channels) {
  // Convert to mono if stereo
  std::vector<float> monoBuffer(frames);
  if (channels == 2) {
    for (size_t i = 0; i < frames; ++i) {
      monoBuffer[i] = (buffer[i * 2] + buffer[i * 2 + 1]) * 0.5f;
    }
  } else {
    std::copy(buffer, buffer + frames, monoBuffer.begin());
  }

  // Resample to internal rate if needed
  std::vector<float> resampledBuffer;
  float *audioData = monoBuffer.data();
  size_t audioFrames = frames;

  if (inputResampler_) {
    resampledBuffer = inputResampler_->process(monoBuffer.data(), frames);
    audioData = resampledBuffer.data();
    audioFrames = resampledBuffer.size();
  }

  // Push to ring buffer
  inputBuffer_.write(audioData, audioFrames);

  // Signal processing thread
  processingCv_.notify_one();
}

void Engine::processingThread() {
  std::cout << "Processing thread started" << std::endl;

  // Set thread name for debugging
  setThreadName("AudioProcessing");

  // Intel Core Ultra optimization: Run on P-cores for consistent performance
  const auto &cpu = CPUFeatures::get();
  if (cpu.isHybrid()) {
    setThreadCorePreference(CorePreference::Performance);
    std::cout << "  Using P-core scheduling for audio processing" << std::endl;
  }

  // Set up multimedia thread mode (WASAPI MMCSS integration)
  MultimediaThreadScope mmcss("Pro Audio");
  if (mmcss.isActive()) {
    std::cout << "  MMCSS Pro Audio mode active" << std::endl;
  }

  while (running_.load()) {
    // Wait for data
    {
      std::unique_lock<std::mutex> lock(processingMutex_);
      processingCv_.wait_for(lock, std::chrono::milliseconds(10), [this]() {
        return inputBuffer_.availableRead() >= PROCESSING_BLOCK_SIZE ||
               !running_.load();
      });
    }

    if (!running_.load()) {
      break;
    }

    // Process available blocks
    while (inputBuffer_.availableRead() >= PROCESSING_BLOCK_SIZE) {
      // Read a block
      inputBuffer_.read(processingBuffer_.data(), PROCESSING_BLOCK_SIZE);

      // Process the block
      processAudioBlock(processingBuffer_.data(), PROCESSING_BLOCK_SIZE);

      // Write to output buffer
      outputBuffer_.write(processingBuffer_.data(), PROCESSING_BLOCK_SIZE);

      // Feed to render
      if (render_ && render_->isReady()) {
        std::vector<float> renderBuffer;

        // Resample for output if needed
        if (outputResampler_) {
          renderBuffer = outputResampler_->process(processingBuffer_.data(),
                                                   PROCESSING_BLOCK_SIZE);
          render_->write(renderBuffer.data(), renderBuffer.size());
        } else {
          render_->write(processingBuffer_.data(), PROCESSING_BLOCK_SIZE);
        }
      }
    }
  }

  std::cout << "Processing thread stopped" << std::endl;
}

void Engine::processAudioBlock(float *buffer, size_t frames) {
  // Update input metering
  if (inputMetering_) {
    inputMetering_->process(buffer, frames);
  }

  // Bypass mode - skip all processing
  if (bypass_.load()) {
    // Update output metering with unprocessed audio
    if (outputMetering_) {
      outputMetering_->process(buffer, frames);
    }
    return;
  }

  // AI Enhancement (RNNoise or DeepFilterNet)
  const auto &config = configManager_.getConfig();

  if (config.aiModel == "rnnoise" && rnnoise_) {
    rnnoise_->process(buffer, frames);
  }
#ifdef USE_DEEPFILTER
  else if (config.aiModel == "deepfilter" && deepfilter_) {
    deepfilter_->process(buffer, frames);
  }
#endif

  // DSP Chain

  // 1. Expander (noise gate)
  if (expander_ && expander_->isEnabled()) {
    expander_->process(buffer, frames);
  }

  // 2. Equalizer (before compression for tonal shaping)
  if (equalizer_ && equalizer_->isEnabled()) {
    equalizer_->process(buffer, frames);
  }

  // 3. Compressor
  if (compressor_ && compressor_->isEnabled()) {
    compressor_->process(buffer, frames);
  }

  // 4. Limiter
  if (limiter_ && limiter_->isEnabled()) {
    limiter_->process(buffer, frames);
  }

  // Update output metering
  if (outputMetering_) {
    outputMetering_->process(buffer, frames);
  }

  // Send meter updates if callback is set
  {
    std::lock_guard<std::mutex> lock(meterMutex_);
    if (meterCallback_) {
      float gainReduction =
          compressor_ ? compressor_->getGainReduction() : 0.0f;
      meterCallback_(outputMetering_->getPeak(), outputMetering_->getRMS(),
                     gainReduction);
    }
  }

  // Update status
  {
    std::lock_guard<std::mutex> lock(statusMutex_);
    status_.inputLevel = inputMetering_ ? inputMetering_->getRMS() : 0.0f;
    status_.outputLevel = outputMetering_ ? outputMetering_->getRMS() : 0.0f;
    status_.gainReduction =
        compressor_ ? compressor_->getGainReduction() : 0.0f;
  }
}

void Engine::setInputDevice(const std::wstring &deviceId) {
  if (running_.load()) {
    capture_->stop();
  }

  capture_->initialize(deviceId);

  if (running_.load()) {
    capture_->start();
  }
}

void Engine::setOutputDevice(const std::wstring &deviceId) {
  if (running_.load()) {
    render_->stop();
  }

  render_->initialize(deviceId);

  if (running_.load()) {
    render_->start();
  }
}

void Engine::setAIModel(const std::string &modelName) {
  auto config = configManager_.getConfig();
  config.aiModel = modelName;
  configManager_.applyConfig(config);
}

void Engine::applyPreset(const std::string &presetName) {
  auto config = configManager_.getConfig();

  if (presetName == "podcast") {
    // Warm, present voice with controlled dynamics
    config.expander = {true, -45.0f, 2.5f, 5.0f, 100.0f, 3.0f};
    config.compressor = {true, -16.0f, 3.5f, 6.0f, 10.0f, 100.0f, 6.0f};
    config.limiter = {true, -1.0f, 50.0f, 5.0f};
    config.equalizer.highPass = {80.0f, 0.7f};
    config.equalizer.lowShelf = {200.0f, 1.0f};
    config.equalizer.presence = {3000.0f, 3.0f, 1.0f};
    config.equalizer.highShelf = {8000.0f, 2.0f};
  } else if (presetName == "meeting") {
    // Natural, less aggressive processing
    config.expander = {true, -50.0f, 2.0f, 10.0f, 150.0f, 4.0f};
    config.compressor = {true, -20.0f, 2.5f, 8.0f, 15.0f, 150.0f, 4.0f};
    config.limiter = {true, -3.0f, 100.0f, 3.0f};
    config.equalizer.highPass = {100.0f, 0.7f};
    config.equalizer.lowShelf = {200.0f, 0.0f};
    config.equalizer.presence = {3000.0f, 1.5f, 1.0f};
    config.equalizer.highShelf = {10000.0f, 1.0f};
  } else if (presetName == "streaming") {
    // Punchy, broadcast-style
    config.expander = {true, -40.0f, 3.0f, 3.0f, 80.0f, 2.0f};
    config.compressor = {true, -14.0f, 4.5f, 4.0f, 5.0f, 80.0f, 8.0f};
    config.limiter = {true, -0.5f, 30.0f, 5.0f};
    config.equalizer.highPass = {80.0f, 0.8f};
    config.equalizer.lowShelf = {150.0f, 2.0f};
    config.equalizer.presence = {4000.0f, 4.0f, 1.2f};
    config.equalizer.highShelf = {12000.0f, 3.0f};
  }

  config.activePreset = presetName;
  configManager_.applyConfig(config);

  // Apply to processors
  setExpanderParams(config.expander);
  setCompressorParams(config.compressor);
  setLimiterParams(config.limiter);
  setEqualizerParams(config.equalizer);
}

void Engine::setBypass(bool bypass) { bypass_ = bypass; }

void Engine::setExpanderParams(const ExpanderConfig &params) {
  if (expander_) {
    expander_->setEnabled(params.enabled);
    expander_->setThreshold(params.threshold);
    expander_->setRatio(params.ratio);
    expander_->setAttack(params.attack);
    expander_->setRelease(params.release);
    expander_->setHysteresis(params.hysteresis);
  }
}

void Engine::setCompressorParams(const CompressorConfig &params) {
  if (compressor_) {
    compressor_->setEnabled(params.enabled);
    compressor_->setThreshold(params.threshold);
    compressor_->setRatio(params.ratio);
    compressor_->setKnee(params.knee);
    compressor_->setAttack(params.attack);
    compressor_->setRelease(params.release);
    compressor_->setMakeupGain(params.makeupGain);
  }
}

void Engine::setLimiterParams(const LimiterConfig &params) {
  if (limiter_) {
    limiter_->setEnabled(params.enabled);
    limiter_->setCeiling(params.ceiling);
    limiter_->setRelease(params.release);
    limiter_->setLookahead(params.lookahead);
  }
}

void Engine::setEqualizerParams(const EqualizerConfig &params) {
  if (equalizer_) {
    equalizer_->setEnabled(params.enabled);
    equalizer_->setHighPass(params.highPass.freq, params.highPass.q);
    equalizer_->setLowShelf(params.lowShelf.freq, params.lowShelf.gain);
    equalizer_->setPresence(params.presence.freq, params.presence.gain,
                            params.presence.q);
    equalizer_->setHighShelf(params.highShelf.freq, params.highShelf.gain);
  }
}

void Engine::setMeterCallback(MeterCallback callback) {
  std::lock_guard<std::mutex> lock(meterMutex_);
  meterCallback_ = std::move(callback);
}

Engine::Status Engine::getStatus() const {
  std::lock_guard<std::mutex> lock(statusMutex_);
  return status_;
}

} // namespace WindowsAiMic
