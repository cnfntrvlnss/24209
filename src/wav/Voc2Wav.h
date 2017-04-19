/////////////////////////////
#ifndef VOC2WAV_H
#define VOC2WAV_H
#include <math.h>

#ifdef WIN32
#include <windows.h>

#else
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
typedef unsigned char       BYTE;
typedef BYTE                *PBYTE;
typedef short               SHORT;
#endif

#define WAVSMPSERSEC 8000
#define WAVBITSPERSMP 16

/*
	return a buffer pointer containing the resulting wave data. 
	nBufferSize: the returning size in BYTE of the buffer.
	the memory of the returning buffer need NOT pre-malloc before calling this function, but
	you DO NEED release this buffer after you finish using it.
	other settings are the same as above.
 */
PBYTE WavExtractToBufferG(const char *szInputFile,
    unsigned preHeadCutms, unsigned preTailCutms, unsigned minProcessms, unsigned maxProcessms,
    unsigned *nBufferSize, float *fTotalSecond);

//comm.h
#ifndef PI
#define PI 3.14159265428
#endif
#ifndef TPI
#define TPI (2.0*PI)
#endif

#endif
