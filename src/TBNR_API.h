/////////////////////////////// "TBNR_API.h" ///////////////////////////////////////////////////////////////////
//
//       THINKIT LABORATORY / CORPORATION PROPRIETARY INFORMATION
//
//       Implementations for interactive command processing.
//
//       Created:   2006/07/14 for Dalian Project with Twopass, multi-decoder, with gender model classification
//
//       Modified:  2007/06/16 - 07/07/05 for New Onepass(N), Multi-Session, with only one base model, saus CN
//
//       

#ifndef _TBNR_API_HEADER
#define _TBNR_API_HEADER 1

#define TBNR_API_VERSION_NUMBER		"2.3.0"

#ifndef TBNR_API_API
#ifdef WIN32
#define TBNR_API_API __declspec(dllexport)
#else
#define TBNR_API_API
typedef long long int __int64;
TBNR_API_API long GetTickCount();
#endif
#endif

#define  __KWS242__ 1
#define __KWS_PLPP__		1  
#define __KWS_MUL_MODE__	1
#define BACK_TRACE_ALL		1
#define	TRACE_CN_FEAT		1		//trace the feature of the cn files!
#define	TRACE_CN_TRIPHONE	1

#define ENHANCED_RESULT		1


// 系统采样率, 影响到端点检测等前端部分, 提特征的参数FYT_SourceRate在配置文件中定义, 目前没有自动调整
//#define SampleRate 8000
TBNR_API_API int TBNR_SampleRate();

// -----******-----第一部分	变量和结构定义-----******------

enum SOUND_TYPE
{	// 语音参考分类
	MALE=0, //男
	FEMALE, //女
	MUSIC, //音乐
	NOISE, //噪声
	UNK,
};
struct ACand
{
	char *text;   // 候选文本结果
	float score;   // 候选得分，0.00~100.00，candNum个
	float startTime;   // 开始时刻，单位为秒s
	float endTime;   // 结束时刻，单位为秒s
	ACand *next;   // 到下一个的指针

#ifdef ENHANCED_RESULT
	char * phone;//phoneme string
	char * segTime;//segTime string
#endif
	ACand (){
		text = 0;
		next = 0;
		score = startTime = endTime = 0;
		phone = 0;
		segTime = 0;
	};
};
struct WordResult   // 具体单个词的信息，首个代表当前全句
{
	int   candNum; //该词可选个数+1，至少为1不固定，首个为最终结果1-best，通常为第一选
	ACand *cands;
	WordResult *next;
	WordResult(){
		candNum = 0;
		cands = 0;
		next = 0;
	}
};

#ifdef __KWS_PLPP__
//9-23-2009, JGao for PLPP calculation
//tri-phone information for a word 
typedef struct _TriPhoneLnk {
	int dur;
	int start;
	char phone[32];
	char lphone[32];
	char rphone[32];
	unsigned short phoneIdx;		// corresponding to state index
	float PLPP;						// PLPP confidence
	_TriPhoneLnk()
	{
		dur			= 0;
		start		= 0;
		phone[0]	= '\0';
		lphone[0]	= '\0';
		rphone[0]	= '\0';
		phoneIdx = 0;
		PLPP = -100;
	}
}aTriPhoneLnk;


//for a keyword 
typedef struct _aWORDLINK
{
	char	word[256];
	int		starttime;			//start time in frame numbers
	int		endtime;			//end time in frame numbers
	float	cn_confidence;		//confidence from confusion network
	float	plpp_confidence;	//confidence from the plpp
	float	mix_confidence;		//confidence from fusions
	int		nPhone;
	aTriPhoneLnk*triphone;		//tri-phone information
	_aWORDLINK()
	{
		word[0]			='\0';
		starttime		=0;
		endtime			=0;
		cn_confidence	=0;
		plpp_confidence	=0;
		mix_confidence	=0;
		triphone		=0;
		nPhone			=0;
	}
}aWORDLINK;
#endif

//--------------------lattice structure for API interface---------------------
struct NodeCand
{
         short nodeId; 
         float time;    
         short pronId;
         char text[32];   
         NodeCand (){
                   nodeId = 0;
                   pronId=0;
                   time = 0;
                   text[0] = '\0';
         };
};

struct LinkCand
{
         short linkId;
         int sNode;
         int eNode;
         float amProb;
         float lmProb;
         char text[32];   // 候选文本结果
          LinkCand (){
                   linkId = 0;
                   sNode=eNode=0;
                   amProb = lmProb=0;
                   text[0] = '\0';
         };
};

