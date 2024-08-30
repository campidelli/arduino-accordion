#ifndef VOICER_H
#define VOICER_H

#include <Arduino.h>
#include "Voice.h"

const int MAX_VOICES = 10;

class Voicer {
public:
    Voicer();
    ~Voicer();

    void noteOn(byte note);
    void noteOff(byte note);
    void update();
    long getSample();
    bool isPlaying();

private:
    Voice* voices[MAX_VOICES] = {nullptr};
    bool voiceOccupied[MAX_VOICES] = {false};

    int getFreeVoiceIndex();
    void shiftVoicesLeft(int startIndex);
};

#endif // VOICER_H
