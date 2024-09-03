#include "Voice.h"
#include <mozzi_midi.h>

Voice::Voice(Oscil<NUM_TABLE_CELLS, MOZZI_AUDIO_RATE>& osc1,
             Oscil<NUM_TABLE_CELLS, MOZZI_AUDIO_RATE>& osc2,
             ADSR<MOZZI_CONTROL_RATE, MOZZI_AUDIO_RATE>& envelope) : osc1(osc1),
                                                                     osc2(osc2),
                                                                     envelope(envelope),
                                                                     note(0),
                                                                     baseFrequency(0.0f),
                                                                     triggeredAt(0) {

    Serial.printf("MOZZI_CONTROL_RATE: %d, MOZZI_AUDIO_RATE: %d\n", MOZZI_CONTROL_RATE, MOZZI_AUDIO_RATE);

    detuneAmount[OSCIL_1] = 0.0f;
    detuneAmount[OSCIL_2] = 0.0f;
    octaveShift[OSCIL_1] = 0;
    octaveShift[OSCIL_2] = 0;
}

void Voice::play(byte note) {
    this->note = note;
    baseFrequency = mtof(float(note));
    updateOscillatorFrequency(OSCIL_1);
    updateOscillatorFrequency(OSCIL_2);
    envelope.noteOn();
    triggeredAt = millis();
}

void Voice::stop() {
    envelope.noteOff();
    note = 0;
}

bool Voice::isPlaying() const {
    return note != 0 && envelope.playing();
}

byte Voice::getNote() const {
    return note;
}

long Voice::getTriggeredAt() const {
    return triggeredAt;
}

int Voice::nextSample() {
    if (isPlaying()) {
        return (osc1.next() + osc2.next()) * envelope.next();
    }
    return 0;
}

void Voice::update() {
    envelope.update();
}

void Voice::detune(OscilNumber oscilNumber, float cents) {
    detuneAmount[oscilNumber] = cents;
    updateOscillatorFrequency(oscilNumber);
}

void Voice::changeOctave(OscilNumber oscilNumber, int octaveShift) {
    this->octaveShift[oscilNumber] = octaveShift;
    updateOscillatorFrequency(oscilNumber);
}

void Voice::updateOscillatorFrequency(OscilNumber oscilNumber) {
    float frequency = baseFrequency * pow(2.0, octaveShift[oscilNumber]);
    frequency *= pow(2.0, detuneAmount[oscilNumber] / 1200.0);
    
    if (oscilNumber == OSCIL_1) {
        osc1.setFreq(frequency);
    } else if (oscilNumber == OSCIL_2) {
        osc2.setFreq(frequency);
    }
}

String Voice::toString() const {
    String str = "Voice State:\n";
    str += "Note: " + String(note) + "\n";
    str += "Base Frequency: " + String(baseFrequency) + "\n";
    str += "Triggered At: " + String(triggeredAt) + "\n";
    str += "Detune Amount OSCIL_1: " + String(detuneAmount[OSCIL_1]) + "\n";
    str += "Detune Amount OSCIL_2: " + String(detuneAmount[OSCIL_2]) + "\n";
    str += "Octave Shift OSCIL_1: " + String(octaveShift[OSCIL_1]) + "\n";
    str += "Octave Shift OSCIL_2: " + String(octaveShift[OSCIL_2]) + "\n";
    str += "Is Playing: " + String(isPlaying() ? "Yes" : "No");
    return str;
}
