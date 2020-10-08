// TestLoadDll.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <Windows.h>



FILE * pFile;

long lSize;

char * buffer;

size_t result;
bool InitTestReflectiveLoader()
{


	// 一个不漏地读入整个文件，只能采用二进制方式打开

	//pFile = fopen(".\\..\\..\\bin\\server\\CcMainDll.dll", "rb");
	pFile = fopen("C:\\Users\\b\\Desktop\\bin\\server\\CcMainDll.dll", "rb");

	if (pFile == NULL)

	{

		fputs("File error", stderr);

		printf("open file fail");

		return false;

	}



	// 获取文件大小 

	fseek(pFile, 0, SEEK_END);

	lSize = ftell(pFile);

	rewind(pFile);



	// 分配内存存储整个文件

	buffer = (char*)VirtualAlloc(NULL , sizeof(char)*lSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if (buffer == NULL)

	{

		fputs("Memory error", stderr);

		printf("Memory alloc falil");

		return false;

	}



	// 将文件拷贝到buffer中 

	result = fread(buffer, 1, lSize, pFile);

	if (result != lSize)

	{

		fputs("Reading error", stderr);

		printf("Load file to memory falil");

		return false;

	}
	return true;

}

void loadCcmainDllExp()
{
	std::cout << "Hello World!\n";

	//载入服务端dll hijack test
	HMODULE hServerDll = LoadLibrary(".\\..\\..\\bin\\server\\CcMainDll.dll");

	typedef int(_cdecl *TestRunInit)();

	TestRunInit InitTestReflectiveLoader = (TestRunInit)GetProcAddress(hServerDll, "InitTestReflectiveLoader");

	if (InitTestReflectiveLoader != NULL)
	{
		InitTestReflectiveLoader();   //调用这个函数
	}


	//声明导出函数类型--导出的TestRun函数
	typedef void(_cdecl *TestRunT)();
	//寻找dll中导出函数
	TestRunT pReflectiveLoader = (TestRunT)GetProcAddress(hServerDll, "ReflectiveLoader");
	//判断函数是否为空
	if (pReflectiveLoader != NULL)
	{
		pReflectiveLoader();   //调用这个函数
	}

}

int main()
{
  
	InitTestReflectiveLoader();
	PDWORD lpflOldProtect;
	VirtualProtect(buffer, lSize, PAGE_EXECUTE_READWRITE, lpflOldProtect);
	__asm {
		call buffer
	}

	return 0;
}
