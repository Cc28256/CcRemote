// TestLoadDll.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <Windows.h>

int main()
{
    std::cout << "Hello World!\n";
	char strHost[] = "127.0.0.1";          //声明上线地址
	int  nPort = 80;                     //声明上线端口
	//载入服务端dll
	HMODULE hServerDll = LoadLibrary(".\\..\\..\\bin\\server\\CcMainDll.dll");
	//HMODULE hServerDll = LoadLibrary(".\server.dll");
	//声明导出函数类型--查看上一节导出的TestRun函数
	typedef void(_cdecl *TestRunT)(char* strHost, int nPort);
	//寻找dll中导出函数
	TestRunT pTestRunT = (TestRunT)GetProcAddress(hServerDll, "TestFun");
	//判断函数是否为空
	if (pTestRunT != NULL)
	{
		pTestRunT(strHost, nPort);   //调用这个函数
	}

}