struct LatticeResult   // lattice 的信息
{
         float lmscale;
         float wdpenalty;
         float acscale;
         float base;
         int nodeNum; 
         int linkNum;
         float version;
         NodeCand *nCands;
         LinkCand *lCands;
         LatticeResult(){
                   lmscale = wdpenalty=acscale=base=0;
                   nodeNum = linkNum=0;
                   version = 1.0;
         }
};


struct Task	{     // 详细结果/任务格式定义
	void         *newSet;   // could also be pResultArray, pWORDRESULT
    void         *latSet;	//lattice set
	int           frameNum;   // could also be numberOfTasks, wordNumber
	//unsigned long sessionId;
	long long         sessionId;
	unsigned long saveFileIdx;   // real serialNum, 编号从0开始, 类内唯一
	unsigned long dataTimestamp;   // sample of start
	long          decoderId;   // could also be hHandle
	unsigned long dataTimestampEnd;   // sample of end
	SOUND_TYPE    soundType;
	};

enum TBNR_EVENT
{
     TBNR_EVENT_START_COMPLETE = 0,   // 启动识别完成
     TBNR_EVENT_STOP_COMPLETE,        // 停止识别完成
     TBNR_EVENT_RECOGNITION_COMPLETE, // 识别完成
     TBNR_EVENT_RECOGNITION_DOING,    // 识别中
	 TBNR_EVENT_RECOGNITION_IDLE,     // 空闲
	 TBNR_EVENT_RECOGNITION_ERROR,    // 异常
};   // 061230 即时反馈解码器状态
enum TBNR_ERROR
{
     TBNR_SUCCESS = 0,
     TBNR_ERROR_COMMON = -1,
     TBNR_ERROR_LICENSE_INVALID = -2
};   // 程序返回状态
// 数据格式声明
// 注意: 此处只有目前常用一些格式定义, 没有穷举, 因为组合使用可以更加高效 [6/22/2007 explained by syq]
// 粗分: 语音数据/特征/ 类型, 语音细分采样率/位长/编码类型, 特征细分类型(/维数: 精细参考CreateFeature\comm.h)
// 以语音为主, 兼顾特征: 低2位是wav编码格式(a-law/u-law/lpcm/adpcm), 接下去两位表示表示位长/采样率
// 高4位组合, 0表示wav语音, 1(16)表示特征, 2(32)表示类型(共解码器参考), 3(48)或其它表示其它压缩格式
enum SPEECHTYPE
    {TSR_ALAW_PCM         = 0,       //8k 8bit a-law
     TSR_ULAW_PCM         = 1,       //8k 8bit u-law
     TSR_LINEAR_PCM       = 2,       //8k 8bit linear pcm, centered at value 128
     TSR_ADPCM            = 3,       //ADPCM
     TSR_RAW_16           = 4 | TSR_LINEAR_PCM,   //8k 16 bit raw data
     TSR_16K_16           = 8 | TSR_RAW_16,       //16k 16 bit raw data
     TSR_MFCC             = 16,      //TSR MFCC format
     TSR_MFCCPLP          = 17,
     TSR_RASTAPLP         = 18,
     TSR_WITHEAD          = 19,      //with Head
     TSR_MFCC_FE          = 8 | TSR_MFCC,      //TSR MFCC format of Female
     TSR_MFCCPLP_FE       = 8 | TSR_MFCCPLP,
     TSR_RASTAPLP_FE      = 8 | TSR_RASTAPLP,
     TSR_WITHEAD_FE       = 8 | TSR_WITHEAD,
     TSR_SILENCE          = 32,      //silence
     TSR_LOST             = 33,      //lost samples   // 64
     };

//增加和TKW的接口，以便能够检查两边的内存大小是正常的
struct TKWResultSize{
	int mysize;
	int HypoElementSize;
	int SliceElementSize;
	int SegElementSize;
	int ConfusionNetSize;
	int TimeSize;	
	int	OffPtrSize;
	int HypoIDSize;
	int	SliceIDSize;
	int	SegIDSize;
	int	DocIDSize;
	int WordIDSize; 
	int HitIDSize;
	int LnProbSize;
};


// -----******-----第二部分	函数和接口定义-----******------
// If not specially mentioned, return value will be 0 if succeeded, or less than 0 if failed.
// 函数的返回值，如无特别说明，成功就是0（一般能返回只能是0），否则返回一个负数，并说明原因

