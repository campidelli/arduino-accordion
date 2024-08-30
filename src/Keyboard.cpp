#include "Keyboard.h"

Keyboard::Keyboard() {
    // Initialize row pins as inputs with pull-up resistors
    for (int i = 0; i < ROWS; i++) {
        pinMode(rowPins[i], INPUT_PULLUP);
    }
    // Initialize control pins for the 74HC595
    pinMode(DATA_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    
    // Initialize callbacks to nullptr
    keyPressCallback = nullptr;
    keyReleaseCallback = nullptr;
}

void Keyboard::setColumn(int col) {
    uint8_t colValue = ~(1 << col);  // Set only the selected column LOW
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, colValue);

    // Latch to transfer the data to output pins
    digitalWrite(LATCH_PIN, LOW);
    digitalWrite(LATCH_PIN, HIGH);

    // Small delay to allow the shift register output to stabilize
    delayMicroseconds(10);
}

void Keyboard::update() {
    for (int col = 0; col < COLS; col++) {
        setColumn(col);
        for (int row = 0; row < ROWS; row++) {
            int currentState = digitalRead(rowPins[row]);
            int button = row * COLS + col + 1;

            if (currentState == LOW && !buttonStates[row][col]) {
                buttonStates[row][col] = true;
                if (keyPressCallback) {
                    keyPressCallback(button);
                }
            } else if (currentState == HIGH && buttonStates[row][col]) {
                buttonStates[row][col] = false;
                if (keyReleaseCallback) {
                    keyReleaseCallback(button);
                }
            }
        }
    }
}

void Keyboard::onKeyPress(void (*callback)(int key)) {
    keyPressCallback = callback;
}

void Keyboard::onKeyRelease(void (*callback)(int key)) {
    keyReleaseCallback = callback;
}

int Keyboard::getTotalKeys() {
    return ROWS * COLS;
}
