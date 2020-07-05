#pragma once


// CHelpWindow 对话框

class CHelpWindow : public CDialogEx
{
	DECLARE_DYNAMIC(CHelpWindow)

public:
	CHelpWindow(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CHelpWindow();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_HELP };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CStatic				m_PicHelp;
	afx_msg void OnStnClickedStaticHelp();
};
