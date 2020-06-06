#pragma once

#include "resource.h"
// CFileTransferModeDlg 对话框

class CFileTransferModeDlg : public CDialog
{
	DECLARE_DYNAMIC(CFileTransferModeDlg)

public:
	CString m_strFileName;
	CFileTransferModeDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CFileTransferModeDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TRANSFERMODE_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	afx_msg	void OnEndDialog(UINT id);
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	
};
