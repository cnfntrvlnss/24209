/********************************************************************
	created:	2013/05/22
	filename: 	dllSRVADCluster.h
	author:		wanyulong
	
	purpose:	VAD(Voice Activity Detection) 
	            Clustering for Speaker Recognition.
				Multiple Threads supported.
*********************************************************************/
#pragma once

extern "C"
{
    
         bool InitVADCluster(const char* CfgPath);

	 bool VADBuffer(const bool bAllOut, const short* psPCMBuffer, const int iSampleNum, short* psPCMBufferVAD, int& riSampleNumVAD, bool bUseDetector=true);

	 bool VADClusterBuffer(const short* psInWavData, const int iInSampleNum, short* psOutWavDataC1, int& riOutSampleNumC1, short* psOutWavDataC2, int& riOutSampleNumC2);

	 bool FreeVADCluster();
};
