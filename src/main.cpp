#include "config.h"

#include <Arduino.h>
#include <ml_epiano.h>
#include "status_module.h"
#include "Keyboard.h"

#define ML_SYNTH_INLINE_DECLARATION
#include <ml_inline.h>
#undef ML_SYNTH_INLINE_DECLARATION

ML_EPiano myRhodes;
ML_EPiano *rhodes = &myRhodes;

char shortName[] = "ML_Piano";
float mainVolume = 0.5f;

Keyboard keyboard;

// Core 0: This is used to add a task to core 0 */
TaskHandle_t Core0TaskHnd;

// init your stuff for core0 here
void Core0TaskSetup() {
    Status_Setup();
}

// put your loop stuff for core0 here
void Core0TaskLoop() {
    Status_Process();
}

void Core0Task(void *parameter) {
    Core0TaskSetup();
    while (true) {
        Core0TaskLoop();
        // this seems necessary to trigger the watchdog
        delay(1);
        yield();
    }
}

inline void Core0TaskInit() {
    // we need a second task for the terminal output
    xTaskCreatePinnedToCore(Core0Task, "CoreTask0", 8000, NULL, 999, &Core0TaskHnd, 0);
}

byte getNote(int key) {
  return key + 60; // Key 0 == C4
}

void noteOn(int key) {
  rhodes->NoteOn(0, getNote(key), 127);
}

void noteOff(int key) {
  rhodes->NoteOff(0, getNote(key));
}

void setup() {
    Serial.begin(SERIAL_BAUDRATE);

    // Setup the keyboard
    keyboard.begin();
    keyboard.onKeyPress(noteOn);
    keyboard.onKeyRelease(noteOff);

    Serial.printf("Loading data\n");
    Serial.printf("Firmware started successfully\n");

    Serial.printf("Initialize Audio Interface\n");
    Audio_Setup();

    Serial.printf("ESP.getFreeHeap() %d\n", ESP.getFreeHeap());
    Serial.printf("ESP.getMinFreeHeap() %d\n", ESP.getMinFreeHeap());
    Serial.printf("ESP.getHeapSize() %d\n", ESP.getHeapSize());
    Serial.printf("ESP.getMaxAllocHeap() %d\n", ESP.getMaxAllocHeap());
    /* PSRAM will be fully used by the looper */
    Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());

    Serial.printf("Firmware started successfully\n");
}

void loop() {
    static int loop_cnt_1hz = SAMPLE_BUFFER_SIZE;
    if (loop_cnt_1hz >= SAMPLE_RATE) {
        loop_cnt_1hz -= SAMPLE_RATE;
    }
    Status_Process();

    // Update the keyboard
    keyboard.update();

    // And finally the audio stuff
    float mono[SAMPLE_BUFFER_SIZE], left[SAMPLE_BUFFER_SIZE], right[SAMPLE_BUFFER_SIZE];
    memset(left, 0, sizeof(left));
    memset(right, 0, sizeof(right));

    rhodes->Process(mono, SAMPLE_BUFFER_SIZE);
    // reduce gain to avoid clipping
    for (int n = 0; n < SAMPLE_BUFFER_SIZE; n++) {
        mono[n] *= mainVolume;
    }
    // mono to left and right channel
    for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
        left[i] += mono[i];
        right[i] += mono[i];
    }

    // Output the audio
    Audio_Output(left, right);

/*

    Do I need this? I am sending audio to my I2S DAC

    int32_t mono[SAMPLE_BUFFER_SIZE];
    Organ_Process_Buf(mono, SAMPLE_BUFFER_SIZE);
    Audio_OutputMono(mono);
*/

    Status_Process_Sample(SAMPLE_BUFFER_SIZE);
}
