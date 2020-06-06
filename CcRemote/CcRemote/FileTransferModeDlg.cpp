// CInputDlg.cpp: 实现文件
//

#include "pch.h"
#include "CcRemote.h"
#include "FileTransferModeDlg.h"
#include "afxdialogex.h"


// CFileTransferModeDlg 对话框

IMPLEMENT_DYNAMIC(CFileTransferModeDlg, CDialog)

CFileTransferModeDlg::CFileTransferModeDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_TRANSFERMODE_DLG, pParent)
{

}

CFileTransferModeDlg::~CFileTransferModeDlg()
{
}

void CFileTransferModeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CFileTransferModeDlg, CDialog)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_OVERWRITE, IDC_CANCEL, OnEndDialog)
END_MESSAGE_MAP()


// CFileTransferModeDlg 消息处理程序


BOOL CFileTransferModeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	CString	str;
	str.Format("此文件夹已包含一个名为“%s”的文件", m_strFileName);

	for (int i = 0; i < str.GetLength(); i += 120)
	{
		str.Insert(i, "\n");
		i += 1;
	}

	SetDlgItemText(IDC_TIPS, str);
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


//重写这个函数是因为让继承他的子类能够重载这个函数来判断id吧
void CFileTransferModeDlg::OnEndDialog(UINT id)
{
	// TODO: Add your control notification handler code here
	EndDialog(id);
}