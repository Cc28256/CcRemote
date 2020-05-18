// CSettingDlg.cpp: 实现文件
//

#include "pch.h"
#include "CcRemote.h"
#include "CSettingDlg.h"
#include "afxdialogex.h"


// CSettingDlg 对话框

IMPLEMENT_DYNAMIC(CSettingDlg, CDialogEx)

CSettingDlg::CSettingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_SETTING, pParent)
	, m_nListernProt(0)
	, m_nMax_Connect(0)
{

}

CSettingDlg::~CSettingDlg()
{
}

void CSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_EDIT_PORT, m_nListernProt);
	DDX_Text(pDX, IDC_EDIT_MAX, m_nMax_Connect);
}


BEGIN_MESSAGE_MAP(CSettingDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSettingDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CSettingDlg 消息处理程序



void CSettingDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码

	UpdateData(TRUE);
	((CCcRemoteApp *)AfxGetApp())->m_IniFile.SetInt("Settings", "ListenPort", m_nListernProt);      //向ini文件中写入值
	((CCcRemoteApp *)AfxGetApp())->m_IniFile.SetInt("Settings", "MaxConnection", m_nMax_Connect);
	MessageBox("设置成功，重启本程序后生效！");

	CDialogEx::OnOK();
}
