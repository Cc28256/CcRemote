// CSystemDlg.cpp: 实现文件
//

#include "pch.h"
#include "CcRemote.h"
#include "CSystemDlg.h"
#include "afxdialogex.h"
#include "..\..\common\macros.h"


// CSystemDlg 对话框

IMPLEMENT_DYNAMIC(CSystemDlg, CDialog)

CSystemDlg::CSystemDlg(CWnd* pParent /*=nullptr*/, CIOCPServer* pIOCPServer, ClientContext *pContext)
	: CDialog(IDD_SYSTEM, pParent)
{
	m_iocpServer = pIOCPServer;        //参数赋值给成员变量
	m_pContext = pContext;
	m_hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_SYSTEM));
	//这里判断是窗口管理还是进程管理因为进程管理的数据头是TOKEN_PSLIST
	//窗口管理的数据头TOKEN_WSLIST  我们可以用这两个数据头来区分
	char	*lpBuffer = (char *)(m_pContext->m_DeCompressionBuffer.GetBuffer(0));
	m_caseSyetemIs = lpBuffer[0];
}

CSystemDlg::~CSystemDlg()
{
}

void CSystemDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PROCESS_OR_WINDOW, m_list_process_or_windows);
}


BEGIN_MESSAGE_MAP(CSystemDlg, CDialog)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_COMMAND(IDM_KILLPROCESS, &CSystemDlg::OnKillprocess)
	ON_COMMAND(IDM_REFRESHPSLIST, &CSystemDlg::OnRefreshpslist)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_PROCESS_OR_WINDOW, &CSystemDlg::OnNMRClickListProcess)
	ON_COMMAND(ID_WINDOW_CLOST, &CSystemDlg::OnWindowClost)
	ON_COMMAND(ID_WINDOW_HIDE, &CSystemDlg::OnWindowHide)
	ON_COMMAND(ID_WINDOW_MAX, &CSystemDlg::OnWindowMax)
	ON_COMMAND(ID_WINDOW_MIN, &CSystemDlg::OnWindowMin)
	ON_COMMAND(ID_WINDOW_RETURN, &CSystemDlg::OnWindowReturn)
	ON_COMMAND(ID_WINDOW_REFLUSH, &CSystemDlg::OnWindowReflush)
END_MESSAGE_MAP()


// CSystemDlg 消息处理程序

void CSystemDlg::AdjustList(void)
{
	if (m_list_process_or_windows.m_hWnd == NULL)
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

	m_list_process_or_windows.MoveWindow(&rectList);
	
}


void CSystemDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	AdjustList();
	// TODO: 在此处添加消息处理程序代码
}


void CSystemDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialog::OnClose();
}




void CSystemDlg::GetProcessList(void)
{
	BYTE bToken = COMMAND_PSLIST;
	m_iocpServer->Send(m_pContext, &bToken, 1);
}


