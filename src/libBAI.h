#pragma once
#include <stdio.h>

#ifndef LIBBAI_API
#ifdef WIN32
#define LIBBAI_API __declspec(dllexport)
#else
#define LIBBAI_API
#endif
#endif

#define BAI_RETRIEVAL_TYPE_RETIEVAL   0
#define BAI_RETRIEVAL_TYPE_PARTLY     1
#define BAI_RETRIEVAL_TYPE_PARTLY_VAD 2 
#define BAI_RETRIEVAL_TYPE_VAD        3 

#ifdef __cplusplus
extern "C"
{
#endif

	enum BAI_Code
	{
		BAI_OK = 0, 
		BAI_NO_LICENSE = -100, 
		BAI_ENGINE_NOT_INITIALIZED,	// system is not initialized yet
		BAI_SYS_INIT_ERROR,
		BAI_CONFIG_ERROR,
		BAI_ENGIN_OPEN_ERROR,
		BAI_INPUT_NOT_SUPPORTED,	
		BAI_FEATURE_ERROR,
		BAI_INDEX_FAILED,
		BAI_RETRIEVAL_FAILED,
		BAI_FILE_OPEN_FAILED,
		BAI_FILE_LOAD_FAIL,
		BAI_FILE_SAVE_FAIL,
		BAI_EXIT_FAILED,
		BAI_ENGINE_EXIST_ERROR
	};

	struct BAI_InputItem
	{
		int iAudioID;
		char acAudioUrl[1024];
		int iDataType;   	        //(0-pcm, 1-feature)
		char* pcDataBuffer;         //audio data buff  
		int iBufferSize;            //audio data buff size (Bytes).
		LIBBAI_API BAI_InputItem();
		LIBBAI_API ~BAI_InputItem();
	};

	struct BAI_ResultItem
	{
		BAI_Code eErrCode;            // Error code for each result
		int iLibraryID;
        char acAudioUrl[1024];
		float fMatchedRate;           // matched rate, score
		float fTimeStartInTestS;
        float fTimeStartInWaveS;      // start time of the input wave in template audio for AI mode. 
		float fDurationS;             // duration of the result.
		LIBBAI_API BAI_ResultItem();
	};

	struct BAI_ResultList
	{
		BAI_Code eErrCode; 	             // The error code for a single job
		int iTestID;
		int iResultNum;                  // number of the result returned.
		BAI_ResultItem* pstResultItems;  // the result list for one job
		LIBBAI_API BAI_ResultList();
		LIBBAI_API ~BAI_ResultList();
	};

	// Step1. initialize audio identifier engine. 
	LIBBAI_API BAI_Code BAI_Init(const char* kpcConfigFile, const char* kpcLicensePath, const int kiThreadNum);

	// Step2. extract audio DNA from the input PCM data, kiDNAType(0 for index, 1 for retrieval) 
	LIBBAI_API BAI_Code BAI_ExtractDNA(BAI_InputItem* pstInputItems, const int kiInputNum, const int kiDNAType);

	// Step3. open processing thread for retrieval.
	LIBBAI_API BAI_Code BAI_Open(void** ppThreadIndex);

/*****************************************************
Step 4. 
	Method:    BAI_Index
	Purpose:   Index the library audio feature for retrieval.
	Added by:  Zhichao Wang 2016/7/24
******************************************************/
	// Build index, don't write index to file.
	LIBBAI_API BAI_Code BAI_Index(BAI_InputItem* pstInputItems, const int kiInputNum, void** ppThreadIndex);  

	// Save the index to 'kpcHashFile', called after BAI_Index.
	LIBBAI_API BAI_Code BAI_SaveIndex(void** ppThreadIndex, const char* kpcHashFile); 
    
	// Build index, write index to file 'kpcHashIndex'
	LIBBAI_API BAI_Code BAI_BuildIndex(BAI_InputItem* pstInputItems, const int kiInputNum, const char* kpcHashIndex, const int kiThreadNum); 

	// Load index from file, called after BAI_BuildIndex.
	LIBBAI_API BAI_Code BAI_LoadIndex(const char* kpcHashFile, void** ppThreadIndex);    

	// Add new items to existed index file. if kpcDstIndexFile is not NULL, write new index to it, else rewrite to kpcSrcIndexFile.
	LIBBAI_API BAI_Code BAI_AddIndex(BAI_InputItem* pstInputItems, const int kiInputNum, const char* kpcSrcIndexFile, const char* kpcDstIndexFile = NULL); 

/*****************************************************
Step 5. 
	Method:    BAI_Retrieval
	Purpose:   Retrieval the template audio/feature in the index.
	Added by:  Zhichao Wang 2016/7/24
******************************************************/
	LIBBAI_API BAI_Code BAI_Retrieval(BAI_InputItem* pstInputItems, const int kiInputNum, BAI_ResultList*& rpstResultLists, void** ppThreadIndex);  //whole template retrieval. 
    
	LIBBAI_API BAI_Code BAI_Retrieval_VAD(BAI_InputItem* pstInputItems, const int kiInputNum, BAI_ResultList*& rpstResultLists, void** ppEngineHandle);
	
	LIBBAI_API BAI_Code BAI_Retrieval_Partly(BAI_InputItem* pstInputItems, const int kiInputNum, BAI_ResultList*& rpstResultLists, void** ppThreadIndex);  //partly template retrieval.
    
	LIBBAI_API BAI_Code BAI_Retrieval_Partly_VAD(BAI_InputItem* pstInputItems, const int kiInputNum, BAI_ResultList*& rpstResultLists, void** ppThreadIndex);   //using VAD to remove silence segment in template audio.

    LIBBAI_API BAI_Code BAI_Retrieval_(int typeRetrieval, char* pathIndex, BAI_InputItem* pstInputItems, const int kiInputNum, BAI_ResultList*& rpstResultLists, void** ppThreadIndex);
	
	// Step6. close processing thread.
	LIBBAI_API BAI_Code BAI_Close(void** ppThreadIndex);

	// step7. exist system and free all resources.
	LIBBAI_API BAI_Code BAI_Exit();

#ifdef __cplusplus
}
#endif
