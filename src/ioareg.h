/*************************************************************************
	> File Name: ioareg.h
	> Author: 
	> Mail: 
	> Created Time: Tue 22 Nov 2016 04:01:31 AM EST
 ************************************************************************/

#ifndef _IOAREG_H
#define _IOAREG_H


#include "../include/interface242.h"
#include "spk_ex.h"
#include "utilites.h"
#include "commonFunc.h"
#include "ProjectBuffer.h"

using namespace std;
//using namespace BufferGlobal;
using namespace zen4audio;

//extern bool g_bUseBamp;
//extern char m_TSI_SaveTopDir[];
extern const unsigned int g_uVADType;
extern const unsigned int g_uVADID;
extern const unsigned int g_uMusicType;
extern const unsigned int g_uMusicID;

extern LoggerId g_StatusLogger;
extern std::vector<std::pair<unsigned int, std::pair<unsigned int, unsigned int> > > g_mLangReports;

extern bool g_bSaveAfterRec;
bool ioareg_init();
bool ioareg_rlse();
bool reportIoacasResult(CDLLResult &result, char *writeLog, unsigned &len);
extern bool g_bUseSpk;
/**
 * SpkInfo for speaker template id_type_harm.param
 */
class SpkInfoChd: public SpkInfo
{
    SpkInfoChd(const SpkInfoChd&);
    SpkInfoChd& operator=(const SpkInfoChd&);
public:
    unsigned char servType;
    int harmLevel;
    explicit SpkInfoChd(unsigned long spkId =0, unsigned char type=0, int level=0):
        SpkInfo(spkId), servType(type), harmLevel(level)
    { }
    string toStr()const{
        ostringstream oss;
        oss << std::dec<< spkId<<"_"<< static_cast<int>(servType)<< "_" << harmLevel<< ".param";
        return oss.str();
    }
    bool fromStr(const char* strSpk);
};

bool addSpkPerFile(const char* szDir, const char* filename);

//////////////////////////////////
typedef struct ProjectRecord{
	ProjectRecord(const char *filepath="", time_t timemark = 0){
		if(timemark == 0){
			this->timemark = time(NULL);
		}
		else{
			this->timemark = timemark;
		}
		this->fp = NULL;
		if(strlen(filepath) > 0){
			if(strlen(filepath) > MAX_PATH){
				this->filename[0] = 0;
			}
			else{
				sprintf(this->filename, "%s", filepath);
			}
			fp = fopen(filepath, "a");
			if(fp == NULL){
				LOG_WARN(g_logger, "fail to open file "<<filepath<<"; error: "<< strerror(errno));
			}
			else{
			LOG_DEBUG(g_logger, "have open file "<<filepath);
			}
		}
	}
	~ProjectRecord(){
		if(fp != NULL){
			fclose(fp);
			LOG_DEBUG(g_logger, "have closed file "<<filename);
		}
	}
	FILE *fp;
	time_t timemark;
	char filename[MAX_PATH];
}ProjRecord_t;

extern std::map<unsigned long,ProjRecord_t> NewReportedID;
extern pthread_mutex_t g_lockNewReported;
//for dubug NewReportedID
inline void debugstring_newreported(const char * head)
{
	char tmpcz[500];
	sprintf(tmpcz, "%s, newreported [%lu] ", head, NewReportedID.size());
    std::string tmpStr = tmpcz;
	LOG_DEBUG(g_logger, tmpStr.c_str());
}

void maintain_newreported(time_t curtime, unsigned seconds);
void ioareg_maintain_procedure(time_t curTime);

#endif
