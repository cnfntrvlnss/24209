/*************************************************************************
	> File Name: sni_fork.cpp
	> Author: 
	> Mail: 
	> Created Time: 2017年05月09日 星期二 16时58分46秒
 ************************************************************************/

#include "SNI_API.h"
#include<iostream>
using namespace std;


int SNI_Init(            
	char * pszSNISystemDirectory,
	int nLineNumber
	)
{
    return 0;
}
int SNI_Exit()
{
    return 0;
}
int SNI_Open(
	SNI_HANDLE &hSNI
)
{
    
    return 0;
}
int SNI_Close(
	SNI_HANDLE hSNI
)
{
    return 0;
}
int SNI_Recognize(
	SNI_HANDLE hSNI,
	short * pvPCMBuffer,
	int nBytesOfBuffer,
	int nMinLimit,
	int nMaxLimit
    ){
return 0;
}


int SNI_GetResult(
	SNI_HANDLE hSNI,
	float * pfResults,
	int nNumber
)
{
    for(int idx=0; idx < nNumber; idx++){
        pfResults[idx] = 0.0;
    }
    if(nNumber > 1) pfResults[1] = 1.0;
    return 0;
}

