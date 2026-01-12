/**
 * WindowsAiMic Virtual Audio Driver
 *
 * A minimal WDM audio driver that creates a virtual speaker and virtual
 * microphone. Audio written to the virtual speaker is looped back to the
 * virtual microphone.
 *
 * Build requires: Windows Driver Kit (WDK)
 */

#include <ksdebug.h>
#include <ntddk.h>
#include <portcls.h>
#include <stdunk.h>

// Forward declarations
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

// Driver globals
static PDEVICE_OBJECT g_DeviceObject = NULL;

// Device extension
typedef struct _DEVICE_EXTENSION {
  PDEVICE_OBJECT PhysicalDeviceObject;
  PDEVICE_OBJECT FunctionalDeviceObject;
  PDEVICE_OBJECT NextLowerDriver;

  // Audio buffer for loopback
  PUCHAR AudioBuffer;
  SIZE_T BufferSize;
  SIZE_T WritePosition;
  SIZE_T ReadPosition;
  KSPIN_LOCK BufferLock;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// Buffer size: 1 second of 48kHz, 16-bit, stereo audio
#define AUDIO_BUFFER_SIZE (48000 * 2 * 2)

/**
 * Driver entry point
 */
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject,
            _In_ PUNICODE_STRING RegistryPath) {
  NTSTATUS status;

  UNREFERENCED_PARAMETER(RegistryPath);

  DbgPrint("WindowsAiMic: DriverEntry\n");

  // Set up driver unload routine
  DriverObject->DriverUnload = DriverUnload;

  // Initialize PortCls for WDM audio
  status = PcInitializeAdapterDriver(DriverObject, RegistryPath,
                                     (PDRIVER_ADD_DEVICE)AddDevice);

  if (!NT_SUCCESS(status)) {
    DbgPrint("WindowsAiMic: PcInitializeAdapterDriver failed: 0x%08X\n",
             status);
    return status;
  }

  DbgPrint("WindowsAiMic: Driver loaded successfully\n");
  return STATUS_SUCCESS;
}

/**
 * Driver unload routine
 */
VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);
  DbgPrint("WindowsAiMic: DriverUnload\n");
}

/**
 * Add device routine - called when device is detected
 */
NTSTATUS
AddDevice(_In_ PDRIVER_OBJECT DriverObject,
          _In_ PDEVICE_OBJECT PhysicalDeviceObject) {
  NTSTATUS status;
  PDEVICE_OBJECT deviceObject;
  PDEVICE_EXTENSION deviceExtension;

  DbgPrint("WindowsAiMic: AddDevice\n");

  // Create the device object
  status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), NULL,
                          FILE_DEVICE_SOUND, FILE_DEVICE_SECURE_OPEN, FALSE,
                          &deviceObject);

  if (!NT_SUCCESS(status)) {
    DbgPrint("WindowsAiMic: IoCreateDevice failed: 0x%08X\n", status);
    return status;
  }

  // Initialize device extension
  deviceExtension = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
  RtlZeroMemory(deviceExtension, sizeof(DEVICE_EXTENSION));

  deviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
  deviceExtension->FunctionalDeviceObject = deviceObject;

  // Attach to device stack
  deviceExtension->NextLowerDriver =
      IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

  if (deviceExtension->NextLowerDriver == NULL) {
    IoDeleteDevice(deviceObject);
    return STATUS_NO_SUCH_DEVICE;
  }

  // Allocate loopback audio buffer
  deviceExtension->AudioBuffer =
      (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, AUDIO_BUFFER_SIZE, 'cimA');

  if (deviceExtension->AudioBuffer == NULL) {
    IoDetachDevice(deviceExtension->NextLowerDriver);
    IoDeleteDevice(deviceObject);
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  RtlZeroMemory(deviceExtension->AudioBuffer, AUDIO_BUFFER_SIZE);
  deviceExtension->BufferSize = AUDIO_BUFFER_SIZE;
  deviceExtension->WritePosition = 0;
  deviceExtension->ReadPosition = 0;
  KeInitializeSpinLock(&deviceExtension->BufferLock);

  // Enable device
  deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
  deviceObject->Flags |= DO_POWER_PAGABLE;

  g_DeviceObject = deviceObject;

  DbgPrint("WindowsAiMic: Device added successfully\n");
  return STATUS_SUCCESS;
}

/**
 * Write audio data to loopback buffer (called from render endpoint)
 */
NTSTATUS
WriteAudioData(_In_ PDEVICE_EXTENSION DeviceExtension, _In_ PUCHAR Data,
               _In_ SIZE_T Length) {
  KIRQL oldIrql;

  KeAcquireSpinLock(&DeviceExtension->BufferLock, &oldIrql);

  // Write to circular buffer
  SIZE_T firstPart =
      min(Length, DeviceExtension->BufferSize - DeviceExtension->WritePosition);
  RtlCopyMemory(DeviceExtension->AudioBuffer + DeviceExtension->WritePosition,
                Data, firstPart);

  if (firstPart < Length) {
    RtlCopyMemory(DeviceExtension->AudioBuffer, Data + firstPart,
                  Length - firstPart);
  }

  DeviceExtension->WritePosition =
      (DeviceExtension->WritePosition + Length) % DeviceExtension->BufferSize;

  KeReleaseSpinLock(&DeviceExtension->BufferLock, oldIrql);

  return STATUS_SUCCESS;
}

/**
 * Read audio data from loopback buffer (called from capture endpoint)
 */
NTSTATUS
ReadAudioData(_In_ PDEVICE_EXTENSION DeviceExtension, _Out_ PUCHAR Data,
              _In_ SIZE_T Length) {
  KIRQL oldIrql;

  KeAcquireSpinLock(&DeviceExtension->BufferLock, &oldIrql);

  // Read from circular buffer
  SIZE_T firstPart =
      min(Length, DeviceExtension->BufferSize - DeviceExtension->ReadPosition);
  RtlCopyMemory(Data,
                DeviceExtension->AudioBuffer + DeviceExtension->ReadPosition,
                firstPart);

  if (firstPart < Length) {
    RtlCopyMemory(Data + firstPart, DeviceExtension->AudioBuffer,
                  Length - firstPart);
  }

  DeviceExtension->ReadPosition =
      (DeviceExtension->ReadPosition + Length) % DeviceExtension->BufferSize;

  KeReleaseSpinLock(&DeviceExtension->BufferLock, oldIrql);

  return STATUS_SUCCESS;
}
