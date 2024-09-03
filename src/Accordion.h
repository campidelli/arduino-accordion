#ifndef ACCORDION_H
#define ACCORDION_H

#include <Oscil.h>
#include <ADSR.h>
#include <tables/saw2048_int8.h>
#include <tables/square_no_alias_2048_int8.h>
#include "Voice.h"

#define MAX_VOICES       10
#define OSCIL_1_TABLE    SAW2048_DATA
#define OSCIL_2_TABLE    SQUARE_NO_ALIAS_2048_DATA
#define ATTACK_LEVEL     96
#define DECAY_LEVEL      64
#define ATTACK_TIME      50
#define DECAY_TIME       200
#define SUSTAIN_DURATION 8000
#define RELEASE_TIME     200

class Accordion {
public:
    Accordion();

    void init();
    void noteOn(byte note);
    void noteOff(byte note);
    void update();
    int nextSample();
    bool isPlaying();

private:
    Voice* voices[MAX_VOICES];
    int findFreeVoice();
};

#endif // ACCORDION_H
