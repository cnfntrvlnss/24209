#include "TBNR_API.h"
#include "TKW_API.h"

TBNR_ResultCallback g_PutResult;
TBNR_ProcessEventCallback g_SendStatus;


int TBNR_SetRecResultCallbackFunc(TBNR_ResultCallback pFunc, int sessionId)
{
    return 0;
}
TKW_ERRCODE TKW_Init( char* sysdir, int nThread,  char* cfgfile)
{
    return TKW_ERRCODE(0);
}
TKW_ERRCODE TKW_Exit()
{
    return TKW_ERRCODE(0);
}
TKW_ERRCODE TKW_Open(TKW_HANDLE *handle, RUNMODE mode)
{
    return TKW_ERRCODE(0);
}
TKW_ERRCODE TKW_Close(TKW_HANDLE handle)
{
    return TKW_ERRCODE(0);
}
TKW_ERRCODE TKW_Start(TKW_HANDLE handle, SESSIONINFOR *pSessionInfor)
{
    return TKW_ERRCODE(0);
}
TKW_ERRCODE TKW_Recognize(TKW_HANDLE handle, SPEECHBUF*SpeechBuf)
{
    return TKW_ERRCODE(0);
}
TKW_ERRCODE TKW_Stop(TKW_HANDLE handle,int bAbort)
{
    return TKW_ERRCODE(0);
}
TKW_ERRCODE TKW_SetKeyWordList(TKW_HANDLE handle,KWLIST *KwList)
{
    return TKW_ERRCODE(0);
}
TKWAPI TKW_ERRCODE TKW_Search(TKW_HANDLE handle, SEARCHINFOR*pSearchInfor ,int  *nResult,SERACHSCOPE scope)
{
    return TKW_ERRCODE(0);
}
TKWAPI TKW_ERRCODE TKW_GetKwResult(TKW_HANDLE handle, KWRESULTLIST *KwResultList,int *nLen,RESULTSORTMODE mode)
{
    return TKW_ERRCODE(0);
}
