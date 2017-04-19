/******************************DOCUMENT*COMMENT***********************************
*D
*D 文件名称            : dllSpkSegClust.cpp
*D
*D 项目名称            : 
*D
*D 版本号              : 1.1.0005
*D
*D 文件描述            :
*D
*D
*D 文件修改记录
*D ------------------------------------------------------------------------------ 
*D 版本号       修改日期       修改人     改动内容
*D ------------------------------------------------------------------------------ 
*D 1.1.0001     2007.06.25     wangwei     创建文件
*D 1.1.0002     2007.06.25     plu         用LP的segment类替换掉王炜的DW_VAD
*D 1.1.0003     2007.07.02     plu         因为GYM的DetectRingFax速度太慢，Detect仍改用WW的DetectRing
*D 1.1.0004     2008.03.26     plu        修改了一些给841做的时候增加的限制
*D 1.1.0005     2008.04.16     plu        按照3所的要求增加两个参数
*D*******************************************************************************/

#define MAX_LEN 9999

#include "stdio.h"
#include "Voc2Wav.h"

static const short g_alaw2pcm[256] = {
	-5504, -5248, -6016, -5760, -4480, -4224, -4992, -4736, -7552, -7296, -8064,
	-7808, -6528, -6272, -7040, -6784, -2752, -2624, -3008, -2880, -2240, -2112, -2496,
	-2368, -3776, -3648, -4032, -3904, -3264, -3136, -3520, -3392, -22016, -20992,
	-24064, -23040, -17920, -16896, -19968, -18944, -30208, -29184, -32256, -31232,
	-26112, -25088, -28160, -27136, -11008, -10496, -12032, -11520, -8960, -8448, -9984,
	-9472, -15104, -14592, -16128, -15616, -13056, -12544, -14080, -13568, -344, -328, 
	-376, -360, -280, -264, -312, -296, -472, -456, -504, -488, -408, -392,
	-440, -424, -88, -72, -120, -104, -24, -8, -56, -40, -216, -200, -248, -232, -152,
	-136, -184, -168, -1376, -1312, -1504, -1440, -1120, -1056, -1248, -1184, -1888,
	-1824, -2016, -1952, -1632, -1568, -1760, -1696, -688, -656, -752, -720, -560,
	-528, -624, -592, -944, -912, -1008, -976, -816, -784, -880, -848, 5504, 5248, 
	6016, 5760, 4480, 4224, 4992, 4736, 7552, 7296, 8064, 7808, 6528, 6272, 7040,
	6784, 2752, 2624, 3008, 2880, 2240, 2112, 2496, 2368, 3776, 3648, 4032, 3904, 3264,
	3136, 3520, 3392, 22016, 20992, 24064, 23040, 17920, 16896, 19968, 18944, 30208,
	29184, 32256, 31232, 26112, 25088, 28160, 27136, 11008, 10496, 12032, 11520, 
	8960, 8448, 9984, 9472, 15104, 14592, 16128, 15616, 13056, 12544, 14080, 13568, 
	344, 328, 376, 360, 280, 264, 312, 296, 472, 456, 504, 488, 408, 392, 440, 424, 
	88, 72, 120, 104, 24, 8, 56, 40, 216, 200, 248, 232, 152, 136, 184, 168, 1376,
	1312, 1504, 1440, 1120, 1056, 1248, 1184, 1888, 1824, 2016, 1952, 1632, 1568, 1760,
	1696, 688, 656, 752, 720, 560, 528, 624, 592, 944, 912, 1008, 976, 816, 784, 880, 848};

	extern bool CheckWavHeader(FILE *p_fpWav,int &p_nSmpNum,short &p_nWavType);
	extern short ulaw2linear(	unsigned char	u_val);
	extern short alaw2linear(	unsigned char	a_val);

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM		1
#endif
#ifndef WAVE_FORMAT_MULAW
#define WAVE_FORMAT_MULAW	0x0007 
#define WAVE_FORMAT_ALAW	0x0006 
#endif

