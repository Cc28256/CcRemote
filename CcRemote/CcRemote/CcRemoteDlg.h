
// CcRemoteDlg.h: 头文件
//
#include "PublicStruct.h"
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
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_CList_Online;//在线列表变量
	CListCtrl m_CList_Message;//消息列表变量
	afx_msg void OnSize(UINT nType, int cx, int cy);


private:
	//--------------主界面列表的显示变量及常量----------------
#define COLUMN_ONLINE_COUNT 7	//在线列表的个数
#define COLUMN_MESSAGE_COUNT 3	//消息列表的个数
	int m_Column_Online_Width = 0;		//在线列表宽度和
	int m_Column_Message_Width = 0;		//消息列表的宽度和

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
	int InitList();//初始化list控件信息
	void AddList(CString strIP, CString strAddr, CString strPCName, CString strOS, CString strCPU, CString strVideo, CString strPing);
	void ShowMessage(bool bIsOK, CString strMsg);
	void Test();
public:
	afx_msg void OnNMRClickOnline(NMHDR *pNMHDR, LRESULT *pResult);
};
