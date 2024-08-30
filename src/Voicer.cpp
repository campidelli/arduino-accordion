#include "Voicer.h"

Voicer::Voicer() {
    // Initialize voiceOccupied to false by default
    for (int i = 0; i < MAX_VOICES; ++i) {
        voiceOccupied[i] = false;
    }
}

Voicer::~Voicer() {
    // Clean up dynamically allocated voices
    for (int i = 0; i < MAX_VOICES; ++i) {
        delete voices[i];
    }
}

int Voicer::getFreeVoiceIndex() {
    // Find an available voice index
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (!voiceOccupied[i]) {
            return i; // Return index of the first available voice
        }
    }

    // All voices are occupied, steal the oldest voice (index 0)
    delete voices[0]; // Deallocate the oldest voice
    for (int i = 1; i < MAX_VOICES; ++i) {
        voices[i - 1] = voices[i]; // Shift voices left
        voiceOccupied[i - 1] = voiceOccupied[i]; // Update occupancy status
    }
    voices[MAX_VOICES - 1] = nullptr; // Mark the last position as available
    voiceOccupied[MAX_VOICES - 1] = false; // Mark the last position as available
    
    return MAX_VOICES - 1; // Return the index of the newly available slot
}

void Voicer::noteOn(byte note) {
    int index = getFreeVoiceIndex();
    // Allocate a new Voice and mark it as occupied
    voices[index] = new Voice(note);
    voices[index]->noteOn();
    voiceOccupied[index] = true;
}

void Voicer::noteOff(byte note) {
    // Release the note from the appropriate voice
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voiceOccupied[i] && voices[i]->getNote() == note) {
            // Release the voice or set it to an idle state
            voices[i]->noteOff();
            delete voices[i]; // Deallocate the Voice
            voices[i] = nullptr;
            voiceOccupied[i] = false; // Mark voice as available
            break;
        }
    }
}

void Voicer::update() {
    // Update all active voices
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voiceOccupied[i]) {
            voices[i]->update(); // Update the Voice
        }
    }
}

long Voicer::getSample() {
    // Accumulate all notes samples
    long accumulatedSamples = 0;
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voiceOccupied[i]) {
            accumulatedSamples += voices[i]->getSample();
        }
    }
    return accumulatedSamples;
}

bool Voicer::isPlaying() {
    // Update all active voices
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voiceOccupied[i] && voices[i]->isPlaying()) {
            return true;
        }
    }
    return false;
}
