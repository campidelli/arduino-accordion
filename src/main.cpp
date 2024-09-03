#include <Arduino.h>
#include "Keyboard.h"
#include "AudioTools.h"
#include "StkAll.h"

// Define constants
const float AMPLITUDE = 16.0;
const int GROUP = 0;

// Create instances of the audio components
I2SStream i2s;
ArdStreamOut output(i2s, 1);
Clarinet clarinet(440); // Create a clarinet instance
Voicer voicer;
Keyboard keyboard;

// Define key press and release handlers
int getNote(int key) {
    return key + 60;
}

void noteOn(int key) {
    int note = getNote(key);
    voicer.noteOn(note, AMPLITUDE, GROUP);
}

void noteOff(int key) {
    int note = getNote(key);
    voicer.noteOff(note, AMPLITUDE, GROUP);
}

void setup() {
    Serial.begin(115200);

    // Initialize the keyboard
    keyboard.begin();
    keyboard.onKeyPress(noteOn);
    keyboard.onKeyRelease(noteOff);

    // Add the clarinet to the voicer
    voicer.addInstrument(&clarinet, GROUP);

    // Configure the audio output
    auto cfg = i2s.defaultConfig(TX_MODE);
    cfg.bits_per_sample = 16;
    cfg.sample_rate = Stk::sampleRate();
    cfg.channels = 1;
    cfg.pin_bck = 26;
    cfg.pin_ws = 25;
    cfg.pin_data = 22;
    i2s.begin(cfg);
}

void loop() {
  for (int i = 0; i < 1024; i++) {
    output.tick(voicer.tick());
  }
  keyboard.update();
}
