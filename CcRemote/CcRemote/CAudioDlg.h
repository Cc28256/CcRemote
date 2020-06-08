#pragma once


// CAudioDlg 对话框
#include "include/IOCPServer.h"
#include "..\..\common\Audio.h"
class CAudioDlg : public CDialog
{
	DECLARE_DYNAMIC(CAudioDlg)

public:
	CAudioDlg(CWnd* pParent = NULL, CIOCPServer* pIOCPServer = NULL, ClientContext *pContext = NULL);   // 标准构造函数
	virtual ~CAudioDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_AUDIO };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	bool m_bIsWorking;
	CAudio m_Audio;
private:

	UINT m_nTotalRecvBytes;
	HICON m_hIcon;
	HANDLE	m_hWorkThread;
	ClientContext* m_pContext;
	CIOCPServer* m_iocpServer;
	CString m_IPAddress;
public:
	virtual BOOL OnInitDialog();
	static DWORD  WorkThread(LPVOID lparam);
	BOOL m_bIsSendLocalAudio;
	afx_msg void OnClose();
	void OnReceiveComplete(void);
};
