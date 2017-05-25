#include "TBNR_API.h"

TBNR_ResultCallback g_PutResult;
TBNR_ProcessEventCallback g_SendStatus;

int TBNR_Init(const char * systemDir, char * configFile, int NumberOfSession)
{
    return 0;
}
int TBNR_Exit()
{
    return 0;
}
int TBNR_SetResultCallbackFunc(TBNR_ResultCallback pFunc, int sessionId)
{
    g_PutResult = pFunc;
    return 0;
}
int TBNR_SetEventCallbackFunc(TBNR_ProcessEventCallback pFunc, int sessionId)
{
    g_SendStatus = pFunc;
    return 0;
}
int TBNR_Start(int sessionId)
{
    return 0;
}
int TBNR_Stop(int sessionId)
{
    return 0;
}
int TBNR_SendData(char * buf, int length, int sessionId, SPEECHTYPE type, long timestamp)
{
    return 0;
}
int TBNR_StopRecording(int sessionId)
{
    g_SendStatus(TBNR_EVENT_RECOGNITION_COMPLETE, sessionId);
    return 0;
}
