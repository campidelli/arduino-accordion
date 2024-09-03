#ifndef VOICE_H
#define VOICE_H

#include <Oscil.h>
#include <ADSR.h>

#define NUM_TABLE_CELLS 2048

class Voice {
public:
    enum OscilNumber {
        OSCIL_1,
        OSCIL_2
    };

    Voice(Oscil<NUM_TABLE_CELLS, MOZZI_AUDIO_RATE>& osc1,
          Oscil<NUM_TABLE_CELLS, MOZZI_AUDIO_RATE>& osc2,
          ADSR<MOZZI_CONTROL_RATE, MOZZI_AUDIO_RATE>& envelope);

    void play(byte note);
    void stop();
    void update();
    bool isPlaying() const;
    byte getNote() const;
    long getTriggeredAt() const;
    int nextSample();
    void detune(OscilNumber oscilNumber, float cents);
    void changeOctave(OscilNumber oscilNumber, int octaveShift);
    String toString() const;

private:
    Oscil<NUM_TABLE_CELLS, MOZZI_AUDIO_RATE>& osc1;
    Oscil<NUM_TABLE_CELLS, MOZZI_AUDIO_RATE>& osc2;
    ADSR<MOZZI_CONTROL_RATE, MOZZI_AUDIO_RATE>& envelope;
    byte note;
    float baseFrequency;
    float detuneAmount[2]; // Detuning for OSCIL_1 and OSCIL_2
    int octaveShift[2];    // Octave shifts for OSCIL_1 and OSCIL_2
    long triggeredAt;
    
    void updateOscillatorFrequency(OscilNumber oscilNumber);
};

#endif // VOICE_H
