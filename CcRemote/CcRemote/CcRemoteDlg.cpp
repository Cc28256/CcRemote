
// CcRemoteDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "CcRemote.h"
#include "CcRemoteDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif





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
}

void CCcRemoteDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ONLINE, m_CList_Online);
	DDX_Control(pDX, IDC_MESSAGE, m_CList_Message);
}

BEGIN_MESSAGE_MAP(CCcRemoteDlg, CDialogEx)
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
END_MESSAGE_MAP()


// CCcRemoteDlg 消息处理程序

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


	InitList();//初始化列表控件
	//---------改变窗口大小出发动态调整-------|
	CRect rect;
	GetWindowRect(&rect);
	rect.bottom += 20;
	MoveWindow(rect);
	//----------------------------------------|
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
		rc.bottom = cy - 6;		//列表的下坐标
		m_CList_Message.MoveWindow(rc);

		for (int i = 0; i < COLUMN_MESSAGE_COUNT; i++) {                   //遍历每一个列
			double dd = m_Column_Message_Data[i].nWidth;     //得到当前列的宽度
			dd /= m_Column_Message_Width;                    //看一看当前宽度占总长度的几分之几
			dd *= dcx;                                       //用原来的长度乘以所占的几分之几得到当前的宽度
			int lenth = dd;                                   //转换为int 类型
			m_CList_Message.SetColumnWidth(i, (lenth));        //设置当前的宽度
		}
	}
	// TODO: 在此处添加消息处理程序代码
}

int CCcRemoteDlg::InitList()
{
	m_CList_Online.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_CList_Message.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	
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


void CCcRemoteDlg::AddList(CString strIP, CString strAddr, CString strPCName, CString strOS, CString strCPU, CString strVideo, CString strPing)
{
	m_CList_Online.InsertItem(0, strIP);           //默认为0行  这样所有插入的新列都在最上面
	m_CList_Online.SetItemText(0, ONLINELIST_ADDR, strAddr);      //设置列的显示字符   这里 ONLINELIST_ADDR等 为第二节课中的枚举类型 用这样的方法
	m_CList_Online.SetItemText(0, ONLINELIST_COMPUTER_NAME, strPCName); //解决问题会避免以后扩展时的冲突
	m_CList_Online.SetItemText(0, ONLINELIST_OS, strOS);
	m_CList_Online.SetItemText(0, ONLINELIST_CPU, strCPU);
	m_CList_Online.SetItemText(0, ONLINELIST_VIDEO, strVideo);
	m_CList_Online.SetItemText(0, ONLINELIST_PING, strPing);
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
}


void CCcRemoteDlg::Test()
{
	AddList("192.168.0.1", "本机局域网", "CHANG", "Windows7", "2.2GHZ", "有", "123232");
	AddList("192.168.10.1", "本机局域网", "WANG", "Windows10", "2.2GHZ", "无", "111111");
	AddList("192.168.18.25", "本机局域网", "LIU", "Windows8", "2.2GHZ", "有", "654321");
	AddList("192.168.97.162", "本机局域网", "SHANG", "WindowsXP", "2.2GHZ", "无", "123456");
	ShowMessage(true, "软件初始化成功...");
}

void CCcRemoteDlg::OnNMRClickOnline(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CMenu	popup;//声明一个菜单变量
	popup.LoadMenu(IDR_MENU_ONLINE);//载入菜单资源
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
