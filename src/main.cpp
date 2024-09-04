#define TSF_IMPLEMENTATION

#define I2S_BCK_IO      26
#define I2S_WS_IO       25
#define I2S_DATA_IO     22
#define SAMPLE_RATE     44100
#define I2S_BUF_SIZE    1024
#define VOLUME_GAIN_DB  -20.0f
#define SOUND_FILE_NAME "/tiredAccordion.sf2"

#include <Arduino.h>
#include <driver/i2s.h>
#include <SPIFFS.h>
#include "libtinysoundfont/tsf.h"
#include "Esp32SynchronizationContext.h"
#include "Keyboard.h"

static tsf* g_TinySoundFont;
static Keyboard keyboard;

// Thread-safe synchronization context
Esp32SynchronizationContext syncContext;
bool updateKeyboard = false;

int getNote(int key) {
    return key + 60;
}

void noteOn(int key) {
    int note = getNote(key);
    Serial.printf("Note %d ON\n", note);
    tsf_note_on(g_TinySoundFont, 0, note, 1.0f);
}

void noteOff(int key) {
    int note = getNote(key);
    Serial.printf("Note %d OFF\n", note);
    tsf_note_off(g_TinySoundFont, 0, note);
}

void initI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = I2S_BUF_SIZE
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_IO,
        .ws_io_num = I2S_WS_IO,
        .data_out_num = I2S_DATA_IO,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

void updateKeyboardTask(void *state) {
    // RUNS ON OTHER CORE
    while (true) {
        if (updateKeyboard) {
            delay(1); // Feed watchdog
            continue; // Don't do anything if the main thread is still processing the last update
        }
        // Request the main thread to update keyboard states
        syncContext.send(
            [](void *state) {
                // RUNS ON MAIN CORE
                updateKeyboard = true;
            }
        );
        delay(10); // Feed watchdog
    }
}

void keyboardTask(void *pvParameters) {
    while (true) {
        keyboard.update();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);

    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS.");
        while (1);
    }

    File sfFile = SPIFFS.open(SOUND_FILE_NAME, "r");
    if (!sfFile) {
        Serial.println("Failed to open SoundFont file.");
        while (1);
    }

    // Read the SoundFont file into a buffer
    size_t fileSize = sfFile.size();
    uint8_t* fileBuffer = new uint8_t[fileSize];
    sfFile.read(fileBuffer, fileSize);
    sfFile.close();

    // Load the SoundFont from the buffer
    g_TinySoundFont = tsf_load_memory(fileBuffer, fileSize);
    delete[] fileBuffer; // Free the buffer after loading the SoundFont
    if (!g_TinySoundFont) {
        Serial.println("Could not load SoundFont.");
        while (1);
    }

    // Config the audio output
    initI2S();
    tsf_set_output(g_TinySoundFont, TSF_STEREO_INTERLEAVED, SAMPLE_RATE, VOLUME_GAIN_DB);

    // Setup the keyboard and the callbacks
    keyboard.begin();
    keyboard.onKeyPress(noteOn);
    keyboard.onKeyRelease(noteOff);

    // Begin the synchronization context
    if (!syncContext.begin()) {
        Serial.println("Error initializing synchronization context");
        while (true) {
            ; // Halt
        }
    }
    // Create a task on the first core to asynchronously update keyboard states
    xTaskCreatePinnedToCore(
        updateKeyboardTask,  // Function that should be called
        "Keyboard Updater",  // Name of the task (for debugging)
        1000,               // Stack size (bytes)
        NULL,               // Parameter to pass
        1,                  // Task priority
        NULL,               // Task handle
        0                   // Core
    );
}

void loop() {
    // RUNS ON MAIN CORE
    if (!syncContext.update()) {
        Serial.println("Could not update synchronization context");
    }

    if (updateKeyboard) {
        keyboard.update();
        updateKeyboard = false;
    }

    int16_t audioBuffer[I2S_BUF_SIZE];
    tsf_render_short(g_TinySoundFont, audioBuffer, I2S_BUF_SIZE / 2, 0);
    size_t bytesWritten;
    if (i2s_write(I2S_NUM_0, (const char*)audioBuffer, I2S_BUF_SIZE * sizeof(int16_t), &bytesWritten, portMAX_DELAY) != ESP_OK) {
        Serial.println("Failed to write to I2S.");
    }
}
