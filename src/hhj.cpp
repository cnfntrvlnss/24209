
#include "utilites.h"
#include "hhj.h"
//#include "porting.h"
#include "./TLI_API.h"
#include "commonFunc.h"
//#include <string.h>
using namespace std;

//#include "log4z.h"


///////////////speaker model storage//////////////////
#ifdef HAVE_SPKREC
void* g_pSpeakerModel[MAX_SPKMODEL_NUM];
int g_TSI_nSpkID[MAX_SPKMODEL_NUM];
int g_Spk_HarmLevel[MAX_SPKMODEL_NUM];
char g_Spk_Flag[MAX_SPKMODEL_NUM];
float SpkThreshold[MAX_SPKMODEL_NUM + 1];// last place for default threshold.
int g_TSI_nSpkMdlNum = 0;
pthread_rwlock_t g_spklistupdating=PTHREAD_RWLOCK_INITIALIZER;

bool spkRegProcess(short *waveData, int smpNum, RESULT_STRUCT &result, int thread_id)
{
	result.isSpeakerResult = false;
	result.isLangResult = false;
	result.speaker[0] = '\0';
	result.speakerHarmLevel = 0;
	result.speakerFlag = 0;
	result.speakerID = -1;
	result.speakerConfidence = INT_MIN;

	pthread_rwlock_rdlock(&g_spklistupdating);
	float Scores[MAX_SPKMODEL_NUM];
	for(int i=0; i<MAX_SPKMODEL_NUM; i++){
		Scores[i] = -1000.0;
	}

	TITStatus ret = TIT_SPKID_VERIFY_CLUSTER(waveData,smpNum,(const void**)g_pSpeakerModel,g_TSI_nSpkMdlNum,Scores);
	if(ret != StsNoError) {
		LOG_ERROR(g_logger, "error: engine call TIT_SPKID_VERIFY_CLUSTER fails. ret: "<< ret);
	}
	else {
		LOG_DEBUG(g_logger, "SPKREG after calling spkid_verify, THRESHOLDARR= "<<arr2str(SpkThreshold, g_TSI_nSpkMdlNum) <<"; SCOREARRAY="<< arr2str(Scores, g_TSI_nSpkMdlNum)<<"; IDARRAY="<< arr2str(g_TSI_nSpkID, g_TSI_nSpkMdlNum)<<";");
		//TODO 对于每个说话人都有一个阈值，这种根据得分列表取出命中说话人的方式显然是不可取的，需要新的方法
		int nMax = 0;
		float maxScore = 0.0;
		for(int i=1;i<g_TSI_nSpkMdlNum;++i){
			if(Scores[i] > Scores[nMax])
				nMax=i;
		}
		maxScore = Scores[nMax];
		if(maxScore >= SpkThreshold[nMax]){
			result.isSpeakerResult = true;
			result.speakerID = g_TSI_nSpkID[nMax];
			result.speakerConfidence = maxScore;
			strcpy(result.speaker, int2str(g_TSI_nSpkID[nMax]).c_str());
			result.speakerHarmLevel = g_Spk_HarmLevel[nMax];
			result.speakerFlag = g_Spk_Flag[nMax];
			//dumpResultStruct(result, "for debug wrong result file problem, after filling the struct with speaker result: ");
		}
		
	}

	pthread_rwlock_unlock(&g_spklistupdating);

	return true;
}
#endif

/**
 *
 * return nMax: idx of language in file sysdir/lang.list.
 */
/*
bool LIDScore(short* wavdata,int wavLen,int &nMax,float &score,int hTLI)
{
	int LanguageNUM = g_TEMPLATENUM;
	int *arrTemplateIDs = pnAllTemplateIDs;
	int ResID;float ResScore;
	int nMaxSpeechSec = 3600,nMinSpeechSec = 5;

	nMax = -1;
    score = -1000;
    TLI_Recognize_1(hTLI,arrTemplateIDs,LanguageNUM, (void*)wavdata, wavLen * sizeof(short), nMinSpeechSec,nMaxSpeechSec,ResID,ResScore,"");
    LOG_DEBUG(g_logger, "after LIDREG, ResID="<< ResID<<" ResScore="<< ResScore << " " << &pnAllTemplateIDs<< " "<< pnAllTemplateIDs<< " "<< &g_TEMPLATENUM << " "<< g_TEMPLATENUM);

    nMax = ResID;
    score = ResScore;
    return true;
}
*/

void Trim(string &str)
{
	while((str[str.length()-1]==' ')||(str[str.length()-1]=='	'))
	{
		str[str.length()-1]=0;
		str=&str[0];
	}
	while((str[0]==' ')||(str[0]=='	'))
	{
		str=&str[1];
	}
}

int  LoadList(const char* file,vector<string> &WavTlist)
{
	string p_cLine;
	WavTlist.clear();
	ifstream p_fpTxt(file);
	if(!p_fpTxt)
	{
		printf("Cannot Open %s For Read\n",file);
		return -1;
	}
	while(getline(p_fpTxt,p_cLine))
	{
		Trim(p_cLine);
		if(p_cLine.empty())
			continue;
		WavTlist.push_back(p_cLine);
	}
	p_fpTxt.close();
	return WavTlist.size();
}
