/*++
                         ThinkIT Keyword Spotting API  
---------------------------------------------------------------------------------
Name:       TKWAPI.h

Function:   ThinkIT	Keyword Spotting API declaration

Author:     JGao


Revision :  2010/06/01
			3.1   Add support to Spoken Document Retrieval (SDR) format search and output
				  Add support to Unicode/ASCII interpretation of the input/output string 
Revision :  2010/04/01
			2.1	  Add support to different sort criterion of search output
  				  Add support for multi-threaded search on pre-loaded index;
				  Add support to pre-load the index for search;
				  Add index management (delete)
				  Add acoustic re-scoring for higher precision;
				  Add support to report the indexing process;
				  Add support to English and Uyghur;
Revision :  1.02  Modify the TKW_Stop fuction to support abort 
            1.01  Add support to kinds of wave format

Copyright:  ThinkIT Speech Lab . All rights reserved.

---------------------------------------------------------------------------------
--*/


/*
						API Quick Prototype Overview
----------------------------------------------------------------------
|  typedef 		void (*TKW_EventCallback)(TKW_HANDLE handle, int eventID, EventInfo *info);
|
|  TKWAPI TKW_ERRCODE TKW_Init( char* sysdir, int nThread,  char* cfgfile);
|  TKW_ERRCODE TKW_Exit();		
|											 
|  TKW_ERRCODE TKW_Open(TKW_HANDLE *handle, RUNMODE mode);
|  TKW_ERRCODE TKW_Close(TKW_HANDLE handle);
|
|
|  TKW_ERRCODE TKW_SetEventCallback(TKW_HANDLE handle,TKW_EventCallback callback);
|  TKW_ERRCODE TKW_Start(TKW_HANDLE handle, SESSIONINFOR *pSessionInfor);
|  TKW_ERRCODE TKW_Stop(TKW_HANDLE handle,int bAbort);
|  TKW_ERRCODE TKW_Recognize(TKW_HANDLE handle, SPEECHBUF*SpeechBuf);
|
|
|	TKW_ERRCODE TKW_PreLoadIndices(TKW_HANDLE handle, char	*szSessionDir, IDXLOADMODE mode,				int nThread);
|	TKW_ERRCODE TKW_PreLoadIndices(TKW_HANDLE handle, SESSIONINFOR *pInfor, int nInfor,IDXLOADMODE mode,	int nThread );
|	TKW_ERRCODE TKW_UnLoadIndices (TKW_HANDLE handle, char	*szSessionDir);
|	TKW_ERRCODE TKW_UnLoadIndices (TKW_HANDLE handle);
|	TKW_ERRCODE TKW_UnLoadIndices (TKW_HANDLE handle, SESSIONINFOR *pInfor, int nInfor);
|	TKW_ERRCODE TKW_DeleteIndices (TKW_HANDLE handle, char	*szSessionDir);
|	TKW_ERRCODE TKW_DeleteIndices (TKW_HANDLE handle, SESSIONINFOR *pInfor,int nInfor);

|	TKW_ERRCODE TKW_DeleteOneIndex(TKW_HANDLE handle, SESSIONINFOR *pInfor);
|	TKW_ERRCODE TKW_MergeMultilpleIndices(TKW_HANDLE handle, SESSIONINFOR *pInforIn, int nInforIn,SESSIONINFOR *pInforOut,int KeepIn)
|
|
|  TKW_ERRCODE TKW_SetParam(TKW_HANDLE handle,char* paraName,char*paraValue);
|  TKW_ERRCODE TKW_GetParam(TKW_HANDLE handle,char* paraName,char*paraValue)
|
|  TKW_ERRCODE TKW_SetKeyWordList(TKW_HANDLE handle,KWLIST *KwList);
|  TKW_ERRCODE TKW_Search(TKW_HANDLE handle, SEARCHINFOR*pSearchInfor ,int  *nResult);
|  TKW_ERRCODE TKW_GetKwResult(TKW_HANDLE handle, KWRESULTLIST *KwResult,int nLen);
|
|  TKW_ERRCODE TKW_CheckStatus(TKW_HANDLE handle,TKW_STATE *TKWState );
|  TKW_ERRCODE TKW_GetErrorCode(char*szErrCode,TKW_ERRCODE errCode);
----------------------------------------------------------------------
*/