#define  SAMPLE_RATE 8000

	//  [7/5/2010 llu]
	struct WavHead_t
	{
		//Resource Interchange File Flag (0-3) "RIFF"
		char RIFF[4];
		//File Length ( not include 8 bytes from the beginning ) (4-7)
		int FileLength;
		//WAVE File Flag (8-15) "WAVEfmt "
		char WAVEfmt_[8];
		//Transitory Byte ( normally it is 10H 00H 00H 00H ) (16-19)
		unsigned int noUse;
		//Format Category ( normally it is 1 means PCM-u Law ) (20-21)
		short FormatCategory;
		//NChannels (22-23)
		short NChannels;
		//Sample Rate (24-27)
		int SampleRate;
		//l=NChannels*SampleRate*NBitsPersample/8 (28-31)
		int SampleBytes;
		//i=NChannels*NBitsPersample/8 (32-33)
		short BytesPerSample;
		//NBitsPersample (34-35)
		short NBitsPersample;
		//Data Flag (36-39) "data"
		char data[4];
		//Raw Data File Length (41-43)
		int RawDataFileLength;
	};

	enum SPEECHTYPE
	{
		TSR_ALAW_PCM         = 0,       //8k 8bit a-law
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

	FILE *CheckWav(const char *p_filename,int &p_nSmpRate,int &p_nSmpNum,SPEECHTYPE &type)
	{
		// 注意： 输入的文件指针fpWav的位置应该是文件的初始位置
		//        如果RIFF文件头正常，则返回时，文件指针的位置指向的是实际声音数据的起始位置，
		//		  本函数返回的是声音数据的字节长度
		FILE *p_fpWav=fopen(p_filename,"rb");
		if(p_fpWav==NULL)
		{
			printf("Cannot open %s!\n",p_filename);
			return NULL;
		}
		p_nSmpNum=-1; 
		char magic[4];
		int len,lng,nSmpRate;
		short p_nWavType=-1;
		char c;
		short sht,sampSize,chans;//type,chans;
		fseek(p_fpWav,0,SEEK_SET);
		fread(magic, 4, 1, p_fpWav);
		if (strncmp("RIFF", magic, 4))
		{
			rewind(p_fpWav);
			fseek(p_fpWav,0,SEEK_END);
			p_nSmpNum=ftell(p_fpWav);
			/*type=TSR_RAW_16;
			p_nSmpRate=8000;
			p_nSmpNum /= 2;*/
			type=TSR_ALAW_PCM;
			p_nSmpRate=8000;
			rewind(p_fpWav);
			printf("Input file is not in RIFF format");
			return p_fpWav;
		}
		fread(&lng, 4, 1, p_fpWav);
		fread(magic, 4, 1, p_fpWav);
		/*if (strncmp("WAVE", magic, 4))
		{
		printf("Input file is not in WAVE format");
		return false;
		}*/
		/* Look for "fmt " before end of file */
		while(1) 
		{
			if (feof(p_fpWav))
			{
				printf("No data portion in WAVE file");
				return NULL;
			}
			fread(magic, 4, 1, p_fpWav);		// RIFF块的标识， “fmt ”或 “data” : ChunkID
			fread(&len, 4, 1, p_fpWav);		// 这一块的大小，字节数	: ChunkSize
			/* Check for data chunk */
			if (strncmp("data", magic, 4)==0)	// 如果本块是数据块，则退出
				break;
			if (strncmp("fmt ", magic, 4)==0)   // 格式块，读取格式信息
			{
				fread(&p_nWavType, 2, 1, p_fpWav);		// 读取数据类型，例如： PCM， ALaw，Mulaw等
				switch(p_nWavType)
				{
				case WAVE_FORMAT_PCM: 
					type=TSR_LINEAR_PCM;
					break;
				case WAVE_FORMAT_ALAW: 
					type=TSR_ALAW_PCM; 
					break;
				case WAVE_FORMAT_MULAW:
					type=TSR_ULAW_PCM;
					break;
				default:
					printf("Format Category %d not supported!\a", type);
				}

				fread(&chans, 2, 1, p_fpWav);     // Number of Channels, 单通道或双通道 
				//printf("channel=%d\n",chans);
				if (chans!=1) //if (chans!=1 && chans!=2)
				{
					printf("No single channel\n");
				}
				fread(&nSmpRate, 4, 1, p_fpWav);       // Sample Rate , 采样率
				if (nSmpRate!=16000 && nSmpRate!=8000)
				{
					printf("sample rate %d is not support!\n",lng);
				}
				p_nSmpRate = nSmpRate;
				fread(&lng, 4, 1, p_fpWav);                  // Average bytes/second , 每秒的字节数。例如：8k16bit时为 8000×2=16000
				fread(&sht, 2, 1, p_fpWav);                  // Block align , Chunk 的对齐单位
				fread(&sampSize, 2, 1, p_fpWav);             // Data size ,一个采样点的大小（比特数）
				if(sampSize==16)
				{
					type=SPEECHTYPE((int)type|4);
				}

				if (sampSize != 16 && sampSize!=8)
				{
					printf("Only 8/16 bit audio supported");
					sampSize=16;
					type=TSR_RAW_16;
				}

				// 类型检查
				if((p_nWavType==WAVE_FORMAT_MULAW||p_nWavType==WAVE_FORMAT_ALAW))
				{
					if ( sampSize!=8 )
					{
						printf("Only 8-bit mu-law/a-law supported");					
					}
					if ( nSmpRate!=8000 )
					{
						printf("Only 8K mu-law/a-law supported");					
					}
				}
				// 如果是扩展的RIFF格式，则“fmt ”块的大小可能大于16字节
				// 多余的信息，我们没兴趣
				len -= 16;	
			} // end if (strncmp("fmt ", magic, 4)==0) 

			// Skip chunk :  跳过“fmt ”16字节以后的内容
			for (; len>0; len--) fread(&c,1,1,p_fpWav);

		} // end while(1) 
		//numBytes=len;		// 声音数据的字节数，即data标识后记录的字节大小
		p_nSmpNum = len  / (sampSize/8);		// 声音数据的采样点数目
		return p_fpWav;
	}

	FILE * OpenParseWave2(const char *fileName, SPEECHTYPE &type, int &nSameplenum)
	{
		FILE *wav_file = fopen(fileName, "rb");
		if (wav_file==NULL)
		{
			printf("Can not open file: %s\n", fileName);
			return NULL;
		}

		bool isRIFFWave=true;
		WavHead_t aWavHead;
		WavHead_t *WavHead=&aWavHead;
		char riff[8],wavefmt[8];
		int nHeadLen=0;
		short i;
		rewind(wav_file);

		fseek(wav_file,0,SEEK_END);
		nSameplenum = ftell(wav_file);
		rewind(wav_file);

		fread(WavHead,sizeof(struct WavHead_t),1,wav_file);
		nHeadLen+=sizeof(struct WavHead_t);

		for ( i=0;i<8;i++ )
		{
			riff[i]=WavHead->RIFF[i];
			wavefmt[i]=WavHead->WAVEfmt_[i];
		}
		riff[4]='\0';
		wavefmt[7]='\0';
		if ( strcmp(riff,"RIFF")==0 && strcmp(wavefmt,"WAVEfmt")==0 )
		{
			//JGao add for common pcm
			//RIFF Type Chunk Values 12 bytes
			//fmt chunk header 8
			//data id +data length 8
			//Read the size of fmt in noUse
			//printf("HeadSize=%d,%d,%c,%c,%c,%c\n",sizeof(struct WavHead_t),WavHead->noUse,WavHead->data[0],WavHead->data[1],WavHead->data[2],WavHead->data[3]);
			if (sizeof(struct WavHead_t)!=(12+8+WavHead->noUse+8)
				||	tolower(WavHead->data[0])!='d'
				||	tolower(WavHead->data[1])!='a'
				||	tolower(WavHead->data[2])!='t'
				||	tolower(WavHead->data[3])!='a')
			{
				//This must be different
				//RIFF Type Chunk Values 12 bytes
				//fmt chunk header 8
				//fmt chunk WavHead->noUse
				fseek(wav_file,12+8+WavHead->noUse,SEEK_SET);
				//check the fact chuck 
				char fact[5];
				int factSize=0;
				fact[4]='\0';
				fread(fact,4,1,wav_file);
				if ( strcmp(fact,"fact")==0 )
				{
					//如果存在fact块
					//RIFF Type Chunk Values 12 bytes
					//fmt chunk header 8
					//fmt chunk WavHead->noUse
					//Fact header 8
					//Fact data:factSize
					fread(&factSize,4,1,wav_file);
					fseek(wav_file,12+8+WavHead->noUse+8+factSize,SEEK_SET);
					nHeadLen=12+8+WavHead->noUse+8+factSize;
					nHeadLen+=8;
				}
				else
				{
					//如果没有fact块,跳过RIFF头和fmt块
					fseek(wav_file,12+8+WavHead->noUse,SEEK_SET);
					nHeadLen=12+8+WavHead->noUse;
					nHeadLen+=8;
				}
				//Re-read the file size
				fread(&(WavHead->data),8,1,wav_file);
				if (	tolower(WavHead->data[0])!='d'
					||	tolower(WavHead->data[1])!='a'
					||	tolower(WavHead->data[2])!='t'
					||	tolower(WavHead->data[3])!='a'
					)

				{
					//不是wave文件
					isRIFFWave=false;
					rewind(wav_file);
				}
				else
				{
					fseek(wav_file,nHeadLen,SEEK_SET);
					//return	true;  // It is WAV file.
				}
			}
			else
			{
				nHeadLen=44;
				fseek(wav_file,nHeadLen,SEEK_SET);
				//return	true;  // It is WAV file.
			}
		}
		else
		{
			//不是wave 文件
			isRIFFWave=false;
			rewind(wav_file);
		}

		//if (!isRIFFWave)             //default 8k 8bit alaw
		//{
		//	type=TSR_ALAW_PCM; 
		//	return wav_file;
		//}

		if (!isRIFFWave)            //default 8k 16bit PCM
		{
			type=TSR_RAW_16;
			nSameplenum = nSameplenum/2;
			return wav_file;
		}

		//解析格式
		switch(WavHead->FormatCategory)
		{
		case '\01': 
			type=TSR_LINEAR_PCM;
			break;
		case '\06': 
			type=TSR_ALAW_PCM; 
			break;
		case '\07':
			type=TSR_ULAW_PCM;
			break;
		case '\11':
			type=TSR_ADPCM;
			break;
		default:
			printf("Format Category %d! not supported !", WavHead->FormatCategory);
		}

		switch(WavHead->SampleRate)
		{
		case 8000: 
			break;
		case 16000: 
			type = (SPEECHTYPE)(int(type) | 8);
			break;
		default: printf("samplerate of %d! not supported !", WavHead->SampleRate);
		}
		//printf("NBitsPersample=%d,nSampleNum=%d\n",WavHead->NBitsPersample,nSameplenum);
		switch(WavHead->NBitsPersample)
		{
		case 8: 
			nSameplenum = nSameplenum - nHeadLen;
			break;   // 8bit
		case 16:
			nSameplenum = (nSameplenum - nHeadLen)/2;	
			type = (SPEECHTYPE)((int)type | 4); 
			break;
		default: printf("NBitsPersample of %d! not supported !", WavHead->NBitsPersample);
		}


		return wav_file;
	}

    //only support 8k samples.
PBYTE WavExtractToBufferG(const char *szInputFile,
    unsigned preHeadCutms, unsigned preTailCutms, unsigned minProcessms, unsigned maxProcessms,
    unsigned *nBufferSize, float *fTotalSecond)
	{

		SPEECHTYPE type;
		int nSampleNum=0;
		int nSampRate;

		FILE *fpWav= CheckWav(szInputFile,nSampRate,nSampleNum,type);
		if(fpWav==NULL)
		{
			return NULL;
		}

		unsigned iPreCutTailLen=preTailCutms * 8;
		unsigned iPreHeadCutLen=preHeadCutms * 8;
		unsigned iPreProLen=maxProcessms * 8;

        //find the segment to read.
        int iSurLen = 0;
        if(nSampleNum > iPreCutTailLen + iPreHeadCutLen){
            int iSurLen=nSampleNum-iPreHeadCutLen-iPreCutTailLen;
            if(iSurLen < minProcessms * 8){
                iSurLen = 0;
            }
        }
        if(iSurLen == 0){
            fclose(fpWav);
            *nBufferSize = 0;
            return NULL;
        }

		int iStartRead=iPreHeadCutLen;
		if(iSurLen >= iPreProLen)
		{
			nSampleNum=iPreProLen;
		}
		else
		{
			nSampleNum=iSurLen;
		}

		if(type == TSR_RAW_16)
			iStartRead *= 2;

		fseek(fpWav,iStartRead,SEEK_CUR);

		short* pWavBuf = NULL;
		unsigned char *pOrgBuf = NULL;

		int i;
		switch(type)
		{
		case TSR_ALAW_PCM: 
			pWavBuf = (short*)malloc(nSampleNum*sizeof(short));
			if (pWavBuf==NULL)	
			{
				fclose(fpWav);
				*nBufferSize=0;
				return NULL;
			}
			pOrgBuf=new unsigned char[nSampleNum];
			if (pOrgBuf==NULL)
			{
				free(pWavBuf);
				fclose(fpWav);
				*nBufferSize=0;
				return NULL;
			}
			fread(pOrgBuf,sizeof(char),nSampleNum,fpWav);

			fclose(fpWav);

			for (i=0;i<nSampleNum;i++)
				pWavBuf[i]=g_alaw2pcm[pOrgBuf[i]];

			delete []pOrgBuf;

			break;
		case TSR_ULAW_PCM: 
			pWavBuf = (short*)malloc(nSampleNum*sizeof(short));
			if (pWavBuf==NULL)	
			{
				fclose(fpWav);
				*nBufferSize=0;
				return NULL;
			}
			pOrgBuf=new unsigned char[nSampleNum];
			if (pOrgBuf==NULL)
			{
				free(pWavBuf);
				fclose(fpWav);
				*nBufferSize=0;
				return NULL;
			}
			fread(pOrgBuf,sizeof(char),nSampleNum,fpWav);
			fclose(fpWav);

			for (i=0;i<nSampleNum;i++)
				pWavBuf[i]=ulaw2linear(pOrgBuf[i]);

			delete []pOrgBuf;
			break;
		case TSR_RAW_16: 
			pWavBuf = (short*)malloc(nSampleNum*sizeof(short));
			if (pWavBuf==NULL)	
			{
				fclose(fpWav);
				*nBufferSize=0;
				return NULL;
			}
			fread(pWavBuf,sizeof(short),nSampleNum,fpWav);
			fclose(fpWav);
			break;
		default:
			printf("Format Category %d not supported !", type);
		}
		*nBufferSize=nSampleNum*sizeof(SHORT);
		*fTotalSecond=nSampleNum/SAMPLE_RATE;
		return (PBYTE)pWavBuf; 
	}