/* 1
Descriptions:   Init TSR engine as configFile and param:NumberOfSession
                初始化识别TBNR引擎，包括识别器个数，buffer长度等
Parameters:
Input:
systemDir, where lvcsr_standard is - absolute path
configFile, a string pointer to configuration file's name
NumberOfSession, now support M path in, if not declared here read from configFile's "NumberOfSession"
Return:
0, success
<0, fail [需要定义错误原因]
其中-1代表版权校验失败*/
TBNR_API_API int TBNR_Init(const char * systemDir, char * configFile, int NumberOfSession = -1);

/* 2
Descriptions:   Shutdown TSR engine
                关闭所有识别引擎, 任务队列, 缓冲区 等资源 */
TBNR_API_API int TBNR_Exit();

/*
Descriptions:   Definition of Result Callback function to be defined by user to handle result
                定义：识别结果回调函数的接口参数
Parameters:
pResultArray, 结果数组
numberOfSentences, 结果个数
sessionId, 任务编号 */
//typedef void (*TBNR_ResultCallback) (TBNRRESULT  *pResultArray, int numberOfSentences);
typedef void (*TBNR_ResultCallback) (const Task *pResultArray, int numberOfTasks, int sessionId);   // only out to user, can't in a parameter
//注意：结果由系统在回调后删除！用户不用过问

/* 3
Descriptions:   User set a predefined result function as the Callback function
                设置识别结果回调函数，一般用于保存结果，也相当于把后续模块包进来
Parameters:
pFunc, 回调函数指针 */
TBNR_API_API int TBNR_SetResultCallbackFunc(TBNR_ResultCallback pFunc, int sessionId = 0);

/*
Descriptions:	User set a predefined rec-result function as the Callback function when KWS is used, the function above returns CN
*/
TBNR_API_API int TBNR_SetRecResultCallbackFunc(TBNR_ResultCallback pFunc, int sessionId = 0);

/*
Descriptions:   Definition of Event Callback function to be defined by user to handle event
                定义：识别事件回调函数，获得引擎状态变化(例如识别结束)
Parameters:
eventID,事件标识， 参见上面的TBNR_EVENT
*/
typedef void (*TBNR_ProcessEventCallback) (int eventID, int sessionId);   // =0:[vs2005]default-arguments are not allowed on this symbol

/* 4
Descriptions:   User set a predefined event function as the Callback function
                设置识别结果回调函数，用于及时通知用户识别器的状态
Parameters:
pFunc, 回调函数指针. */
TBNR_API_API int TBNR_SetEventCallbackFunc(TBNR_ProcessEventCallback pFunc, int sessionId = 0); //func为处理事件回调函数指针


/*系统性能调整、常用配置参数列表
参数名称                取值范围                默认值          说明
sensitivity             0~100                   50              对语音的敏感程度。
speedvsaccuracy         0~100                   50              速度与精准性。该值越大，速度越优先；相反则精准性优先
recognitiontimeout      0~MAXTIMEOUT            5000            最大识别时间（单位：毫秒）。当一次识别时间超过该值的时候，便会停止识别。MAXTIMEOUT由系统平台决定。
incompletetimeout       0~MAXTIMEOUT            40              用户语音后的最大静音时间（单位：毫秒）。当静音时间超过该值时，认为说话结束。MAXTIMEOUT由系统平台决定。
isSaveData              boolen(0,1)             false           是否保存检测到的语音到文件
isSaveFeature           boolen(0,1)             false           是否保存识别过的特征到文件
timestamp               -maxfloat->+maxfloat    0               同步时间戳, 主要用于关键词检测
其中第三项暂时不提供实现接口，第二项必须在识别停顿期设置，否则无效！*/

/* 5
Descriptions:   Set parameters for all engines, usually when it's idle
                设置识别参数
Parameters:
name, 参数名.
value, 参数值.
-1代表失败，可能是值越界，可能是时机不对，也可能是不支持该选项. */
TBNR_API_API int TBNR_SetParam(char *paramName, char *paramValue, int sessionId = 0);

/* 6
Descriptions:   Set parameters for all engines
                获取识别参数
Parameters:
name, 参数名
value, 参数值
-1代表失败，可能是时机不对（变量无效），也可能是不支持该选项. */
TBNR_API_API int TBNR_GetParam(char *paramName, char *paramValue, int sessionId = 0);

/* 7
Descriptions:   Start a engine cycle
                启动识别
Parameters:
sessionId, 识别对话编号，用于标识识别过程. */
TBNR_API_API int TBNR_Start(int sessionId = 0);

/* 8
Descriptions:   Tell an engine to stop recogniton
                发送停止识别命令
-1代表启动终止系统的进程失败. */
TBNR_API_API int TBNR_Stop(int sessionId = 0);
TBNR_API_API int TBNR_Stop(int sessionId,bool isForceToEnd);

