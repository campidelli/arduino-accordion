#include "MozziConfigValues.h"
#define MOZZI_AUDIO_MODE   MOZZI_OUTPUT_I2S_DAC
#define MOZZI_I2S_PIN_BCK  26
#define MOZZI_I2S_PIN_WS   25
#define MOZZI_I2S_PIN_DATA 22
#define MOZZI_CONTROL_RATE 256

#include <Arduino.h>
#include <WiFi.h>
#include <Mozzi.h>
#include <Oscil.h>
#include <mozzi_midi.h>
#include <ADSR.h>
#include <tables/saw512_int8.h>
#include <tables/triangle512_int8.h>
#include <tables/sin512_int8.h>
#include <tables/square_no_alias512_int8.h>
#include "Esp32SynchronizationContext.h"
#include "Keyboard.h"  // Include the new Keyboard class

// General config
#define LED_PIN          2
#define MAX_VOICES      10
#define OSC_TABLE_SIZE 512

// Envelope parameters
unsigned int attackTime      = 22;
unsigned int decayTime       = 500;
unsigned int sustainDuration = 8000; // Max sustain duration, not sustain level
unsigned int releaseTime     = 300;
byte attackLevel             = 96;
byte decayLevel              = 64;

// Voice structure
struct Voice {
    Oscil<OSC_TABLE_SIZE, AUDIO_RATE> oscillator;
    ADSR<CONTROL_RATE, AUDIO_RATE> envelope;
    byte noteNumber = 0;
    byte velocity = 0;
};
Voice voices[MAX_VOICES];

// Thread-safe synchronization context
Esp32SynchronizationContext syncContext;
bool updateRequested = false;

// Create a Keyboard object
Keyboard keyboard;

void processNoteOn(byte channel, byte note, byte velocity) {
    if (velocity > 0) {
        int activeVoices = 0;
        int voiceToReplace = 0;
        int lowestVelocity = 128;

        for (unsigned int i = 0; i < MAX_VOICES; i++) {
            if (!voices[i].envelope.playing()) {
                voices[i].envelope.noteOff();
                voices[i].oscillator.setFreq(mtof(float(note)));
                voices[i].envelope.noteOn();
                voices[i].noteNumber = note;
                voices[i].velocity = velocity;
                break;
            } else {
                activeVoices++;
                if (lowestVelocity >= voices[i].velocity) {
                    lowestVelocity = voices[i].velocity;
                    voiceToReplace = i;
                }
            }
        }

        if (activeVoices == MAX_VOICES) {
            voices[voiceToReplace].envelope.noteOff();
            voices[voiceToReplace].oscillator.setFreq(mtof(float(note)));
            voices[voiceToReplace].envelope.noteOn();
            voices[voiceToReplace].noteNumber = note;
            voices[voiceToReplace].velocity = velocity;
        }
        digitalWrite(LED_PIN, HIGH);
    }
}

void processNoteOff(byte channel, byte note, byte velocity) {
    byte noActiveNotes = 0;
    for (unsigned int i = 0; i < MAX_VOICES; i++) {
        if (note == voices[i].noteNumber) {
            voices[i].envelope.noteOff();
            voices[i].noteNumber = 0;
            voices[i].velocity = 0;
        }
        noActiveNotes += voices[i].noteNumber;
    }

    if (noActiveNotes == 0) {
        digitalWrite(LED_PIN, LOW);
    }
}

// Callback functions for handling key press and release events
void onKeyPress(int key) {
    byte note = key + 60;  // Adjust the key to match your desired note range
    Serial.printf("Note %d ON\n", note);
    processNoteOn(0, note, 127);
}

void onKeyRelease(int key) {
    byte note = key + 60;  // Adjust the key to match your desired note range
    Serial.printf("Note %d OFF\n", note);
    processNoteOff(0, note, 0);
}

void updateControl() {
    if (!syncContext.update()) {
        Serial.println("Could not update synchronization context");
    }

    if (updateRequested) {
        keyboard.update();  // Use the keyboard object to update key states
        updateRequested = false;
    }

    // Update envelope for each voice
    for (unsigned int i = 0; i < MAX_VOICES; i++) {
        voices[i].envelope.update();
    }
}

void setOscillatorWaveform(const int8_t *waveformData) {
    // RUNS ON MAIN CORE
    // Set the waveforms for the oscillators
    for (unsigned int i = 0; i < MAX_VOICES; i++) {
        voices[i].oscillator.setTable(waveformData);
    }
}

void configureADSR() {
    // Use values from the global variables to set the ADSR
    for (unsigned int i = 0; i < MAX_VOICES; i++) {
        voices[i].envelope.setTimes(attackTime, decayTime, sustainDuration, releaseTime);
        voices[i].envelope.setADLevels(attackLevel, decayLevel);
    }
}

void handleConfiguration() {
    Serial.print("Octave set to ");
    Serial.println(1);  // Example value

    setOscillatorWaveform(SIN512_DATA);
    Serial.println("Waveform set to sine");
    setOscillatorWaveform(SQUARE_NO_ALIAS512_DATA);
    Serial.println("Waveform set to square");
    setOscillatorWaveform(SAW512_DATA);
    Serial.println("Waveform set to saw");
    setOscillatorWaveform(TRIANGLE512_DATA);
    Serial.println("Waveform set to triangle");

    Serial.print("Attack time set to ");
    Serial.println(attackTime);
    configureADSR();

    Serial.print("Decay time set to ");
    Serial.println(decayTime);
    configureADSR();

    Serial.print("Sustain duration set to ");
    Serial.println(sustainDuration);
    configureADSR();

    Serial.print("Release time set to ");
    Serial.println(releaseTime);
    configureADSR();

    // Blink the LED to indicate configuration
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
}

AudioOutput updateAudio() {
    long outputSample = 0;
    // Accumulate sample values from all voices
    for (int i = 0; i < MAX_VOICES; i++) {
        outputSample += voices[i].oscillator.next() * voices[i].envelope.next();
    }
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
    keyboard.onKeyPress(onKeyPress);
    keyboard.onKeyRelease(onKeyRelease);

    // Initialize the voices
    for (unsigned int i = 0; i < MAX_VOICES; i++) {
        voices[i].envelope.setADLevels(attackLevel, decayLevel);
        voices[i].envelope.setTimes(attackTime, decayTime, sustainDuration, releaseTime);
        voices[i].oscillator.setTable(SAW512_DATA);
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
