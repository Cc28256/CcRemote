// CHelpWindow.cpp: 实现文件
//

#include "pch.h"
#include "CcRemote.h"
#include "CHelpWindow.h"
#include "afxdialogex.h"


// CHelpWindow 对话框

IMPLEMENT_DYNAMIC(CHelpWindow, CDialogEx)

CHelpWindow::CHelpWindow(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_HELP, pParent)
{

}

CHelpWindow::~CHelpWindow()
{
}

void CHelpWindow::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_HELP, m_PicHelp);
}


BEGIN_MESSAGE_MAP(CHelpWindow, CDialogEx)
	ON_STN_CLICKED(IDC_STATIC_HELP, &CHelpWindow::OnStnClickedStaticHelp)
END_MESSAGE_MAP()


// CHelpWindow 消息处理程序



void CHelpWindow::OnStnClickedStaticHelp()
{
	// TODO: 在此添加控件通知处理程序代码
	ShellExecute(NULL, "open", "explorer.exe", "https://github.com/Cc28256/CcRemote", NULL, SW_SHOW);
}
