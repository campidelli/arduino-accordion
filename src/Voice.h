#ifndef VOICE_H
#define VOICE_H

#include <Arduino.h>
#include <ADSR.h>
#include <Oscil.h>
#include <tables/saw512_int8.h>
#include <Smooth.h>

class Voice {
public:
    // Constructor to initialize the note (velocity removed)
    Voice(byte note);
    
    void noteOn();
    void noteOff();
    void update();
    bool isPlaying();
    byte getNote();
    long getSample();

private:
    Oscil<SAW512_NUM_CELLS, AUDIO_RATE> oscillator;   // Single oscillator
    ADSR<MOZZI_CONTROL_RATE, AUDIO_RATE> envelope; // Single ADSR envelope
    byte note;        // MIDI note number
    bool playing;     // Is this voice currently playing?
};

#endif // VOICE_H
