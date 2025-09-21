
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "SynthEngine.h"
#include "UiManager.h"
#include "esp_log.h"


// The main application's entry point.
extern "C" void app_main() {
  // A FreeRTOS queue to safely pass messages (e.g., MIDI events) between tasks.
  QueueHandle_t event_queue = xQueueCreate(10, sizeof(SynthEvent));

  // Global objects and handles.
  // Use pointers if allocation is too large for the main task stack.
  SynthEngine* synth = new SynthEngine(event_queue);
  UiManager* ui = new UiManager(event_queue);

  // Create and pin the high-priority audio task to Core 0.
  // This task should have minimal stack and never yield unnecessarily.
  xTaskCreatePinnedToCore(
      synth_audio_task,      // Task function
      "AudioTask",           // Task name
      10000,                 // Stack size (bytes)
      synth,                 // Parameter to pass
      24,                    // Task priority (max is 24 on ESP32)
      NULL,                  // Task handle
      0                      // Core to run on (Core 0)
  );

  // Create and pin the lower-priority control/UI task to Core 1.
  xTaskCreatePinnedToCore(
      control_ui_task,       // Task function
      "ControlUITask",       // Task name
      10000,                 // Stack size (bytes)
      ui,                    // Parameter to pass
      1,                     // Task priority (lower than audio)
      NULL,                  // Task handle
      1                      // Core to run on (Core 1)
  );
}
