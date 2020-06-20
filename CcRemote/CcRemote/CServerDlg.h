#pragma once
#include "include/IOCPServer.h"
// CServerDlg 对话框

class CServerDlg : public CDialog
{
	DECLARE_DYNAMIC(CServerDlg)

public:
	CServerDlg(CWnd* pParent = NULL, CIOCPServer* pIOCPServer = NULL, ClientContext *pContext = NULL);   // 标准构造函数
	virtual ~CServerDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERVERDLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()private:
		CListCtrl m_list;
		ClientContext* m_pContext;
		CIOCPServer* m_iocpServer;
public:
	virtual BOOL OnInitDialog();
protected:
	int ShowServiceList(void);
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNMRClickList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnServerRefubish();
	void OnReceiveComplete(void);
	void ServiceConfig(BYTE bCmd);
	afx_msg void OnServerStart();
	afx_msg void OnServerStop();
	afx_msg void OnServerAuto();
	afx_msg void OnServerManual();
};
