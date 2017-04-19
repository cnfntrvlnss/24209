/*****************************************************************************/
/*M*
//                        THINKIT INTERNATIONAL PROPRIETARY INFORMATION
//        This software is supplied under the terms of the license agreement
//		or nondisclosure agreement with Thinkit International and may not be copied
//		or disclosed except in accordance with the terms of that agreement.
//            Copyright (c) 1999-2000 Thinkit International. All Rights Reserved.
//     VSS:
//     $Workfile:: config.h                           $
//     $Author:: Jjwang                               $
//     $Revision:: 1                                  $
//     $Modtime:: 11/09/13 20:39                      $
//     $NoKeywords: $
//
// --------------------------------------------------------------
//								config.h
//	Description:
//			Read User Config Module
//	Author:
//			Thinkit International Speech Lab.
//	Date:
//			99/8/24
// --------------------------------------------------------------
M*/

#ifndef aaaCONFIGURATIONaaa
#define aaaCONFIGURATIONaaa
#include "isdt.h"
// ---------------------------
// aConfigEnv Structure.
// ---------------------------
// typedef struct aConfigEnv;
struct aConfigEnv
{
    aConfigEnv *next;
    char256 env;
    char256 def;
};
// ---------------------------
// Definition of Class Config.
// ---------------------------
class Config
{
protected:

    aConfigEnv *envs;
    FILE *fenv;
    char *value;
    bool exam;

    char *GetEnv(char *env);

public:
    // =%d
    void ReadConfig(char *line,int& num);
    // =%d %d
    void ReadConfig(char *line,int& num1,int& num2);
    // =true/false
    void ReadConfig(char *line,bool& bln);
    // =%g
    void ReadConfig(char *line,float& num);
    // =%g %g
    void ReadConfig(char *line,float& num1,float& num2);
    // =%g %g %g
    void ReadConfig(char *line,float& num1,float& num2,float& num3);
    // =%g,%g %g,%g
    void ReadConfig(char *line,float& n1,float& n2,float& n3,float& n4);
    // =%s
    void ReadConfig(char *line,char *str);
    // =%s %s
    void ReadConfig(char *line,char *str1,char *str2);

    Config(int argc,char *argv[],char *head=NULL);
    ~Config(void);

    // modified by ccao on 2007.12.12

    Config(char *configFile);
};

#endif