/* 9
Descriptions:   send any speech data into engine, it will detect silence or noise automatically.
                发送数据（任务号区分标志）
Parameters:
buf,the pointer to voice buffer, the voice format is PCM 16Bits,16K.
length, the length of voice buffer, in bytes.
timestamp, 时间戳, 用于校准, 一般内部维护, 单位: 样本点数.
           "0"也会校准, 真正让系统维护的话请赋负值 */
TBNR_API_API int TBNR_SendData(char * buf, int length, int sessionId = 0, SPEECHTYPE type = TSR_RAW_16, long timestamp = -1);
//9-23-2009, JGao
//Send in the features and verify a keyword~
#ifdef __KWS_PLPP__
TBNR_API_API int TBNR_VerifyKeyword(aWORDLINK *aWordlnk,float*featSet,int nFrmNum,int nFrmSize,int decoderID=0,int sessioId=0);
TBNR_API_API int TBNR_ForcedAlign(aWORDLINK *aWordlnk,float*featSet,int nFrmNum,int nFrmSize,int decoderID=0,int sessioId=0);
#endif

/* 10
Descriptions:   Tell an engine it is end point of speech buffer
                告诉系统录音停止，一旦检测到起点即使检测不到终点也要强制结束、识别 */
TBNR_API_API int TBNR_StopRecording(int sessionId = 0);

/* 11
Descriptions:   
Parameters:
sessionId, . */
TBNR_API_API int TBNR_SetCurSessWavName(int sessionId,char *wavName);

// -----******-----第三部分	功能函数-----******------
#include "string.h"
#include <stdlib.h>
#include <stdio.h>

TBNR_API_API void strpcpy(char *(& dest), const char *source);   // 字符串指针拷贝

TBNR_API_API void candcpy(ACand *(&dest), const ACand *source);   // 候选拷贝

TBNR_API_API void clrWord(WordResult *startw);   // 清除词信息

TBNR_API_API void GetTime(FILE *fp = NULL);   // 获取时间计数
TBNR_API_API void GetTime(char* str);   // 获取时间计数

TBNR_API_API int TBNR_AdaptGlobal(char * systemDir, char* configName, char* tlistName, char* outName);   // 兼容旧版离线自适应

// [6/23/2007 copyed from options.cpp of TSRE4.2 by syq] 080112 rewrite
TBNR_API_API int ConvertStrToInt(const char *optName, char *valStr);

// [2/29/2008 add for vb BSTR (a kind of wide char) support by syq
TBNR_API_API int EnableUnicode(int charType);

// 
TBNR_API_API int TBNR_LoadFile(char *filename, char *ext, Task* (&aResult), int sessionId = 0);

// [1/3/2010 add by syq for file process
TBNR_API_API int TBNR_Convert(char *szInFile, int nSrcSampleRate, char *(&szOutFile), int nDestSampleRate=TBNR_SampleRate());
TBNR_API_API int TBNR_SegWav (char *szInFile, int nSmpRate, char *(&szOutDir), float bic_base=1.5f, float bic_step=0.1f, int winsize=100, int winshift=10);
TBNR_API_API long TBNR_OpenFile(FILE*(&aFile), char *fileName, SPEECHTYPE *type = NULL, char *mode = "rb");
TBNR_API_API int TBNR_SendFile(char *szInFile, TBNR_EVENT &myevent, int nSmpRate=0, int nFrmRate=25, char *szOutFile=NULL, float tstart=0, float tend=0, TBNR_EVENT *fatherevent=NULL);
TBNR_API_API int TBNR_SendList(char *szInList, TBNR_EVENT *(&pevent));
TBNR_API_API int TBNR_GetFeature(short *waveData, int smpNum, int sessionId, float *(&newSet), int &frameNum, int &vectorSize);
TBNR_API_API int TBNR_FreeFeature(float *(&newSet));
#endif //_TBNR_API_HEADER

// -----******-----第四部分	流程参考-----******------
/*
usage:
main flow:
1,	TBNR_Init();
2,	TBNR_Start();
3-4	TBNR_SetResultCallbackFunc();
	TBNR_SetEventCallbackFunc();
	[G/SetParam]SetParam
5,	TBNR_SendData();
6,	TBNR_StopRecording();
7,	TBNR_Stop();
8,	TBNR_Exit();

Callback implement:
TBNR_ProcessEventCallback()
TBNR_ResultCallback()
	
CAUTION:
always check the return value;
	  
*/
