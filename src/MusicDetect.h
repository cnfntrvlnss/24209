/* 
 * File:   MusicDetect.h
 * Author: huanghoujun
 *
 * Created on 2014年1月6日, 下午9:36
 */

#ifndef MUSICDETECT_H
#define	MUSICDETECT_H

#ifdef WIN32
#ifdef DLLMUSICDETECT_EXPORTS
#define MUSIC_DETECT_API __declspec(dllexport)
#else
#define MUSIC_DETECT_API __declspec(dllimport)
#endif
#else
#define MUSIC_DETECT_API
#endif

extern "C"
{
    MUSIC_DETECT_API bool MusicCut_Initial(char* CfgFile,int ThreadNum);
    MUSIC_DETECT_API bool MusicCut(int ThreadIndex,short* src,int iLen,short* &dst,int &oLen);
    MUSIC_DETECT_API bool MusicCut_Free();
}

#endif	/* MUSICDETECT_H */

