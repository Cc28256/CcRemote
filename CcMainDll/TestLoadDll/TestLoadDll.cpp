// TestLoadDll.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <Windows.h>

int main()
{
    std::cout << "Hello World!\n";

	//载入服务端dll hijack test
	HMODULE hServerDll = LoadLibrary(".\\..\\..\\bin\\server\\CcMainDll.dll");

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
