#ifndef DLLLRVAD_H
#define DLLLRVAD_H

#ifdef WIN32
#ifdef DLLLRVAD_EXPORTS
#define DLLLRVAD_API __declspec(dllexport)
#else
#define DLLLRVAD_API __declspec(dllimport)
#endif
#else
#define DLLLRVAD_API 
#endif

#include <vector>
using namespace std;

struct VADCfg
{
	int iThreadNum; // Number of threads.
	int iOutWavSec; // Out buffer time period.
	int iMinProcSec; // Min file time to process.
	int iMaxProcSec; // Max file time to process.
	int iMaxDetectSec; // Max file time to detect music and ring.

	bool bDetectRing;
	bool bDetectSong;
	bool bDetectMusic;

	VADCfg():iThreadNum(1),iOutWavSec(180),iMinProcSec(3),iMaxProcSec(360),iMaxDetectSec(60),bDetectRing(true),bDetectSong(false),bDetectMusic(true){};
};

extern "C"
{       

	bool DLLLRVAD_API InitializeVADs(const VADCfg& rsVADCfg);

	bool DLLLRVAD_API ExtractSpeechBuf(const int iThreadIdx, const short* psPCMBuffer, const int iSampleNumber, short*& psPCMBufferVAD, int& iSampleNumVAD);

	bool DLLLRVAD_API FreeVADs();
};
#endif
