#pragma once


// CBuildDlg 对话框

class CBuildDlg : public CDialog
{
	DECLARE_DYNAMIC(CBuildDlg)

public:
	CBuildDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CBuildDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_BUILD };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	// IP变量
	CString m_strIP;
	// 端口变量
	CString m_strPort;
private:
	int memfind(const char *mem, const char *str, int sizem, int sizes);
	CString FindFiles(const char* dir, BYTE *lpBuffer, DWORD lpSize);
};
