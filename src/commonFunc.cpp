#define CHECK_PERFOMANCE
#include "commonFunc.h"
#include "porting.h"

#include <time.h>

zsummer::log4z::ILog4zManager* initLog4z()
{
    static const char*myLogCfg="\
        [main]\n\
        path=./ioacas/log/\n\
        [ioacas]\n\
        path=./ioacas/log/\n\
        level=ALL\n\
        display=true\n\
        monthdir=false\n\
        fileline=false\n\
        enable=true\n\
        outfile=true\n\
        [globalBuffer]\n\
        path=./ioacas/log/\n\
        level=ALL\n\
        display=true\n\
        monthdir=false\n\
        fileline=false\n\
        enable=true\n\
        outfile=true\n\
        ";
    zsummer::log4z::ILog4zManager* tmpPtr = zsummer::log4z::ILog4zManager::getPtr();
    tmpPtr->configFromString(myLogCfg);   
    tmpPtr->config("ioacas/log4z.ini");
    tmpPtr->setAutoUpdate(6);
    tmpPtr->start();
    return tmpPtr;
}

zsummer::log4z::ILog4zManager *g_Log4zManager = initLog4z();

/*
char* GetLocalIP()    
{          
	int MAXINTERFACES=16;    
    static char retIP[50];
    if(retIP[0] != '\0') return retIP;
	const char *ip = "127.0.0.1";    
	int fd, intrface;      
	struct ifreq buf[MAXINTERFACES];      
	struct ifconf ifc;      
	int thrdNum = 0;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)      
	{      
		ifc.ifc_len = sizeof(buf);      
		ifc.ifc_buf = (caddr_t)buf;      
		if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))      
		{      
			intrface = ifc.ifc_len / sizeof(struct ifreq);      

			while (intrface-- > 0)      
			{      
				if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface])))      
				{      
					ip=(inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));      
					//fetch management ip. determined by wheath the third part is even.
					if(strcmp(ip, "127.0.0.1") == 0) continue;
					if(sscanf(ip, "%*d.%*d.%d.%*d", &thrdNum) != 1){
						continue;
					}
                    if(thrdNum %2 == 0){
                      strncpy(retIP, ip, 50);
                      break;       
                    }
				}                          
			}    
		}      
		close (fd);
	}  
	return retIP; 
} 
*/

bool if_directory_exists(const char *dir, bool bForce)
{
	struct stat buf;
	if(stat(dir, &buf)<0 || !S_ISDIR(buf.st_mode)){
		//试着创建目录.
		if(bForce && mkdir(dir, S_IRWXU | S_IRWXG | S_IRWXO)!= -1){
			return true;
		}
		return false;
	}
	return true;
}

int save_binary_data(const char *filePath, const void* ptr, size_t num, ...)
{
    FILE *fp = fopen(filePath, "wb");
    if(fp == NULL){
        LOG_WARN(g_logger, "fail to open file to write data, file: "<< filePath);
        return -1;
    }

    int ret = 0;
    va_list vl;
    va_start(vl, num);
    const void *curPtr = ptr;
    size_t curNum = num;
    while(true){
        int retwr = fwrite(curPtr, 1, curNum, fp);
        ret += retwr;
        if(retwr != curNum){
            LOG_WARN(g_logger, "fail to write data to file, file: " << filePath);
            break;
        }
        curPtr = va_arg(vl, void*);
        if(curPtr == NULL) break;
        curNum = va_arg(vl, size_t);
    }
    
    fclose(fp);
    return ret;
}

/**
 * 文件存储路径为：topdir/200109/01/increNum_ID_username_confidence.wav
 * 文件存储路径写到savedname指向的内存中，作为结果返回。
 */
bool  gen_spk_save_file(char *savedname, const char *topDir, const char *subDir, time_t curtime, unsigned long id, const unsigned short *typeId, const unsigned *userId, const int *confidence)
{
    if(curtime == 0){
        time(&curtime);
    }
	struct tm *tmif;
	tmif = localtime(&curtime);
	char fipnt[10], sepnt[5];
	snprintf(fipnt, 10, "%04d%02d%02d", tmif->tm_year+1900, tmif->tm_mon+1, tmif->tm_mday);
	snprintf(sepnt, 5, "%02d", tmif->tm_hour);
    savedname[0] = '\0';
    int curCnt = 0;
    if(topDir != NULL){
        int topDirLen = strlen(topDir);
        if(subDir == NULL){
            if(topDir[topDirLen - 1] == '/') curCnt = sprintf(savedname, "%s%s", topDir, fipnt);
            else curCnt = sprintf(savedname, "%s/%s", topDir, fipnt);
            if_directory_exists(savedname, true);
            curCnt += sprintf(savedname+ curCnt, "/%s/", sepnt);
            if_directory_exists(savedname, true);
        }
        else{
            if(topDir[topDirLen - 1] == '/')curCnt = sprintf(savedname, "%s%s/", topDir, subDir);
            else curCnt = sprintf(savedname, "%s/%s/", topDir, subDir);
            if_directory_exists(savedname, true); //create subdir if not exists.
        }
    }
    curCnt += sprintf(savedname + curCnt, "%s%s%02d%02d_%lu", fipnt, sepnt, tmif->tm_min, tmif->tm_sec, id);
    if(typeId != NULL){
        curCnt += sprintf(savedname + curCnt, "_%lu", *typeId);
    }
    if(userId != NULL){
        curCnt += sprintf(savedname + curCnt, "_%u", *userId);
    }
    if(confidence != NULL){
        curCnt += sprintf(savedname + curCnt, "_%d", *confidence);
    }

    curCnt += sprintf(savedname + curCnt, ".wav");
	return true;
}

