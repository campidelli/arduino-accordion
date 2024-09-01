#include "status_module.h"

// External variables definition
bool triggerTerminalOutput = true; /*!< necessary for usage without vt100 compliant terminal */
char statusMsg[128] = ""; /*!< buffer for top line message */
uint32_t statusMsgShowTimer = 0; /*!< counter for timeout to reset top line */

#define VU_MAX  24 /*!< character length of vu meter */
float statusVuLookup[VU_MAX]; /*!< precalculated lookup */

void Status_Setup(void)
{
    float vuValue = 1.0f;
    for (int i = VU_MAX; i > 0; i--)
    {
        int n = i - 1;
        statusVuLookup[n] = vuValue;
        vuValue *= 1.0f / sqrt(2.0f); /* div by 1/sqrt(2) means 3db */
        vuValue *= 1.0f / sqrt(2.0f);
    }
}

void Status_PrintVu(float *value, uint8_t vuMax)
{
    if (*value >= 0.7071f) /* -3dB */
    {
        Serial.printf("\033[0;31m"); /* red */
    }
    else if (*value >= 0.5) /* -6dB */
    {
        Serial.printf("\033[0;33m"); /* yellow */
    }
    else
    {
        Serial.printf("\033[0;32m"); /* green */
    }

    for (int i = 0; i < vuMax; i++)
    {
        if (statusVuLookup[i + (VU_MAX - vuMax)] <= *value)
        {
            Serial.printf("#");
        }
        else
        {
            Serial.printf(" ");
        }
    }

    Serial.printf("\033[0m"); /* switch back to default */
    *value *= 0.5; /* slowly lower the value */
}

char emptyLine[] = "                                ";

void Status_PrintAll(void)
{
    Serial.printf("--------------------------------\n");
    Serial.printf("%s%s\n", statusMsg, &emptyLine[strlen(statusMsg)]);
}

void Status_Process_Sample(uint32_t inc)
{
    statusMsgShowTimer += inc;
    if (statusMsgShowTimer == SAMPLE_RATE * 3)
    {
        statusMsgShowTimer = SAMPLE_RATE * 3 + 1;
        sprintf(statusMsg, "%s", emptyLine);
        triggerTerminalOutput = true;
    }
}

void Status_Process(void)
{
    if (triggerTerminalOutput)
    {
        Status_PrintAll();
        triggerTerminalOutput = false;
    }
}

void Status_ValueChangedFloat(const char *group, const char *descr, float value)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s - %s: %0.3f", group, descr, value);
    triggerTerminalOutput = true;
}

void Status_ValueChangedFloat(const char *descr, float value)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s: %0.3f", descr, value);
    triggerTerminalOutput = true;
}

void Status_ValueChangedFloatArr(const char *descr, float value, int index)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s[%d]: %0.3f", descr, index, value);
    triggerTerminalOutput = true;
}

void Status_ValueChangedIntArr(const char *descr, int value, int index)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s[%d]: %d", descr, index, value);
    triggerTerminalOutput = true;
}

void Status_ValueChangedInt(const char *group, const char *descr, int value)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s - %s: %d", group, descr, value);
    triggerTerminalOutput = true;
}

void Status_ValueChangedInt(const char *descr, int value)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s: %d", descr, value);
    triggerTerminalOutput = true;
}

void Status_ValueChangedStr(const char *descr, const char *value)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s: %s", descr, value);
    triggerTerminalOutput = true;
}

void Status_TestMsg(const char *text)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s", text);
    triggerTerminalOutput = true;
}

void Status_LogMessage(const char *text)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s", text);
    triggerTerminalOutput = true;
}
