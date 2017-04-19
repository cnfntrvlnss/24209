/******************************DOCUMENT*COMMENT***********************************
*D
*D 文件名称            : WaveFunc.cpp
*D
*D 项目名称            : 
*D
*D 版本号              : 1.1.0001
*D
*D 文件描述            :
*D
*D
*D 文件修改记录
*D ------------------------------------------------------------------------------ 
*D 版本号       修改日期       修改人     改动内容
*D ------------------------------------------------------------------------------ 
*D 1.1.0001     2007.10.05     plu        创建文件
*D*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <conio.h>
#else
typedef short       __int16;
#endif
#include <memory.h>
#include <string.h>
#include <math.h>

#ifndef WAVE_FORMAT_PCM
	#define WAVE_FORMAT_PCM		1
	#define WAVE_FORMAT_MULAW	0x0007 
	#define WAVE_FORMAT_ALAW	0x0006 
#endif

#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */
#define	BIAS		(0x84)		/* Bias for linear code. */

static short seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF,
			    0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};

static __int16 search(	__int16		val,	short		*table,	__int16		size)
{
	__int16		i;

	for (i = 0; i < size; i++) {
		if (val <= *table++)
			return (i);
	}
	return (size);
}

//
// alaw2linear() - Convert an A-law value to 16-bit linear PCM
// 
// 
short alaw2linear(	unsigned char	a_val)
{
	__int16		t;
	__int16		seg;

	a_val ^= 0x55;

	t = (a_val & QUANT_MASK) << 4;
	seg = ((unsigned __int16)a_val & SEG_MASK) >> SEG_SHIFT;
	switch (seg) {
	case 0:
		t += 8;
		break;
	case 1:
		t += 0x108;
		break;
	default:
		t += 0x108;
		t <<= seg - 1;
	}
	return ((a_val & SIGN_BIT) ? t : -t);
}


//
//  ulaw2linear() - Convert a u-law value to 16-bit linear PCM
// 
//  First, a biased linear code is derived from the code word. An unbiased
//  output can then be obtained by subtracting 33 from the biased code.
// 
//  Note that this function expects to be passed the complement of the
//  original code word. This is in keeping with ISDN conventions.
// 
short ulaw2linear(	unsigned char	u_val)
{
	__int16		t;

	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;

	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned __int16)u_val & SEG_MASK) >> SEG_SHIFT;

	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

/*
 * linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 * linear2alaw() accepts an 16-bit integer and encodes it as A-law data.
 *
 *		Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	0000000wxyza			000wxyz
 *	0000001wxyza			001wxyz
 *	000001wxyzab			010wxyz
 *	00001wxyzabc			011wxyz
 *	0001wxyzabcd			100wxyz
 *	001wxyzabcde			101wxyz
 *	01wxyzabcdef			110wxyz
 *	1wxyzabcdefg			111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
unsigned char linear2alaw(__int16		pcm_val)	/* 2's complement (16-bit range) */
{
	__int16		mask;
	__int16		seg;
	unsigned char	aval;

	if (pcm_val >= 0) {
		mask = 0xD5;		/* sign (7th) bit = 1 */
	} else {
		mask = 0x55;		/* sign bit = 0 */
		pcm_val = -pcm_val - 8;
	}

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_end, 8);

	/* Combine the sign, segment, and quantization bits. */

	if (seg >= 8)		/* out of range, return maximum value. */
		return (0x7F ^ mask);
	else {
		aval = seg << SEG_SHIFT;
		if (seg < 2)
			aval |= (pcm_val >> 4) & QUANT_MASK;
		else
			aval |= (pcm_val >> (seg + 3)) & QUANT_MASK;
		return (aval ^ mask);
	}
}