BOOL CSystemDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon


	CString str;
	sockaddr_in  sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);
	BOOL bResult = getpeername(m_pContext->m_Socket, (SOCKADDR*)&sockAddr, &nSockAddrLen); //得到连接的ip 
	str.Format("\\\\%s - 系统管理", bResult != INVALID_SOCKET ? inet_ntoa(sockAddr.sin_addr) : "");
	SetWindowText(str);//设置对话框标题


	if (m_caseSyetemIs == TOKEN_PSLIST)      //进程管理初始化列表
	{
		m_list_process_or_windows.SetExtendedStyle(LVS_EX_FLATSB | LVS_EX_FULLROWSELECT);  //初始化进程的列表
		m_list_process_or_windows.InsertColumn(0, "映像名称", LVCFMT_LEFT, 100);
		m_list_process_or_windows.InsertColumn(1, "PID", LVCFMT_LEFT, 50);
		m_list_process_or_windows.InsertColumn(2, "程序路径", LVCFMT_LEFT, 400);
		ShowProcessList();   //由于第一个发送来的消息后面紧跟着进程的数据所以把数据显示到列表当中
	}
	else if (m_caseSyetemIs == TOKEN_WSLIST)//窗口管理初始化列表
	{
		m_list_process_or_windows.SetExtendedStyle(LVS_EX_FLATSB | LVS_EX_FULLROWSELECT);  //初始化 窗口管理的列表
		m_list_process_or_windows.InsertColumn(0, "PID", LVCFMT_LEFT, 50);
		m_list_process_or_windows.InsertColumn(1, "窗口名称", LVCFMT_LEFT, 300);
		m_list_process_or_windows.InsertColumn(2, "窗口状态", LVCFMT_LEFT, 300);
		ShowWindowsList();
	}
	AdjustList();       //各个列表的大小
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CSystemDlg::ShowProcessList(void)
{
	char	*lpBuffer = (char *)(m_pContext->m_DeCompressionBuffer.GetBuffer(1));
	char	*strExeFile;
	char	*strProcessName;
	DWORD	dwOffset = 0;
	CString str;
	m_list_process_or_windows.DeleteAllItems();
	//遍历发送来的每一个字符 数据结构 Id+进程名+0+完整名+0
	int i;
	for (i = 0; dwOffset < m_pContext->m_DeCompressionBuffer.GetBufferLen() - 1; i++)
	{
		LPDWORD	lpPID = LPDWORD(lpBuffer + dwOffset);        //这里得到进程ID
		strExeFile = lpBuffer + dwOffset + sizeof(DWORD);      //进程名就是ID之后的
		strProcessName = strExeFile + lstrlen(strExeFile) + 1;  //完整名就是进程名之后的
		//数据结构构建巧妙

		m_list_process_or_windows.InsertItem(i, strExeFile);       //将得到的数据加入到列表当中
		str.Format("%5u", *lpPID);
		m_list_process_or_windows.SetItemText(i, 1, str);
		m_list_process_or_windows.SetItemText(i, 2, strProcessName);
		// ItemData 为进程ID
		m_list_process_or_windows.SetItemData(i, *lpPID);

		dwOffset += sizeof(DWORD) + lstrlen(strExeFile) + lstrlen(strProcessName) + 2;   //跳过这个数据结构 进入下一个循环
	}

	str.Format("程序路径 / %d", i);
	LVCOLUMN lvc;
	lvc.mask = LVCF_TEXT;
	lvc.pszText = str.GetBuffer(0);
	lvc.cchTextMax = str.GetLength();
	m_list_process_or_windows.SetColumn(2, &lvc); //在列表中显示有多少个进程
}


void CSystemDlg::OnKillprocess()
{
	// TODO: 在此添加命令处理程序代码
	CListCtrl	*pListCtrl = NULL;
	if (m_list_process_or_windows.IsWindowVisible())
		pListCtrl = &m_list_process_or_windows;
	else if (m_list_process_or_windows.IsWindowVisible())
		pListCtrl = &m_list_process_or_windows;
	else
		return;

	// TODO: Add your command handler code here
	//非配缓冲区
	LPBYTE lpBuffer = (LPBYTE)LocalAlloc(LPTR, 1 + (pListCtrl->GetSelectedCount() * 4));
	//加入结束进程的数据头
	lpBuffer[0] = COMMAND_KILLPROCESS;
	//显示警告信息
	char *lpTips = "警告: 终止进程会导致不希望发生的结果，\n"
		"包括数据丢失和系统不稳定。在被终止前，\n"
		"进程将没有机会保存其状态和数据。";
	CString str;
	if (pListCtrl->GetSelectedCount() > 1)
	{
		str.Format("%s确实\n想终止这%d项进程吗?", lpTips, pListCtrl->GetSelectedCount());
	}
	else
	{
		str.Format("%s确实\n想终止该项进程吗?", lpTips);
	}
	if (::MessageBox(m_hWnd, str, "进程结束警告", MB_YESNO | MB_ICONQUESTION) == IDNO)
		return;

	DWORD	dwOffset = 1;
	POSITION pos = pListCtrl->GetFirstSelectedItemPosition(); //iterator for the CListCtrl
	//得到要结束哪个进程
	while (pos) //so long as we have a valid POSITION, we keep iterating
	{
		int	nItem = pListCtrl->GetNextSelectedItem(pos);
		DWORD dwProcessID = pListCtrl->GetItemData(nItem);
		memcpy(lpBuffer + dwOffset, &dwProcessID, sizeof(DWORD));
		dwOffset += sizeof(DWORD);
	}
	//发送数据到服务端到服务端查找COMMAND_KILLPROCESS这个数据头
	m_iocpServer->Send(m_pContext, lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);
}

