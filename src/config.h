#ifndef CONFIG_H_
#define CONFIG_H_

#define SERIAL_BAUDRATE      115200
#define MIDI_SERIAL_BAUDRATE SERIAL_BAUDRATE

/* use flexible MIDI mapping */
#define MIDI_MAP_FLEX_ENABLED

/*
 * Configuration for
 * Board: "ESP32 Dev Module" or similar
 */
#define LED_PIN     GPIO_NUM_2
#define REVERB_ENABLED /* add simple reverb */
#define ML_CHORUS_ENABLED
#define MIDI_STREAM_PLAYER_ENABLED 

#define SAMPLE_RATE 44100
#define SAMPLE_SIZE_16BIT
#define SAMPLE_BUFFER_SIZE  48

/*
 * I2S Audio In/Out
 */
#define I2S_BCLK_PIN    GPIO_NUM_26
#define I2S_WCLK_PIN    GPIO_NUM_25
#define I2S_DOUT_PIN    GPIO_NUM_22

#endif // CONFIG_H_