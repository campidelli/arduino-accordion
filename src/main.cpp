#include "MozziConfigValues.h"
#define MOZZI_AUDIO_MODE   MOZZI_OUTPUT_I2S_DAC
#define MOZZI_I2S_PIN_BCK  26
#define MOZZI_I2S_PIN_WS   25
#define MOZZI_I2S_PIN_DATA 22
#define MOZZI_CONTROL_RATE 128
#define MOZZI_AUDIO_RATE   32768

#include <Arduino.h>
#include <WiFi.h>
#include <Mozzi.h>
#include <ADSR.h>
#include <ResonantFilter.h>
#include <Oscil.h>
#include <tables/saw8192_int8.h>
#include <tables/smoothsquare8192_int8.h>
#include <mozzi_midi.h>
#include "Esp32SynchronizationContext.h"
#include "Keyboard.h"  // Include the new Keyboard class

// General config
#define LED_PIN           2
#define OCTAVE            4
#define MAX_VOICES        10
#define NUM_TABLE_CELLS   8192
#define OSCIL_1_TABLE     SMOOTHSQUARE8192_DATA
#define OSCIL_2_TABLE     SAW8192_DATA


// Envelope parameters
unsigned int attackTime      = 50;
unsigned int decayTime       = 200;
unsigned int sustainDuration = 8000; // Max sustain duration, not sustain level
unsigned int releaseTime     = 200;
byte attackLevel             = 255;
byte decayLevel              = 255;

// General volume
float volumeFactor           = 0.95f;
float detuneFactor           = pow(2.0, 10 / 1200.0); // 10 cents

// Voice structure
bool mixOscillators = true;
struct Voice {
    Oscil<NUM_TABLE_CELLS, MOZZI_AUDIO_RATE> osc1;
    Oscil<NUM_TABLE_CELLS, MOZZI_AUDIO_RATE> osc2;
    ADSR<MOZZI_CONTROL_RATE, MOZZI_AUDIO_RATE> envelope;
    byte note;
    long triggeredAt;
};
Voice voices[MAX_VOICES];

// Low pass filter
bool lowPassFilterOn = false;
ResonantFilter<LOWPASS> lowPassFilter;
byte cutoffFrequencyLevel = 20;
byte resonanceLevel = 200;

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
  voices[freeVoice].osc2.setFreq(frequency * detuneFactor * 2); // +1 octave up
  
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

byte getMidiNote(int key) {
    return (12 * OCTAVE) + 12 + key;
}

void onKeyPress(int key) {
    byte note = getMidiNote(key);
    noteOn(note);
}

void onKeyRelease(int key) {
    byte note = getMidiNote(key);
    noteOff(note);
}

// Only update the LPF values if the difference between the new and previous values is greater than 10%
void updateLPF() {
    int potValue1 = analogRead(36);
    int potValue2 = analogRead(35);
    
    byte newCutoffFrequencyLevel = map(potValue1, 0, 4096, 0, 255);
    byte newResonanceLevel = map(potValue2, 0, 4096, 0, 255);

    if (abs(newCutoffFrequencyLevel - cutoffFrequencyLevel) > (cutoffFrequencyLevel * 0.3)) {
        lowPassFilter.setCutoffFreq(newCutoffFrequencyLevel);
        cutoffFrequencyLevel = newCutoffFrequencyLevel;
        Serial.printf("LPF Cutoff freq: %d\n", newCutoffFrequencyLevel);
    }
    if (abs(newResonanceLevel - resonanceLevel) > (resonanceLevel * 0.3)) {
        lowPassFilter.setResonance(newResonanceLevel);
        resonanceLevel = newResonanceLevel;
        Serial.printf("LPF resonance: %d\n", newResonanceLevel);
    }
}

void updateControl() {
    if (!syncContext.update()) {
        Serial.println("Could not update synchronization context");
    }

    if (updateRequested) {
        keyboard.update();
        //updateLPF();
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
            if (mixOscillators) {
                outputSample += (voices[i].osc1.next() + voices[i].osc2.next()) * voices[i].envelope.next();
            } else {
                outputSample += voices[i].osc1.next() * voices[i].envelope.next();
            }
        }
    }

    // Filter it
    if (lowPassFilterOn) {
        outputSample = lowPassFilter.next(outputSample);
    }

    outputSample *= volumeFactor;

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
        voices[i].osc1.setTable(OSCIL_1_TABLE);
        voices[i].osc2.setTable(OSCIL_2_TABLE);
        voices[i].envelope.setADLevels(attackLevel, decayLevel);
        voices[i].envelope.setTimes(attackTime, decayTime, sustainDuration, releaseTime);
    }

    lowPassFilter.setCutoffFreqAndResonance(cutoffFrequencyLevel, resonanceLevel);

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