/*
 * linear2ulaw() - Convert a linear PCM value to u-law
 *
 * In order to simplify the encoding process, the original linear magnitude
 * is biased by adding 33 which shifts the encoding range from (0 - 8158) to
 * (33 - 8191). The result can be seen in the following encoding table:
 *
 *	Biased Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	00000001wxyza			000wxyz
 *	0000001wxyzab			001wxyz
 *	000001wxyzabc			010wxyz
 *	00001wxyzabcd			011wxyz
 *	0001wxyzabcde			100wxyz
 *	001wxyzabcdef			101wxyz
 *	01wxyzabcdefg			110wxyz
 *	1wxyzabcdefgh			111wxyz
 *
 * Each biased linear code has a leading 1 which identifies the segment
 * number. The value of the segment number is equal to 7 minus the number
 * of leading 0's. The quantization interval is directly available as the
 * four bits wxyz.  * The trailing bits (a - h) are ignored.
 *
 * Ordinarily the complement of the resulting code word is used for
 * transmission, and so the code word is complemented before it is returned.
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
unsigned char linear2ulaw(__int16		pcm_val)	/* 2's complement (16-bit range) */
{
	__int16		mask;
	__int16		seg;
	unsigned char	uval;

	/* Get the sign and the magnitude of the value. */
	if (pcm_val < 0) {
		pcm_val = BIAS - pcm_val;
		mask = 0x7F;
	} else {
		pcm_val += BIAS;
		mask = 0xFF;
	}

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_end, 8);

	/*
	 * Combine the sign, segment, quantization bits;
	 * and complement the code word.
	 */
	if (seg >= 8)		/* out of range, return maximum value. */
		return (0x7F ^ mask);
	else {
		uval = (seg << 4) | ((pcm_val >> (seg + 3)) & 0xF);
		return (uval ^ mask);
	}

}

bool CheckWavHeader(FILE *p_fpWav,int &p_nSmpNum,short &p_nWavType)
{
	// 注意： 输入的文件指针fpWav的位置应该是文件的初始位置
	//        如果RIFF文件头正常，则返回时，文件指针的位置指向的是实际声音数据的起始位置，
	//		  本函数返回的是声音数据的字节长度

	p_nSmpNum=-1; p_nWavType=-1;

	char magic[4];
	int len,lng,numBytes;
	char c;
	short sht,sampSize,chans;//type,chans;

	fseek(p_fpWav,0,SEEK_SET);
	fread(magic, 4, 1, p_fpWav);
	if (strncmp("RIFF", magic, 4))
	{
		//printf("Input file is not in RIFF format");
		return false;
	}

	fread(&lng, 4, 1, p_fpWav);
	fread(magic, 4, 1, p_fpWav);
	if (strncmp("WAVE", magic, 4))
	{
		//printf("Input file is not in WAVE format");
		return false;
	}

	/* Look for "fmt " before end of file */
	while(1) 
	{
		if (feof(p_fpWav))
		{
			//printf("No data portion in WAVE file");
			return false;
		}

		fread(magic, 4, 1, p_fpWav);		// RIFF块的标识， “fmt ”或 “data” : ChunkID
		fread(&len, 4, 1, p_fpWav);		// 这一块的大小，字节数						  : ChunkSize

		/* Check for data chunk */
		if (strncmp("data", magic, 4)==0)	// 如果本块是数据块，则退出
			break;
		
		if (strncmp("fmt ", magic, 4)==0)   // 格式块，读取格式信息
		{
			fread(&p_nWavType, 2, 1, p_fpWav);		// 读取数据类型，例如： PCM， ALaw，Mulaw等
			if (p_nWavType != WAVE_FORMAT_PCM && p_nWavType!=WAVE_FORMAT_MULAW && p_nWavType!=WAVE_FORMAT_ALAW)
			{
				//printf("Only standard PCM supported.\a");//, mu-law & a-law supported");
				return false;
			}

			fread(&chans, 2, 1, p_fpWav);     // Number of Channels, 单通道或双通道 
			if (chans!=1) //if (chans!=1 && chans!=2)
			{
				//printf("No single channel\n");
				return false;
			}
			
			fread(&lng, 4, 1, p_fpWav);       // Sample Rate , 采样率
			if (lng!=8000)//(lng!=16000 && lng!=8000)
			{
				//printf("sample rate %d is not support!\n",lng);
				return false;
			}
			fread(&lng, 4, 1, p_fpWav);                  // Average bytes/second , 每秒的字节数。例如：8k16bit时为 8000×2=16000
			fread(&sht, 2, 1, p_fpWav);                  // Block align , Chunk 的对齐单位
			fread(&sampSize, 2, 1, p_fpWav);             // Data size ,一个采样点的大小（比特数）
			if (sampSize != 16 && sampSize!=8)
			//if (sampSize != 16 )
			{
				//HRError(6251,"Only 8/16 bit audio supported");
				return false;
			}

			// 类型检查
			if((p_nWavType==WAVE_FORMAT_MULAW||p_nWavType==WAVE_FORMAT_ALAW) && sampSize!=8)
			{
				//HRError(6251,"Only 8-bit mu-law/a-law supported");
				return false;
			}

			// 如果是扩展的RIFF格式，则“fmt ”块的大小可能大于16字节
			// 多余的信息，我们没兴趣
			len -= 16;	
		} // end if (strncmp("fmt ", magic, 4)==0) 

		// Skip chunk :  跳过“fmt ”16字节以后的内容
		for (; len>0; len--) fread(&c,1,1,p_fpWav);

	} // end while(1) 

	//numBytes=len;								// 声音数据的字节数，即data标识后记录的字节大小
	
	p_nSmpNum = len  / (sampSize/8);		// 声音数据的采样点数目
	
	return true;
}

