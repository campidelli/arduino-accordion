#include "Voice.h"
// #include <mozzi_midi.h> // For some reason it doesn't compile if I put this include in the header. Error: multiple definition of `MidiToFreqPrivate::midiToFreq'

Voice::Voice(byte note) : note(note), playing(false), oscillator(SAW512_DATA), envelope() {
    // Set up the oscillator with a default waveform
    oscillator.setTable(SAW512_DATA); // Or any other default waveform
    // Initialize the ADSR envelope
    envelope.setADLevels(96, 64); // Default attack and decay levels
    envelope.setTimes(200, 500, 8000, 300); // Increase attack time
}

void Voice::noteOn() {
    // Set frequency using mtof
    //oscillator.setFreq(mtof(float(note)));
    envelope.noteOn();
    playing = true;
}

void Voice::noteOff() {
    // Stop playing the note
    envelope.noteOff();
    playing = false;
}

void Voice::update() {
    // Update the oscillator and envelope
    if (playing) {
        envelope.update();
    }
}

long Voice::getSample() {
    // Return the sample of this note
    return oscillator.next() * envelope.next();
}

bool Voice::isPlaying() {
    return playing;
}

byte Voice::getNote() {
    return note;
}
