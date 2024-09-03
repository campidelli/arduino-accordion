#include "Accordion.h"

Accordion::Accordion() {
    for (int i = 0; i < MAX_VOICES; i++) {
        voices[i] = nullptr;
    }
}

void Accordion::init() {
    for (int i = 0; i < MAX_VOICES; i++) {
        Oscil<NUM_TABLE_CELLS, MOZZI_AUDIO_RATE>* osc1 = new Oscil<NUM_TABLE_CELLS, MOZZI_AUDIO_RATE>(OSCIL_1_TABLE);
        Oscil<NUM_TABLE_CELLS, MOZZI_AUDIO_RATE>* osc2 = new Oscil<NUM_TABLE_CELLS, MOZZI_AUDIO_RATE>(OSCIL_2_TABLE);
        ADSR<MOZZI_CONTROL_RATE, MOZZI_AUDIO_RATE>* envelope = new ADSR<MOZZI_CONTROL_RATE, MOZZI_AUDIO_RATE>();

        envelope->setAttackLevel(ATTACK_LEVEL);
        envelope->setDecayLevel(DECAY_LEVEL);
        envelope->setSustainLevel(DECAY_LEVEL);
        envelope->setAttackTime(ATTACK_TIME);
        envelope->setDecayTime(DECAY_TIME);
        envelope->setSustainTime(SUSTAIN_DURATION);
        envelope->setReleaseTime(RELEASE_TIME);

        voices[i] = new Voice(*osc1, *osc2, *envelope);
    }
}

void Accordion::noteOn(byte note) {
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voices[i]->isPlaying() && voices[i]->getNote() == note) {
            return; // This note is already being played, ignore it
        }
    }
    int freeVoice = findFreeVoice();
    if (freeVoice != -1) {
        voices[freeVoice]->play(note);
    }
}

void Accordion::noteOff(byte note) {
    for (int i = 0; i < MAX_VOICES; i++) {
        if (voices[i]->getNote() == note) {
            voices[i]->stop();
            break;
        }
    }
}

void Accordion::update() {
    for (int i = 0; i < MAX_VOICES; i++) {
        voices[i]->update();
    }
}

int Accordion::nextSample() {
    int mix = 0;
    for (int i = 0; i < MAX_VOICES; i++) {
        mix += voices[i]->nextSample();
    }
    return mix;
}

bool Accordion::isPlaying() {
    for (int i = 0; i < MAX_VOICES; i++) {
        if (voices[i]->isPlaying()) {
            return true;
        }
    }
    return false;
}

int Accordion::findFreeVoice() {
    int voiceIndex = -1;
    long oldestTriggeredAt = millis();
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!voices[i]->isPlaying()) {
            return i;
        } else if (voices[i]->getTriggeredAt() < oldestTriggeredAt) {
            oldestTriggeredAt = voices[i]->getTriggeredAt();
            voiceIndex = i;
        }
    }
    return voiceIndex;
}