void CSystemDlg::ShowWindowsList(void)
{
	LPBYTE lpBuffer = (LPBYTE)(m_pContext->m_DeCompressionBuffer.GetBuffer(1));
	DWORD	dwOffset = 0;
	char	*lpTitle = NULL;
	//m_list_process.DeleteAllItems();
	bool isDel = false;
	do
	{
		isDel = false;
		for (int j = 0; j < m_list_process_or_windows.GetItemCount(); j++)
		{
			CString temp = m_list_process_or_windows.GetItemText(j, 2);
			CString restr = "隐藏";
			if (temp != restr)
			{
				m_list_process_or_windows.DeleteItem(j);
				isDel = true;
				break;
			}
		}

	} while (isDel);
	CString	str;
	int i;
	for (i = 0; dwOffset < m_pContext->m_DeCompressionBuffer.GetBufferLen() - 1; i++)
	{
		LPDWORD	lpPID = LPDWORD(lpBuffer + dwOffset);
		lpTitle = (char *)lpBuffer + dwOffset + sizeof(DWORD);
		str.Format("%5u", *lpPID);
		m_list_process_or_windows.InsertItem(i, str);
		m_list_process_or_windows.SetItemText(i, 1, lpTitle);
		m_list_process_or_windows.SetItemText(i, 2, "显示"); //(d) 将窗口状态显示为 "显示"
		// ItemData 为窗口句柄
		m_list_process_or_windows.SetItemData(i, *lpPID);  //(d)
		dwOffset += sizeof(DWORD) + lstrlen(lpTitle) + 1;
	}
	str.Format("窗口名称 / %d", i);
	LVCOLUMN lvc;
	lvc.mask = LVCF_TEXT;
	lvc.pszText = str.GetBuffer(0);
	lvc.cchTextMax = str.GetLength();
	m_list_process_or_windows.SetColumn(1, &lvc);

}


void CSystemDlg::OnRefreshpslist()
{
	// TODO: 在此添加命令处理程序代码
	if (m_list_process_or_windows.IsWindowVisible())
		GetProcessList();
	//if (m_list_windows.IsWindowVisible())
		//GetWindowsList();
}


void CSystemDlg::OnNMRClickListProcess(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CMenu	popup;
	if (m_caseSyetemIs == TOKEN_PSLIST)      //进程管理初始化列表
	{
		popup.LoadMenu(IDR_PSLIST);
	}
	else if (m_caseSyetemIs == TOKEN_WSLIST)
	{
		popup.LoadMenu(IDR_WINDOW_LIST);
	}
	CMenu*	pM = popup.GetSubMenu(0);
	CPoint	p;
	GetCursorPos(&p);

	pM->TrackPopupMenu(TPM_LEFTALIGN, p.x, p.y, this);
	*pResult = 0;
}


void CSystemDlg::OnReceiveComplete(void)
{
	switch (m_pContext->m_DeCompressionBuffer.GetBuffer(0)[0])
	{
	case TOKEN_PSLIST:
		ShowProcessList();
		break;
	case TOKEN_WSLIST:
		ShowWindowsList();
		break;
		//case TOKEN_DIALUPASS:
			//ShowDialupassList();
			//break;
	default:
		// 传输发生异常数据
		break;
	}
}



void CSystemDlg::OnWindowClost()
{
	// TODO: 在此添加命令处理程序代码
	BYTE lpMsgBuf[20];
	CListCtrl	*pListCtrl = NULL;
	pListCtrl = &m_list_process_or_windows;

	int	nItem = pListCtrl->GetSelectionMark();
	if (nItem >= 0)
	{
		ZeroMemory(lpMsgBuf, 20);
		lpMsgBuf[0] = COMMAND_WINDOW_CLOSE;      //注意这个就是我们的数据头
		DWORD hwnd = pListCtrl->GetItemData(nItem); //得到窗口的句柄一同发送
		memcpy(lpMsgBuf + 1, &hwnd, sizeof(DWORD));
		m_iocpServer->Send(m_pContext, lpMsgBuf, sizeof(lpMsgBuf));

	}
}


