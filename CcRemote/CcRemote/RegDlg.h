#pragma once

#include "include/IOCPServer.h"
// CRegDlg 对话框

class CRegDlg : public CDialog
{
	DECLARE_DYNAMIC(CRegDlg)

public:
	CRegDlg(CWnd* pParent = NULL, CIOCPServer* pIOCPServer = NULL, ClientContext *pContext = NULL);   // 标准构造函数
	virtual ~CRegDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_REGEDIT};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

	ClientContext* m_pContext;
	CIOCPServer* m_iocpServer;
	bool isEnable;           //控件是否可用
	HTREEITEM	SelectNode;
public:
	CTreeCtrl m_tree;
	CListCtrl m_list;
	CImageList m_HeadIcon;
protected:
	HTREEITEM	m_hRoot;
	HTREEITEM	HKLM;
	HTREEITEM	HKCR;
	HTREEITEM	HKCU;
	HTREEITEM	HKUS;
	HTREEITEM	HKCC;
	CImageList	m_ImageList_tree;
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnTvnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult);
	CString GetFullPath(HTREEITEM hCurrent);
	char getFatherPath(CString& FullPath);
	void OnReceiveComplete(void);
	void AddKey(char* lpBuffer);
	void AddPath(char* lpBuffer);
};