bool saveWave(char *pData, unsigned len, const char *saveFileName)
{
    FILE *fp = fopen(saveFileName, "ab");
    if(fp == NULL){
        LOG_WARN(g_logger, "fail to open file "<< saveFileName<< " error: "<< strerror(errno));
        return false;
    }
    bool ret = false;
    PCM_HEADER pcmheader;
    initialize_wave_header(&pcmheader, len);
    int retw = fwrite(&pcmheader, sizeof(PCM_HEADER), 1, fp);
    if(retw != 1){
        LOG_WARN(g_logger, "fail to save data to file, filename: "<< saveFileName);
    }
    else{
        fwrite(pData, 1, len, fp);
        ret = true;
    }
    fclose(fp);
    return ret;
}

char g_szDebugBinaryDir[MAX_PATH] = "ioacas/debug/";
string saveTempBinaryData(struct timeval curtime, unsigned long pid, char* data, unsigned len)
{
    char filePath[MAX_PATH];
    if(curtime.tv_sec == 0) gettimeofday(&curtime, NULL);
    gen_spk_save_file(filePath, g_szDebugBinaryDir, NULL, curtime.tv_sec, pid, NULL, NULL, NULL);
    char *stSufPtr = strrchr(filePath, '.');
    char tmpMrk[20];
    snprintf(tmpMrk, 20, "_%lu", curtime.tv_usec);
    insertStrAt0(stSufPtr, tmpMrk);
    saveWave(data, len, filePath);
    return filePath;
}
std::string saveProjectSegment(struct timeval curtime, unsigned long pid, unsigned *offset, char* data, unsigned len)
{
    char filePath[MAX_PATH];
    if(curtime.tv_sec == 0) gettimeofday(&curtime, NULL);
    gen_spk_save_file(filePath, g_szDebugBinaryDir, NULL, curtime.tv_sec, pid, NULL, NULL, NULL);
    if(offset){
        char tmpMrk[20];
        char *stSufPtr = strrchr(filePath, '.');
        snprintf(tmpMrk, 20, "_%.2f", ((float)*offset) / 16000);
        insertStrAt0(stSufPtr, tmpMrk);
    }
    saveWave(data, len, filePath);
    return filePath;
}

static std::map<pthread_t, std::vector<std::pair<struct timespec, std::string> > > allClockStack;
LockHelper clockoutput_lock;
void clockoutput_start(const char *fmt, ...)
{
    clockoutput_lock.lock();
    std::vector<std::pair<struct timespec, std::string> > & cur = allClockStack[pthread_self()];
    struct timespec tmptm;
    clock_gettime(CLOCK_REALTIME, &tmptm);
    cur.push_back(std::make_pair(tmptm, ""));
    va_list args;
    va_start(args, fmt);
    cur[cur.size() -1].second.resize(100);
    int retp = vsnprintf(const_cast<char*>(cur[cur.size() -1].second.c_str()), 100, fmt, args);
    cur[cur.size() - 1].second.resize(retp);
    clockoutput_lock.unLock();
}

std::string clockoutput_end()
{
    AutoLock myLock(clockoutput_lock);
    pthread_t thd = pthread_self();
    assert(allClockStack.find(thd) != allClockStack.end());
    std::vector<std::pair<struct timespec, std::string> > & cur = allClockStack[thd];
    struct timespec lastclock = cur[cur.size() -1].first;
    ostringstream oss;
    struct timespec tmptm;
    clock_gettime(CLOCK_REALTIME, &tmptm);
    unsigned long elsus = (tmptm.tv_sec - lastclock.tv_sec) * 1000000 + (tmptm.tv_nsec - lastclock.tv_nsec) / 1000;
    oss<< cur[cur.size() - 1].second << " ElapseClock "<< elsus;
    cur.pop_back();
    return oss.str();
}
