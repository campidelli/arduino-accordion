#include "MozziConfigValues.h"
#define MOZZI_AUDIO_MODE   MOZZI_OUTPUT_I2S_DAC
#define MOZZI_I2S_PIN_BCK  26
#define MOZZI_I2S_PIN_WS   25
#define MOZZI_I2S_PIN_DATA 22
#define MOZZI_CONTROL_RATE 2048

#include <Arduino.h>
#include <WiFi.h>
#include <Mozzi.h>
#include <Oscil.h>
#include <ADSR.h>
#include <mozzi_midi.h>
#include <tables/saw2048_int8.h>
#include <tables/square_no_alias_2048_int8.h>
#include "Esp32SynchronizationContext.h"
#include "Keyboard.h"  // Include the new Keyboard class

// General config
#define LED_PIN           2
#define MAX_VOICES        10
#define SAMPLE_RATE       SAW2048_NUM_CELLS

// Envelope parameters
unsigned int attackTime      = 50;
unsigned int decayTime       = 200;
unsigned int sustainDuration = 8000; // Max sustain duration, not sustain level
unsigned int releaseTime     = 200;
byte attackLevel             = 96;
byte decayLevel              = 64;

// Voice structure
struct Voice {
    Oscil<SAMPLE_RATE, AUDIO_RATE> osc1;
    Oscil<SAMPLE_RATE, AUDIO_RATE> osc2;
    ADSR<CONTROL_RATE, AUDIO_RATE> envelope;
    byte note;
    long triggeredAt;
};
Voice voices[MAX_VOICES];

// Thread-safe synchronization context
Esp32SynchronizationContext syncContext;
bool updateRequested = false;

// Create a Keyboard object
Keyboard keyboard;

// Finds a free voice. It can be either a voice not in use
// or the oldest one if all of them are being used
int getFreeVoice() {
  int voiceIndex = -1;
  long oldestTriggeredAt = millis();
  for (int i = 0; i < MAX_VOICES; i++) {
    if (!voices[i].envelope.playing()) {
      return i; 
    } else if (voices[i].triggeredAt < oldestTriggeredAt) {
      oldestTriggeredAt = voices[i].triggeredAt;
      voiceIndex = i;
    }
  }
  return voiceIndex;
}

void noteOn(byte note) {
  for (int i = 0; i < MAX_VOICES; i++) {
    if (voices[i].envelope.playing() && voices[i].note == note) {
      // This note is already being played, ignore
      return;
    }
  }
  int freeVoice = getFreeVoice();
  float frequency = mtof(float(note));
  voices[freeVoice].osc1.setFreq(frequency);

  float detuneFactor = pow(2.0, 10.0 / 1200.0);
  voices[freeVoice].osc2.setFreq(frequency * detuneFactor * 2);  // 10 cents detuned + 1 octave up
  
  voices[freeVoice].envelope.noteOn();
  voices[freeVoice].note = note;
  voices[freeVoice].triggeredAt = millis();
    
  digitalWrite(LED_PIN, HIGH);
}

void noteOff(byte note) {
    int activeNotes = 0;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (note == voices[i].note) {
            voices[i].note = 0;
            voices[i].envelope.noteOff();
        }
        activeNotes += voices[i].note;
    }
    if (activeNotes == 0) {
        digitalWrite(LED_PIN, LOW);
    }
}

// Callback functions for handling key press and release events
void onKeyPress(int key) {
    byte note = key + 60;  // Adjust the key to match your desired note range
    noteOn(note);
}

void onKeyRelease(int key) {
    byte note = key + 60;  // Adjust the key to match your desired note range
    noteOff(note);
}

void updateControl() {
    if (!syncContext.update()) {
        Serial.println("Could not update synchronization context");
    }

    if (updateRequested) {
        keyboard.update();  // Use the keyboard object to update key states
        updateRequested = false;
    }

    // Update the envelopes
    for (int i = 0; i < MAX_VOICES; i++) {
      voices[i].envelope.update();
    }
}

AudioOutput updateAudio() {
    long outputSample = 0;

    // Accumulate sample values from all playing voices
    for (int i = 0; i < MAX_VOICES; i++) {
        if (voices[i].envelope.playing()) {
            outputSample += (voices[i].osc1.next() + voices[i].osc2.next()) * voices[i].envelope.next();
        }
    }

    float volume = 1.05f;
    outputSample *= volume;

    return MonoOutput::fromNBit(24, outputSample);
}

void updateKeyboardTask(void *state) {
    // RUNS ON OTHER CORE
    while (true) {
        if (updateRequested) {
            delay(1); // Feed watchdog
            continue; // Don't do anything if the main thread is still processing the last update
        }
        // Request the main thread to update keyboard states
        syncContext.send(
            [](void *state) {
                // RUNS ON MAIN CORE
                updateRequested = true;
            }
        );
        delay(10); // Feed watchdog
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_OFF); // Disable WiFi to conserve power for audio and touch updates

    // LED debug
    pinMode(LED_PIN, OUTPUT);

    // Initialize the Keyboard object and set the callbacks
    keyboard.init();
    keyboard.onKeyPress(onKeyPress);
    keyboard.onKeyRelease(onKeyRelease);

    // Initialize the voices
    for (unsigned int i = 0; i < MAX_VOICES; i++) {
        voices[i].osc1.setTable(SAW2048_DATA);
        voices[i].osc2.setTable(SQUARE_NO_ALIAS_2048_DATA);
        voices[i].envelope.setADLevels(attackLevel, decayLevel);
        voices[i].envelope.setTimes(attackTime, decayTime, sustainDuration, releaseTime);
    }

    if (!syncContext.begin()) {
        Serial.println("Error initializing synchronization context");
        while (true) {
            ; // Halt
        }
    }
    // Create a task on the first core to asynchronously update touch states
    xTaskCreatePinnedToCore(
        updateKeyboardTask,  // Function that should be called
        "Keyboard Updater",  // Name of the task (for debugging)
        1000,               // Stack size (bytes)
        NULL,               // Parameter to pass
        1,                  // Task priority
        NULL,               // Task handle
        0                   // Core
    );

    startMozzi(CONTROL_RATE);
}

void loop() {
    audioHook();
}