#ifndef _TKWAPI_H
#define _TKWAPI_H

#ifndef TKWAPI

#ifdef  WIN32 
#define TKWAPI _declspec(dllexport)
#else
#define TKWAPI
#endif

#endif

#ifdef __cplusplus
extern "C"{
#endif

/*Report the rough position while error occurs*/
typedef enum TKW_ERRCODE
{
 TKW_SUCCESS = 0,
 TKW_SEARCH_ERROR = -100,/*error occurred when searching*/ 
 TKW_SEARCH_ENGINE_BUSY,/*error, search engine is begin*/
 TKW_KLIST_ERROR, /*error occurred when set key word list*/ 
 TKW_GETR_ERROR,  /*error occurred when get the searching result*/
 TKW_OPEN_ERROR,  /*error occurred when open a session*/ 
 TKW_RECOG_ERROR, /*error occurred when recognizing*/ 
 TKW_CLOSE_ERROR, /*error occurred when close recognizing*/ 
 TKW_START_ERROR, /*error occurred when start recognizing*/ 
 TKW_STOP_ERROR,  /*error occurred when stop a session*/ 
 TKW_CHECKS_ERROR,/*error occurred when check the states*/
 TKW_DELETE_IDX_ERROR,/*error occurred when delete the index*/
 TKW_PRELAOD_IDX_ERROR,/*error occurred when try to pre-load the indices*/
 TKW_UNLOAD_IDX_ERROR,
 /*NEW ERROR CODE ADDED*/
 TKW_INIT_ERROR_NOLIC,			/*init error, no license*/
 TKW_INIT_ERROR_WRONG_SYSDIR,	/*init error, wrong system directory*/
 TKW_INIT_ERROR_NO_CFG,			/*init error, no cfg file*/
 TKW_IDXNOTEXIST_ERROR,			/*index not exist*/
}TKW_ERRCODE;

#define TKW_MAX_RESULT_NUM	2147483647    /* maximum (signed) int value */

#ifndef		MAX_PATH
#define		MAX_PATH 260							/*copy from windef.h*/
#endif
#define		TKW_MAX_PATH		(MAX_PATH*2+2)	    /* maximum string,make sure the longest path can be processed for unicode */
#define		TKW_MAX_KW_LEN		256

	  
/*The engine's  state of a instance*/
typedef enum TKW_STATE
{
	TKW_STATE_UNINIT,  /*uninitialized*/
	TKW_STATE_IDLE,    /*idle*/
	TKW_STATE_RECOG,   /*recognizing...*/
	TKW_STATE_STOPPING,/*stopping...*/
	TKW_STATE_ERROR    /*system error*/
}TKW_STATE;


/*
 *	Structure for retrieving the search result ,organized in array form
 */
typedef struct Seginfo
{
	float Segst;
	float Seget;

}Seginfo;// add for seg information [5/11/2011 zhzhang]
/*a key word instance*/
typedef struct  KWRESULT
{ 
	/*Session ID*/
	char szSessionID[TKW_MAX_PATH]; 

	// add serial number here [1/30/2013 Administrator]
	char szSerialNumber[512];
	
	/*Keyword*/
	char  kw[TKW_MAX_KW_LEN];
	
	float fSegStartTime;
	float fSegEndTime;

	/*Position*/
	float fStartTime;             /*Start time shift to start time of the session*/
	float fEndTime;               /*End time shift to start time of the session*/

	
	/*Confidence*/
	float fScore;                 /*Confidence between 0~100*/
	float fLogScore;               /*log Score*/
} KWRESULT;

/*all the key word results*/
typedef struct KWRESULTLIST
{
   KWRESULT *pList;   /*Keyword*/
   int       nLen;    /*Number of result entry */
}KWRESULTLIST;


/*Spoken Document Retrieval(SDR) result
  Result of a spoken document*/
typedef struct SDRRESULT
{
	char szSessionID[TKW_MAX_PATH]; 
	KWRESULT	*pList;
	int			nLen;
	float		fSessionRelevance;
}SDRRESULT;

/*Spoken Document Retrieval(SDR) result
  Result of top K spoken documents*/
typedef struct SDRRESULTLIST
{
	SDRRESULT	* pList;
	int			nLen;
}SDRRESULTLIST;

/*speech types of sample send in.only the 8K_16BIT_PCM is supported now*/
typedef enum TKW_SPEECHTYPE
{
	TKW_8K_16BIT_PCM,
    TKW_8K_8BIT_PCM,
	TKW_8K_8BIT_ALAW,
	TKW_8K_8BIT_MULAW,
	TKW_16K_16BIT_PCM,
}
TKW_SPEECHTYPE;

/*A speech buffer instance*/
typedef struct  
{ 
	char *buf;      /*pointer to speech data*/
	int  nByteLen;  /*data length in bytes */
	TKW_SPEECHTYPE type;/*speech type*/	
} SPEECHBUF;




/*
 *	Key word list organized in array form
 */
//关键词的发音制定方式
//How to specify the pronunciation of the  keyword/query
typedef enum TKW_PRON_MODE
{
	    KWLIST_AUTO_GEN_PRON_MODE=0,
		KWLIST_USER_GEN_PRON_MODE
}TKW_PRON_MODE;

//如何理解这个关键词列表
//仅仅是在语音文档检索模式下有效
typedef enum TKW_QUERY_MODE
{
    	/*KWLIST_SINGLE_QUERY_MODE=0,   //仅仅处理第一个查询
		KWLIST_BOOL_AND_QUERY_MODE,   //将所有的关键词的关系理解为bool AND的关系
		KWLIST_BOOL_OR_QUERY_MODE,    //将所有的关键词的关系理解为bool OR的关系
		KWLIST_BOOL_COMM_QUERY_MODE   //将每个关键词单独理解成为一个单独的BOOL子句(Clause)
		*/
		KWLIST_STD_QUERY_MODE=0,	//关键词检测模式
		KWLIST_SDR_QUERY_MODE		//语音文档检测模式 
}TKW_QUERY_MODE;



//The sort criterion of the spoken document
typedef enum SDRSORTMODE
{
	TKW_SDR_SORT_VSM_TF_IDF=0,		//空间向量模型-词频/倒文档频率模型
	TKW_SDR_SORT_VSM_OKAPI_BM25,	//空间向量模型-Okapi BM25模型
	TKW_SDR_SORT_EXP_TERM_FREQ,		//词频模型
	TKW_SDR_SORT_EXP_TERM_FREQ_MSRA,//MSRA-词频模型变体
	TKW_SDR_SORT_MAX_TERMHIT_CM		//关键词命中置信度最高值
}SDRSORTMODE;
/*A keyword instance*/
typedef struct KWENTRY
{
  char szKeyWordChinese[TKW_MAX_KW_LEN];/*The key word in Chinese character*/
  char szKeyWordPinyin[TKW_MAX_KW_LEN]; /*The key word in pinyin,separated with space*/
  void *KWInfor;             /*The information about the keyword*/
  int   pronmode ;	          /*The key word appeared in Chinese character(0) or syllable(1)*/
#ifdef __cplusplus
  KWENTRY()
  {
	pronmode=KWLIST_AUTO_GEN_PRON_MODE;
  }
#endif
}KWENTRY;

/*
 *	Key word list organized in array form
 *	when used for spoken document retreival
 *  only the first query is accepted.
 */


typedef struct  KWLIST
{   
	KWENTRY *pList;  /*pointer to the  key word array*/
	int      nLen;    /*len of key word */
	int      pronmode ;   /*The key word appeared in Chinese character(0) or syllable(1)*/
	int		 querymode;/* How to use the keyword list*/
	float	 ANDSlope ;/* Interval between keywords when logic and is used*/
	float	 termMinScore;/*term min score*/
	int		 bSDRReqSTDHits;/*Whether need the spoken term detection results when*/
	SDRSORTMODE sdrsort;/*How to sort the spoken document?*/
#ifdef __cplusplus
	KWLIST()
	{
		bSDRReqSTDHits=false;
		nLen=0;
		pList=0;
	}
#endif
} KWLIST;

/*
 * Information used to specify the search range
 * If the endTimeStamp = -1,the whole session
 * while be searched
 */
typedef struct SEARCHINFOR
{
 char szSessionID[TKW_MAX_PATH]; /*Session ID*/
 char szSessionDir[TKW_MAX_PATH];/*the session data root directory,use empty for default*/
 char szSessionWavPath[TKW_MAX_PATH];/*the wave path for the session, used for re-scoring the keyword//2-26-2010, JGao*/
 TKW_SPEECHTYPE	type;		/*the wave format for the session, used for re-scoring the keyword//2-26-2010, JGao*/
 int  startTimeStamp;   /*start time*/
 int  endTimeStamp;     /*end time*/
 bool isRescore;
}SEARCHINFOR;


/*
* Information used to specify the search on memory 
* [2016/4/14 add by hm]
*/
typedef struct SEARCHINFOR_M
{
 char szSessionID[TKW_MAX_PATH]; /*Session ID*/
 char* pIndexBuf;
 int  szIndexBufLen;
 int  startTimeStamp;   /*start time*/
 int  endTimeStamp;     /*end time*/
}SEARCHINFOR_M;



typedef enum RUNMODE
{
  TKW_ONLINEMODE,
  TKW_OFFLINEMODE
}/*The running mode */
RUNMODE;

/* The information about a session */
typedef struct SESSIONINFOR
{
 char			szSessionID [TKW_MAX_PATH]; /*session information*/
 double			fTimeStamp;        /*the start time*/
 char			szSessionDir[TKW_MAX_PATH]; /*the session data directory,use empty for default*/
 char			szSessionWavPath[TKW_MAX_PATH];/*the wave path for the session, used for re-scoring the keyword//2-26-2010, JGao*/
 char			szSerialNumber[512];	   // add serial number [1/30/2013 Administrator]
 TKW_SPEECHTYPE	type;				/*the wave format for the session, used for re-scoring the keyword//2-26-2010, JGao*/
#ifdef __cplusplus
 SESSIONINFOR() 
 {
	 szSessionID[0]='\0';
	 szSerialNumber[0] = '\0';
#endif
 }
}SESSIONINFOR;

/*
*for merge index memory
*
*/
typedef struct SESSIONINFOR_MRG
{
 char			szSessionID [TKW_MAX_PATH]; /*session information*/
 char*			pIndexBuf;
 int            szIndexBufLen;
 char			szSerialNumber[512];	   // add serial number [1/30/2013 Administrator]
#ifdef __cplusplus
 SESSIONINFOR_MRG() 
 {
     szSessionID[0]='\0';
	 szSerialNumber[0] = '\0';
	 szIndexBufLen=0;
 }
#endif
}SESSIONINFOR_MRG;

//index load mode
typedef enum LOADMODE{
TKW_LOAD_IDX_REPLACE,		//unload previous loaded, replace all
TKW_LOAD_IDX_ADD	,		//add
TKW_LOAD_IDX_OVERWRITE		//add the ones not exist,	replace the indices with same name
}IDXLOADMODE;

//The result sort criterion
typedef enum RESULTSORTMODE
{
TKW_RESULT_SORT_SCORE=0,
TKW_RESULT_SORT_SESSIONNAME,
TKW_RESULT_SORT_STARTTIME,
TKW_RESULT_SORT_SESSIONNAME_SCORE,
TKW_RESULT_SORT_SESSIONNAME_STARTTIME,
TKW_RESULT_UNSORT
}RESULTSORTMODE;



/*
*The scope of the search procedure
*/

typedef enum SERACHSCOPE
{
	TKW_SEARCH_ONLINE=1,		/*Search on the online indexes*/
	TKW_SEARCH_CUR_SESSION,		/*Search on current session*/
	TKW_SEARCH_PRELOAD_IDX,		/*Search on pre-loaded session indices*/
}SERACHSCOPE;



typedef enum
{
	TKW_EVENT_INDEX_PROGRESS_REPORT=0,
	TKW_EVENT_PRELOAD_PROGRESS_REPORT=1
}EventType;


typedef struct
{
	//For index progress
	char 	SessionID[TKW_MAX_PATH];		//the session Id
	float	DurationProc;		//the duration of the audio processed for this session, in seconds
	int 	nSegProc;			//the number of segment been produced for this session

	//For pre-load index progress, event will be reported per 100 session
	int		nTotal2Load;		//The index to be loaded 
	int		nTotalLoaded;		//The index been loaded
}EventInfo;


typedef int TKW_HANDLE;

//the event 
typedef 		void (*TKW_EventCallback)(TKW_HANDLE handle, EventType eventID, EventInfo *info, void*hook);

/*
 * Parameters to set
 */
 
 
 


/*************************************************************************
 *
 * TKW_Init()
 *
 * Initialize the TKW environment.
 *
 * Parameters:
 * char* sysdir [in] - The system directory
 * int nThread  [in] - Number of thread open
 * char* cfgfile[in] - Configuration file for the environment
 *
 * Return value: return error coder ,0 success, <0 failure 
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_Init( char* sysdir, int nThread,  char* cfgfile);

/*************************************************************************
 *
 * TKW_Exit()
 *
 * Uninitialized the environment.
 *
 * Parameters: None
 *			
 *
 * Return value: return error coder ,0 success, <0 failure 
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_Exit();





/*************************************************************************
 *
 * TKW_Open()
 *
 * Initialize a instance and return its handle.
 *
 * Parameters:
 * TKW_HANDLE* handle[inout]  - Initialize a instance and get the handle
 *			
 *
 * Return value: 0 success, <0 failure 
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_Open(TKW_HANDLE *handle, RUNMODE mode);
/*************************************************************************
 *
 * TKW_Close()
 *
 * Destroy a instance according to the handle
 *
 * Parameters:
 * TKW_HANDLE handle[in]  - Destroy the instance 
 *			
 *
 * Return value: 0 success, <0 failure 
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_Close(TKW_HANDLE handle);

TKWAPI TKW_ERRCODE TKW_ReleaseIdxMem(TKW_HANDLE handle);

/*************************************************************************
 *
 * TKW_SetParam/TKW_GetParam()
 *
 * Initialize the recognize environment 
 *
 * Parameters:
 * TKW_HANDLE handle[in]  -  Instance handle
 * char* paraName   [in/out]  -  Parameter name to set /get
 * char*paraValue   [in/out]  -  Parameter value to set /get
 * for the detail,please refer to the manual 
 *
 *系统性能调整、常用配置参数列表
 *参数名称                取值范围                默认值          说明
 *sensitivity             0~100                   50              对语音的敏感程度。
 *speedvsaccuracy         0~100                   50              速度与精准性。该值越大，速度越优先；相反则精准性优先
 *recognitiontimeout      0~MAXTIMEOUT            5000            最大识别时间（单位：毫秒）。当一次识别时间超过该值的时候，便会停止识别。MAXTIMEOUT由系统平台决定。
 *incompletetimeout       0~MAXTIMEOUT            40              用户语音后的最大静音时间（单位：毫秒）。当静音时间超过该值时，认为说话结束。MAXTIMEOUT由系统平台决定。
 *其中第三项暂时不提供实现接口，第二项必须在识别停顿期设置，否则无效！			
 *
 * Return value: return error coder ,0 success, <0 failure 
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_SetParam(TKW_HANDLE handle,char* paraName,char*paraValue);
TKWAPI TKW_ERRCODE TKW_GetParam(TKW_HANDLE handle,char* paraName,char*paraValue);



/*************************************************************************
 *
 * TKW_SetEventCallback()
 *
 * Setting the event callback function
 *
 * Parameters:
 * TKW_HANDLE handle[in]  - The instance handle
 * TKW_EventCallback callback[in]- the callback function 
 *			
 *
 * Return value: return error coder ,0 success, <0 failure  
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_SetEventCallback(TKW_HANDLE handle,TKW_EventCallback callback,void *userhook);

/*************************************************************************
 *
 * TKW_Start()
 *
 * Initialize the recognize environment 
 *
 * Parameters:
 * TKW_HANDLE handle[in]  - The instance handle
 * SESSIONINFOR *pSessionInfor[inout]- Session information ,ID,timestamp,etc.
 *                            used to index the result  
 *			
 *
 * Return value: return error coder ,0 success, <0 failure  
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_Start(TKW_HANDLE handle, SESSIONINFOR *pSessionInfor);
/*************************************************************************
 *
 * TKW_Recognize()
 *
 * Recognize a speech
 *
 * Parameters:
 * TKW_HANDLE handle[in]   - Handle of the instance
 * SPEECHBUF* SpeechBuf[in]- Speech data to recognize,include the data and the time information
 * int *TKWtate[inout]     - Indicate if the some keyword have be detected has complished		
 *
 * Return value: return error coder ,0 success, <0 failure 
 *
 *************************************************************************/
TKWAPI  TKW_ERRCODE TKW_Recognize(TKW_HANDLE handle, SPEECHBUF*SpeechBuf);
 
/*************************************************************************
 *
 * TKW_Stop()
 * 
 * Wait until the recognize process accomplish(bAbort = 0)
 * Interrupt the un-completed recognize process(bAbort != 0)
 *
 * Parameters:
 * TKW_HANDLE handle[in]  -  Handle of the instance
 * int bAbort       [in]  -  Whether wait until the recognize stop normally
 *                           bAbort == 0,   stop normally till recognize complete
 *                           bAbort = other,stop immediately and discarded the unprocessed data
 * Return value: return error coder ,0 success, <0 failure 
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_Stop(TKW_HANDLE handle,int bAbort);

TKWAPI TKW_ERRCODE TKW_Stop_Mem(TKW_HANDLE handle,char*& IndexBuf,int& idxBufLen);


//Check if the index of this session exist
TKWAPI TKW_ERRCODE TKW_CheckIndexExist(TKW_HANDLE handle, SEARCHINFOR*pSearchInfor);

TKWAPI TKW_ERRCODE TKW_PreLoadIndicesDir(TKW_HANDLE handle, char *szSessionDir, 
									  IDXLOADMODE mode,	  int nThread);
TKWAPI TKW_ERRCODE TKW_PreLoadIndicesList(TKW_HANDLE handle, 
									  SESSIONINFOR *pInfor, int nInfor,
									  IDXLOADMODE mode,	int nThread );

TKWAPI	TKW_ERRCODE TKW_UnLoadIndicesDir (TKW_HANDLE handle, char	*szSessionDir);
TKWAPI	TKW_ERRCODE TKW_UnLoadIndicesAll (TKW_HANDLE handle);
TKWAPI	TKW_ERRCODE TKW_UnLoadIndicesList(TKW_HANDLE handle, SESSIONINFOR *pInfor, int nInfor);




/*************************************************************************
 *
 * TKW_DeleteOneIndex()
 * 
 * Delete the index file for a specific 
 *
 * Parameters:
 * TKW_HANDLE handle[in]  -  Handle of the instance
 * SESSIONINFOR *pSessionInfor [in]- Session information to specify the session
 *                            		
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_DeleteOneIndex(TKW_HANDLE handle, SESSIONINFOR *pSessionInfor);

/*************************************************************************
 *
 * TKW_DeleteAllIndices()
 * 
 * Delete all the session under a given root dir
 *
 * Parameters:
 * TKW_HANDLE handle[in]  -  Handle of the instance
 * char	szSessionDir[128] -  The root dir of the index files
 * Return value: return error coder ,0 success, <0 failure 
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_DeleteAllIndices(TKW_HANDLE handle, char	*szSessionDir);

 
/*************************************************************************
 *
 * TKW_MergeMultilpleIndices()
 * 
 * Merge the index files of multiple sessions into one for speeding up the search
 *
 * Parameters:
 * TKW_HANDLE handle			[in]	-	Handle of the instance
 * SESSIONINFOR *pInforIn		[in]	-	The list of the sessions to be merged
 * int nInforIn					[in]	-	Number of the sessions to be merged
 * SESSIONINFOR *pInforOut		[in]	-	The session merged into
 * int			KeepIn			[in]	-	Whether keep the original indexes
 *										KeepIn  == 0, after the index merging, keep the original file 
 *										KeepOut == 0, after the index merging, remove the original index file
 * Return value: return error coder ,0 success, <0 failure 
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_MergeMultilpleIndices(TKW_HANDLE handle, SESSIONINFOR *pInforIn, int nInforIn,SESSIONINFOR *pInforOut,int KeepIn);
TKWAPI TKW_ERRCODE TKW_MergeMultilpleIndices_Mem(TKW_HANDLE handle, SESSIONINFOR_MRG *pInforIn, int nInforIn,SESSIONINFOR_MRG *pInforOut);
/*************************************************************************
 *
 * TKW_SetKeyWordList()
 *
 * Reset the key word list dynamically
 *
 * Parameters:
 * TKW_HANDLE handle - Handle of the instance
 * KWLIST *pKwList[in] - Key word list information,used in a array form
 *					          
 *
 * Return value: return error coder ,0 success, <0 failure 
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_SetKeyWordList(TKW_HANDLE handle,KWLIST *KwList);

/*************************************************************************
 *
 * TKW_Search()
 *
 * Do the search and return the results,used on the OFFLINE mode only
 *
 * Parameters:
 * TKW_HANDLE handle[in]       - The instance handle
 * SEARCHINFOR*pSearchInfor[in]- Specify certain session to search in.
 * int  *nResult[inout]        - Number of search result this time
 *								 Its value can also be used to specify the maximum results returned
 * SERACHSCOPE scope[in]	   - Specify the searched scope: online index, this session or preloaded?
 *							           
 * Return value: return error coder ,0 success, <0 failure 
 *
 *************************************************************************/
#ifndef S3
TKWAPI TKW_ERRCODE TKW_Search(TKW_HANDLE handle, SEARCHINFOR*pSearchInfor ,int  *nResult,SERACHSCOPE scope);
TKWAPI TKW_ERRCODE TKW_Search_Mem(TKW_HANDLE handle, SEARCHINFOR_M* pSearchInfor ,int  *nResult,SERACHSCOPE scope);
#else
TKWAPI TKW_ERRCODE TKW_Search(TKW_HANDLE handle, SEARCHINFOR*pSearchInfor ,int  *nResult,SERACHSCOPE scope, bool bAcousticRescore=false);
#endif

/*************************************************************************
 *
 * TKW_GetKwResult()
 *
 * Do the search and return the results,used on the OFFLINE mode only
 *
 * Parameters:
 * TKW_HANDLE handle[in]       - The instance handle
 * SEARCHINFOR*pSearchInfor[in]- Specify certain doc to search in.
 * int  *nResult[inout]        - Number of search result this time
 *							           
 * Return value: return error coder ,0 success, <0 failure 
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_GetKwResult(TKW_HANDLE handle, KWRESULTLIST *KwResultList,int *nLen,RESULTSORTMODE mode);
/*************************************************************************
 *
 * TKW_SearchSpokenDoc()
 *
 * Do the spoken document retrieval
 *
 * Parameters:
 * TKW_HANDLE handle[in]       - The instance handle
 * int  *nDoc[inout]		   - Number of spoken document the user want to got
 *								 Its value can also be used to specify the maximum results returned
 * SDRSORTMODE sortmode[in]	   - The sort criterion of the spoken documents
 *							           
 * Return value: return error coder ,0 success, <0 failure 
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_SearchSpokenDoc(TKW_HANDLE handle, SEARCHINFOR*pSearchInfor,int  *nDoc,int *nDocRelated,SERACHSCOPE scope);


/*************************************************************************
 *
 * TKW_GetSDRResult/TKW_FreeSDRResult()
 *
 * Do the search and return the results,used on the OFFLINE mode only
 *
 * Parameters:
 * TKW_HANDLE handle[in]		 - The instance handle
 * SDRRESULTLIST*ResultList[in]  - The results of retreivaled top K spoken document
 *							           
 * Return value: return error coder ,0 success, <0 failure 
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_GetSDRResult(TKW_HANDLE handle, SDRRESULTLIST *ResultList);
TKWAPI TKW_ERRCODE TKW_FreeSDRResult(TKW_HANDLE handle, SDRRESULTLIST *ResultList);

TKWAPI TKW_ERRCODE TKW_GetSeginfo(TKW_HANDLE handle,SESSIONINFOR &session,Seginfo *segbuf,int N,int &retNum);


/*************************************************************************
 *
 * TKW_CheckStatus()
 *
 * Check the engine recognizor's state
 *
 * Parameters:
 * TKW_HANDLE handle[in]     - The instance handle
 * TKW_STATE *TKWState[inout]- The engine state returned
 *							           
 * Return value: return error coder ,0 success, <0 failure 
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_CheckStatus(TKW_HANDLE handle,TKW_STATE *TKWState );

/*************************************************************************
 *
 * TKW_GetErrorCode()
 *
 * Get the corresponding error reason
 *
 * Parameters:
 * TKW_HANDLE handle [in]   - The instance handle
 * char*szErrCode[inout]    - Reason for error
 *							             
 * TKW_ERRCODE errCode[in]  - the error code
 *                           
 *
 * Return value: return error coder ,0 success, <0 failure 
 *
 *************************************************************************/
TKWAPI TKW_ERRCODE TKW_GetErrorCode(TKW_HANDLE handle,char*szErrCode,TKW_ERRCODE errCode);

#ifdef __cplusplus
}
#endif

#endif


