/**
 * WindowsAiMic - OpenVINO AI Processor Implementation
 *
 * Note: This is a stub implementation. Full implementation requires:
 * - OpenVINO Runtime installation
 * - A trained noise suppression model in ONNX/IR format
 */

#include "openvino_processor.h"
#include "../platform/cpu_features.h"
#include <algorithm>
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#endif

// Uncomment when OpenVINO is available:
// #include <openvino/openvino.hpp>

namespace WindowsAiMic {

struct OpenVINOProcessor::Impl {
  // OpenVINO objects would go here:
  // ov::Core core;
  // ov::CompiledModel model;
  // ov::InferRequest inferRequest;

  bool available = false;
};

OpenVINOProcessor::OpenVINOProcessor()
    : frameBuffer_(FRAME_SIZE, 0.0f), impl_(std::make_unique<Impl>()) {}

OpenVINOProcessor::~OpenVINOProcessor() = default;

bool OpenVINOProcessor::isAvailable() {
  // Check if OpenVINO runtime is installed
  // In a full implementation, try to create ov::Core

#ifdef HAS_OPENVINO
  try {
    ov::Core core;
    return true;
  } catch (...) {
    return false;
  }
#else
// Check if OpenVINO DLLs are present
#ifdef _WIN32
  HMODULE hMod = LoadLibraryA("openvino.dll");
  if (hMod) {
    FreeLibrary(hMod);
    return true;
  }
#endif
  return false;
#endif
}

bool OpenVINOProcessor::hasNPU() {
  // Check if Intel NPU is available
  const auto &cpu = CPUFeatures::get();

  if (!cpu.hasNPU()) {
    return false;
  }

#ifdef HAS_OPENVINO
  try {
    ov::Core core;
    auto devices = core.get_available_devices();
    for (const auto &device : devices) {
      if (device.find("NPU") != std::string::npos) {
        return true;
      }
    }
  } catch (...) {
  }
#endif

  return false;
}

std::vector<std::string> OpenVINOProcessor::getAvailableDevices() {
  std::vector<std::string> devices;

#ifdef HAS_OPENVINO
  try {
    ov::Core core;
    devices = core.get_available_devices();
  } catch (...) {
  }
#else
  // Return what we know from CPU detection
  const auto &cpu = CPUFeatures::get();

  devices.push_back("CPU");

  if (cpu.hasNPU()) {
    devices.push_back("NPU");
  }
#endif

  return devices;
}

bool OpenVINOProcessor::initialize() {
  std::cout << "Initializing OpenVINO processor..." << std::endl;

  // Check if OpenVINO is available
  if (!isAvailable()) {
    std::cerr
        << "OpenVINO runtime not available. Install OpenVINO or use RNNoise."
        << std::endl;
    return false;
  }

  // Try to load model
  if (!loadModel()) {
    std::cerr << "Failed to load AI model for OpenVINO" << std::endl;
    return false;
  }

  initialized_ = true;
  bufferPos_ = 0;

  std::cout << "OpenVINO initialized on device: " << currentDevice_
            << std::endl;
  return true;
}

bool OpenVINOProcessor::loadModel() {
#ifdef HAS_OPENVINO
  try {
    ov::Core core;

    // Set device priority: NPU > GPU > CPU
    std::string device = preferredDevice_;
    if (device == "AUTO") {
      auto devices = core.get_available_devices();

      // Prefer NPU for Intel Core Ultra
      for (const auto &d : devices) {
        if (d.find("NPU") != std::string::npos) {
          device = d;
          break;
        }
      }

      // Fallback to GPU, then CPU
      if (device == "AUTO") {
        for (const auto &d : devices) {
          if (d.find("GPU") != std::string::npos) {
            device = d;
            break;
          }
        }
      }

      if (device == "AUTO") {
        device = "CPU";
      }
    }

    // Load model
    auto model = core.read_model(modelPath_);

    // Compile for target device
    impl_->model = core.compile_model(model, device);
    impl_->inferRequest = impl_->model.create_infer_request();
    impl_->available = true;

    currentDevice_ = device;
    return true;

  } catch (const std::exception &e) {
    std::cerr << "OpenVINO error: " << e.what() << std::endl;
    return false;
  }
#else
  std::cout << "OpenVINO not compiled in. This is a stub implementation."
            << std::endl;
  std::cout << "To enable: Install OpenVINO and rebuild with -DHAS_OPENVINO=ON"
            << std::endl;

  // In stub mode, pretend we're using CPU
  currentDevice_ = "CPU (stub)";
  impl_->available = false;
  return true; // Return true so we can fall back to passthrough
#endif
}

void OpenVINOProcessor::process(float *buffer, size_t frames) {
  if (!initialized_) {
    return;
  }

  size_t inputPos = 0;

  while (inputPos < frames) {
    // Fill frame buffer
    size_t toCopy = std::min(frames - inputPos, FRAME_SIZE - bufferPos_);
    std::copy(buffer + inputPos, buffer + inputPos + toCopy,
              frameBuffer_.begin() + bufferPos_);

    bufferPos_ += toCopy;
    inputPos += toCopy;

    // Process complete frame
    if (bufferPos_ == FRAME_SIZE) {
      if (impl_->available) {
        processFrameNPU(frameBuffer_.data());
      } else {
        processFrameCPU(frameBuffer_.data());
      }

      // Copy processed frame back
      std::copy(frameBuffer_.begin(), frameBuffer_.end(),
                buffer + inputPos - FRAME_SIZE);

      bufferPos_ = 0;
    }
  }
}

void OpenVINOProcessor::processFrameNPU(float *frame) {
#ifdef HAS_OPENVINO
  // Set input tensor
  auto input_tensor = impl_->inferRequest.get_input_tensor();
  float *input_data = input_tensor.data<float>();
  std::copy(frame, frame + FRAME_SIZE, input_data);

  // Run inference
  impl_->inferRequest.infer();

  // Get output
  auto output_tensor = impl_->inferRequest.get_output_tensor();
  const float *output_data = output_tensor.data<float>();
  std::copy(output_data, output_data + FRAME_SIZE, frame);
#else
  (void)frame;
#endif
}

void OpenVINOProcessor::processFrameCPU(float *frame) {
  // Stub: passthrough
  // In a full implementation, this would run a CPU-based noise suppressor
  (void)frame;
}

void OpenVINOProcessor::reset() {
  bufferPos_ = 0;
  std::fill(frameBuffer_.begin(), frameBuffer_.end(), 0.0f);
}

std::string OpenVINOProcessor::getName() const {
  return "OpenVINO (" + currentDevice_ + ")";
}

void OpenVINOProcessor::setDevice(const std::string &device) {
  preferredDevice_ = device;
}

void OpenVINOProcessor::setModelPath(const std::string &path) {
  modelPath_ = path;
}

} // namespace WindowsAiMic
