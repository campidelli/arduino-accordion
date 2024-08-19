#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>
#include "Sound.h"

class Input {
public:
    virtual Sound* read() = 0;
};

#endif // INPUT_H