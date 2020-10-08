// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"

#define STR_CRY_LENGTH			0	//加密字符串的长度
char* uncode(char* str)
{
	int len = str[0];
	char * uncodeStr = (char *)operator new(len + 1);
	for (size_t i = 1; i <= len; i++)
	{
		uncodeStr[i - 1] = str[i] ^ (0xCC - i);
	}
	uncodeStr[len] = 0x00;
	return uncodeStr;
}



BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		MessageBoxA(NULL, "dllmain", "test", NULL);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


extern "C" __declspec(dllexport) void wiohrwiaq ()
{

}


extern "C" __declspec(dllexport) void ioqerbwqiaweplkqpoewq ()
{

}

extern "C" __declspec(dllexport) void eiwqiothhahndna()
{

}


/*
为什么C++生成的Dll函数名带有@？如“_Abcd2@4”后面是数字2加@还有个4
_stdcall调用约定的函数会有@，后面的数字表示参数总共所占字节数，这是因为_stdcall函数需要被调用者清空堆栈,所以需要知道参数所占大小
_cedcl调用约定的函数没有@及后面的数字，因为_cedcl调用约定的函数由调用者清空堆栈
*/
extern "C" __declspec(dllexport) int __stdcall run(HMODULE hLibModule)
{
	MessageBoxA(NULL, "hello", "test", NULL);
	char Str_Kernel32[] = { 0x08,0xa0,0xaf,0xbb,0xa6,0xa2,0xaa,0xf6,0xf6 };
	char Str_GetMoudleFileNameA[] = { 0x12,0x8c,0xaf,0xbd,0x85,0xa8,0xa2,0xb0,0xa8,0xa6,0x84,0xa8,0xac,0xda,0xf0,0xdc,0xd1,0xde,0xfb };
	char Str_MainDat[] = { 0x0c,0x97,0x89,0xaa,0xfa,0xff,0xf4,0xf0,0xf2,0xed,0xa6,0xa0,0xb4 };

	char SelfPath[MAX_PATH] = { 0 };

	char* lpKernel32 = uncode(Str_Kernel32);
	HMODULE MoudleHandle = GetModuleHandleA(lpKernel32);			// 解密调用
	memset(lpKernel32, 0, Str_Kernel32[STR_CRY_LENGTH]);
	delete lpKernel32;


	//声明导出函数类型--导出的TestRun函数
	typedef DWORD(__stdcall  *TestRunT)(
		_In_opt_ HMODULE hModule,
		_Out_writes_to_(nSize, ((return < nSize) ? (return +1) : nSize)) LPSTR lpFilename,
		_In_ DWORD nSize
		);


	char* lpGetMoudleFileName = uncode(Str_GetMoudleFileNameA);
	TestRunT pGetMoudleFileNameA = (TestRunT)GetProcAddress(MoudleHandle, lpGetMoudleFileName);				// 解密调用
	memset(lpGetMoudleFileName, 0, Str_Kernel32[STR_CRY_LENGTH]);
	delete lpGetMoudleFileName;


	pGetMoudleFileNameA(NULL, SelfPath, MAX_PATH);

	char* FilePath = strrchr(SelfPath, '\\');

	if (FilePath)
	{
		*FilePath = 0x00;
	}
	else
	{
		return 0;
	}

	char* pMainDat = uncode(Str_MainDat);
	lstrcatA(SelfPath, pMainDat);
	memset(pMainDat, 0, Str_Kernel32[STR_CRY_LENGTH]);
	delete pMainDat;


	MessageBoxA(NULL, SelfPath, "test", NULL);


	return FreeLibrary(hLibModule);
}



extern "C" __declspec(dllexport) void qioewiqj()
{

}
