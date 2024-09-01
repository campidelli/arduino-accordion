#ifndef STATUS_MODULE_H
#define STATUS_MODULE_H

#include <Arduino.h>
#include <esp_debug_helpers.h>
#include "config.h"

// External variables
extern bool triggerTerminalOutput;
extern char statusMsg[128];
extern uint32_t statusMsgShowTimer;
extern float statusVuLookup[];

// Function declarations
void Status_Setup(void);
void Status_PrintVu(float *value, uint8_t vuMax);
void Status_PrintAll(void);
void Status_Process_Sample(uint32_t inc);
void Status_Process(void);
void Status_ValueChangedFloat(const char *group, const char *descr, float value);
void Status_ValueChangedFloat(const char *descr, float value);
void Status_ValueChangedFloatArr(const char *descr, float value, int index);
void Status_ValueChangedIntArr(const char *descr, int value, int index);
void Status_ValueChangedInt(const char *group, const char *descr, int value);
void Status_ValueChangedInt(const char *descr, int value);
void Status_ValueChangedStr(const char *descr, const char *value);
void Status_TestMsg(const char *text);
void Status_LogMessage(const char *text);

#endif // STATUS_MODULE_H