void CSystemDlg::OnWindowHide()
{
	// TODO: 在此添加命令处理程序代码
	BYTE lpMsgBuf[20];
	CListCtrl	*pListCtrl = NULL;
	pListCtrl = &m_list_process_or_windows;

	int	nItem = pListCtrl->GetSelectionMark();
	if (nItem >= 0)
	{
		ZeroMemory(lpMsgBuf, 20);
		lpMsgBuf[0] = COMMAND_WINDOW_TEST;       //窗口处理数据头
		DWORD hwnd = pListCtrl->GetItemData(nItem);  //得到窗口的句柄一同发送
		pListCtrl->SetItemText(nItem, 2, "隐藏");  //注意这时将列表中的显示状态为"隐藏"
		//这样在删除列表条目时就不删除该项了 如果删除该项窗口句柄会丢失 就永远也不能显示了
		memcpy(lpMsgBuf + 1, &hwnd, sizeof(DWORD));  //得到窗口的句柄一同发送
		DWORD dHow = SW_HIDE;               //窗口处理参数 0
		memcpy(lpMsgBuf + 1 + sizeof(hwnd), &dHow, sizeof(DWORD));
		m_iocpServer->Send(m_pContext, lpMsgBuf, sizeof(lpMsgBuf));
	}
}

void CSystemDlg::OnWindowMax()
{
	// TODO: 在此添加命令处理程序代码
	BYTE lpMsgBuf[20];
	CListCtrl	*pListCtrl = NULL;
	pListCtrl = &m_list_process_or_windows;

	int	nItem = pListCtrl->GetSelectionMark();
	if (nItem >= 0)
	{
		ZeroMemory(lpMsgBuf, 20);
		lpMsgBuf[0] = COMMAND_WINDOW_TEST;     //同上
		DWORD hwnd = pListCtrl->GetItemData(nItem);  //同上
		pListCtrl->SetItemText(nItem, 2, "显示");   //将状态改为显示
		memcpy(lpMsgBuf + 1, &hwnd, sizeof(DWORD));
		DWORD dHow = SW_MAXIMIZE;     //同上
		memcpy(lpMsgBuf + 1 + sizeof(hwnd), &dHow, sizeof(DWORD));
		m_iocpServer->Send(m_pContext, lpMsgBuf, sizeof(lpMsgBuf));

	}
	// TODO: 在此添加命令处理程序代码
}


void CSystemDlg::OnWindowMin()
{
	// TODO: 在此添加命令处理程序代码
	// TODO: 在此添加命令处理程序代码
	BYTE lpMsgBuf[20];
	CListCtrl	*pListCtrl = NULL;
	pListCtrl = &m_list_process_or_windows;

	int	nItem = pListCtrl->GetSelectionMark();
	if (nItem >= 0)
	{
		ZeroMemory(lpMsgBuf, 20);
		lpMsgBuf[0] = COMMAND_WINDOW_TEST;
		DWORD hwnd = pListCtrl->GetItemData(nItem);
		pListCtrl->SetItemText(nItem, 2, "显示");
		memcpy(lpMsgBuf + 1, &hwnd, sizeof(DWORD));
		DWORD dHow = SW_MINIMIZE;
		memcpy(lpMsgBuf + 1 + sizeof(hwnd), &dHow, sizeof(DWORD));
		m_iocpServer->Send(m_pContext, lpMsgBuf, sizeof(lpMsgBuf));

	}
}


void CSystemDlg::OnWindowReturn()
{
	// TODO: 在此添加命令处理程序代码
	BYTE lpMsgBuf[20];
	CListCtrl	*pListCtrl = NULL;
	pListCtrl = &m_list_process_or_windows;

	int	nItem = pListCtrl->GetSelectionMark();
	if (nItem >= 0)
	{
		ZeroMemory(lpMsgBuf, 20);
		lpMsgBuf[0] = COMMAND_WINDOW_TEST;
		DWORD hwnd = pListCtrl->GetItemData(nItem);
		pListCtrl->SetItemText(nItem, 2, "显示");
		memcpy(lpMsgBuf + 1, &hwnd, sizeof(DWORD));
		DWORD dHow = SW_RESTORE;
		memcpy(lpMsgBuf + 1 + sizeof(hwnd), &dHow, sizeof(DWORD));
		m_iocpServer->Send(m_pContext, lpMsgBuf, sizeof(lpMsgBuf));

	}
}


void CSystemDlg::OnWindowReflush()
{
	// TODO: 在此添加命令处理程序代码
	GetWindowsList();
}


void CSystemDlg::GetWindowsList(void)
{
	BYTE bToken = COMMAND_WSLIST;
	m_iocpServer->Send(m_pContext, &bToken, 1);
}