void AddWavHead(FILE *wf,int fl,int p_nWavType)
{	
	short i;
	int l;
	
	fputs("RIFF",wf);

	l=36+fl;
	fwrite(&l,sizeof(int),1,wf);
	
	fputs("WAVE",wf);
	fputs("fmt",wf);
	fputc(0x20,wf);
	fputc(0x10,wf);
	fputc(0,wf);
	fputc(0,wf);
	fputc(0,wf);
	
	i=(short)p_nWavType;
	fwrite(&i,sizeof(short),1,wf);
	
	i=1;
	fwrite(&i,sizeof(short),1,wf);
	
	l=8000;
	fwrite(&l,sizeof(int),1,wf);

	if (p_nWavType==WAVE_FORMAT_PCM)	l*=2;	
	fwrite(&l,sizeof(int),1,wf);
	
	if (p_nWavType==WAVE_FORMAT_PCM)	
		i=2;
	else if (p_nWavType==WAVE_FORMAT_MULAW || p_nWavType==WAVE_FORMAT_ALAW)
		i=1;
	fwrite(&i,sizeof(short),1,wf);
	
	if (p_nWavType==WAVE_FORMAT_PCM)	
		i=16;
	else if (p_nWavType==WAVE_FORMAT_MULAW || p_nWavType==WAVE_FORMAT_ALAW)
		i=8;
	fwrite(&i,sizeof(short),1,wf);
	
	fputs("data",wf);
	fwrite(&fl,sizeof(int),1,wf);
}

bool Write2File(char *p_pcFileName,short *p_WavBuf,int p_nSmpNum,int p_nWavType)
{
	if (p_WavBuf==NULL)		return false;
	if (p_nWavType!=WAVE_FORMAT_MULAW && p_nWavType!=WAVE_FORMAT_ALAW && p_nWavType!=WAVE_FORMAT_PCM)
		return false;
	if (p_nSmpNum<=0)		return false;

	FILE *fpOut=fopen(p_pcFileName,"wb");
	if (fpOut==NULL)	return false;

	if (p_nWavType==WAVE_FORMAT_PCM)
	{
		AddWavHead(fpOut,sizeof(short)*p_nSmpNum,p_nWavType);
		fwrite(p_WavBuf,sizeof(short),p_nSmpNum,fpOut);
		fclose(fpOut);
	}
	else
	{
		AddWavHead(fpOut,p_nSmpNum,p_nWavType);
		unsigned char *pTransBuf=new unsigned char[p_nSmpNum];
		if (pTransBuf==NULL)
		{
			fclose(fpOut);
			return false;
		}

		if (p_nWavType==WAVE_FORMAT_ALAW)
		{
			for (int i=0;i<p_nSmpNum;i++)
				pTransBuf[i]=linear2alaw(p_WavBuf[i]);
		}
		else if (p_nWavType==WAVE_FORMAT_MULAW)
		{
			for (int i=0;i<p_nSmpNum;i++)
				pTransBuf[i]=linear2ulaw(p_WavBuf[i]);
		}
		fwrite(pTransBuf,1,p_nSmpNum,fpOut);

		fclose(fpOut);
		delete []pTransBuf;
	}

	return true;
}


