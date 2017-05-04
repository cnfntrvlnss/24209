#pragma pack( push, enter_tgid_api )
#pragma pack ( 8 )

#ifndef __SNIAPI_HEADER_
#define __SNIAPI_HEADER_ 1

#ifndef _MT
#ifdef WIN32
#error("SNI needs multithreading")
#endif
#endif

#if defined( _WIN32 ) || defined ( _WIN64 )
	#ifdef SNI_API_EXPORTS
		#define SNI_DLL_API __declspec(dllexport)
	#else
		#define SNI_DLL_API __declspec(dllimport)
	#endif
#else
    #define SNI_DLL_API
#endif

typedef int SNI_HANDLE;
/***************************函数说明******************************************
*F函数名称			：Get_version
*F函数功能			：获得引擎版本号
*F函数类型			：SNI_DLL_API char*
*F函数参数			：char* version	输入输出 版本信息
*F函数返回值		：0		成功
*F					：-1	失败
*****************************************************************************/
//SNI_DLL_API int Get_version(char* version);

/******************************** 函数说明 ***********************************
*F
*F 函数名称			: SNI_Init
*F 函数功能			: 引擎初始化
*F 函数类型			: SNI_DLL_API int
*F 函数参数			: char * pszSNISystemDirector	输入 模型（SNISysdir）路径;
*F					: int nLineNumber				输入 线程数
*F 函数返回值		: 0							初始化成功; 
*F					: -1							初始化失败，模型文件路径错误
*F					: -2							初始化失败，授权失败
*F					: -3							初始化失败，线程数设置错误
*F					: -4							初始化失败，模型文件缺失
*F					: -10						初始化失败，其他错误
*F 其他说明			: 授权文件名为license.dat，需要放在引擎（SNI_API.dll）文件夹下
*F					: 授权号195
*F
*F*****************************************************************************/
SNI_DLL_API int SNI_Init(            
	char * pszSNISystemDirectory,
	int nLineNumber
	);

/******************************** 函数说明 ***********************************
*F
*F 函数名称			: SNI_Exit
*F 函数功能			: 引擎退出
*F 函数类型			: SNI_DLL_API int
*F 函数参数			: 无参数
*F 函数返回值		: 0							退出成功
*F					: -1							退出失败
*F
*F*****************************************************************************/
SNI_DLL_API int SNI_Exit();

/******************************** 函数说明 ***********************************
*F
*F 函数名称			: SNI_Open
*F 函数功能			: 打开一路线程，获取所开启线程的的句柄
*F 函数类型			: SNI_DLL_API int
*F 函数参数			: SNI_HANDLE & hSNI			输出 所开启线程的的句柄
*F 函数返回值		: 0							打开成功
*F					: -1							打开失败
*F
*F*****************************************************************************/
SNI_DLL_API int SNI_Open(
	SNI_HANDLE &hSNI
);

/******************************** 函数说明 ***********************************
*F
*F 函数名称			: SNI_Close
*F 函数功能			: 关闭一路线程
*F 函数类型			: SNI_DLL_API int
*F 函数参数			: SNI_HANDLE hSNI			输入 所关闭线程的句柄
*F 函数返回值		: 0							关闭成功
*F					: -1							关闭失败
*F
*F*****************************************************************************/
SNI_DLL_API int SNI_Close(
	SNI_HANDLE hSNI
);

/******************************** 函数说明 ***********************************
*F
*F 函数名称			: SNI_Recognize
*F 函数功能			: 性别识别
*F 函数类型			: SNI_DLL_API int
*F 函数参数			: SNI_HANDLE hSNI			输入 当前线程的句柄
*F					: short * pvPCMBuffer			输入 PCM数据缓存指针
*F					: int nBytesOfBuffer			输入 PCM数据缓存大小（字节数）
*F					: int nMinLimit				输入 PCM数据时长最小阈值（以秒计），时长
*F												小于该阈值则拒识
*F					: int nMaxLimit				输入 PCM数据时长最大阈值（以秒计），时长
*F												大于该阈值的部分不作处理
*F 函数返回值		: 0							识别成功
*F					: -1							识别失败
*F
*F*****************************************************************************/
SNI_DLL_API int SNI_Recognize(
	SNI_HANDLE hSNI,
	short * pvPCMBuffer,
	int nBytesOfBuffer,
	int nMinLimit,
	int nMaxLimit
);

/******************************** 函数说明 ***********************************
*F
*F 函数名称			: SNI_GetResult
*F 函数功能			: 获取识别之后的得分
*F 函数类型			: SNI_DLL_API int
*F 函数参数			: SNI_HANDLE hSNI			输入 当前线程的句柄
*F					: float * pfResults			输出 得分数组指针（需要应用程序申请缓存）
*F					: int nNumber					输入 分数个数（性别个数+拒识得分）
*F 函数返回值		: 0							获取成功
*F					: -1							获取失败
*F
*F*****************************************************************************/
SNI_DLL_API int SNI_GetResult(
	SNI_HANDLE hSNI,
	float * pfResults,
	int nNumber
);

#endif /* __SNIAPI_HEADER_ */

#pragma pack( pop, enter_tgid_api )
/* ///////////////////////// End of file "SNI_API.h" //////////////////////// */
