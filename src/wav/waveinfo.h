/*************************************************************************
    > File Name: waveinfo.h
    > Author: ma6174
    > Mail: ma6174@163.com 
    > Created Time: Fri 22 May 2015 03:56:29 AM PDT
 ************************************************************************/

#ifndef WAVEINFO_H
#define WAVEINFO_H
#define SAMPLE_RATE 8000//8000 sample rate.
#define QUANTIZATION 0x10//16bit量化
#define BYTES_EACH_SAMPLE 0x2//QUANTIZATION/8
#define CHANNEL_NUN 0x1//单声道
#define FORMAT_TAG 0x1 //线性pcm
/*----------------Wave File Structure----------------*/
typedef struct RIFF_CHUNK{
	char fccID[4];//must be "RIFF"
	unsigned int dwSize; //all bytes of the wave file subtracting 8.
	char fccType[4];//must be "WAVE"
}WAVE_HEADER;//12bytes.
typedef struct FORMAT_CHUNK{
	char fccID[4];//must be "fmt"
	unsigned int dwSize;//size of this struct, subtracting 8, which is the size of fccID and dwSize.
	unsigned short wFormatTag;//one of these: 1: linear; 6: alaw; 7: u-law;
	unsigned short wChannels;//channel number
	unsigned int dwSamplesPerSec;//sample rate.
	unsigned int dwAvgBytesPerSec;//bytes number per second
	unsigned short wBlockAlign;//NumChannels*uiBitsPerSample/8
	unsigned short uiBitsPerSample;//quantization.
}WAVE_FORMAT;//24bytes
typedef struct{
	char fccID[4];//must be "data"
	unsigned int dwSize;//byte_number of PCM data in byte
}WAVE_DATA;//8bytes.
typedef struct {
	WAVE_HEADER riff;
	WAVE_FORMAT fmt;
	WAVE_DATA dt;
}PCM_HEADER;
#ifdef __cplusplus
#if __cplusplus
//extern "C" {
#endif
#endif
void initialize_wave_header(PCM_HEADER *hd, unsigned long len);
bool read_wave_header(char *buf, PCM_HEADER *ptrHd, unsigned long  *ptrSkippedLen);
bool read_wave_header(std::istream& is, PCM_HEADER *ptrHd, long *outlen);
#ifdef __cplusplus
#if __cplusplus
//}
#endif
#endif
#endif
