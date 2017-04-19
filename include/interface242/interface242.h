#ifndef _TIT_KW242_INTERFACE242_H
#define _TIT_KW242_INTERFACE242_H

typedef struct RESULT_STRUCT
{
    float version;              
#ifdef  WIN32
    __int64 ID;
#else
    unsigned long long ID;               
#endif
    /// LID specific
    bool  isLangResult;         
    char  language[64];
    int	  langID;
    int   langHarmLevel;//语种检测结果的敏感程度，与匹配上的语种ID对应的langHarmLevel值相同        
    int   langConfidence; 
    char  langDatasavpath[256]; 
    char  languageFlag;//模式选择：封堵或者监测  未定义，与中心联系

    bool  isSpeakerResult;          
    char  speaker[64];              
    int	  speakerID;                
    int   speakerHarmLevel;//说话人检测结果的敏感程度，与匹配上的说话人ID对应的speakerHarmLevel值相同
    int   speakerConfidence;        
    char  speakerDatasavpath[256];  
    char  speakerFlag;//模式选择：封堵或者监测 其中，封堵为0x12，监测为0x92

    char  backup[100];              
} RESULT_STRUCT;


typedef void (*CLIENTCALLBACKPROC)(const RESULT_STRUCT &pResults);


extern "C"
{
#define KWEXPORT
    
KWEXPORT int Ioacas_Initialize(const  unsigned long resultFuncAddr, const short task);

    KWEXPORT void Ioacas_API(
        unsigned long long ID,
        const char* LDataBuf,
        unsigned int LDataLen,
        const char* RDataBuf,
        unsigned int RDataLen
    );

    KWEXPORT int Ioacas_ShutDown(void);

    KWEXPORT int AddSpeaker(unsigned int speaker_id,
                            const char* spkmodel,
                            int model_size,
                            char service,
                            int speakerHarmLevel
                           );

    KWEXPORT int RemoveAllSpeaker();
    
    KWEXPORT int RemoveSpeaker(unsigned int speaker_id);

}


#endif
