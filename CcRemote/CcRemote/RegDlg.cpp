// RegDlg.cpp : 实现文件
//

#include "pch.h"
#include "CcRemote.h"
#include "RegDlg.h"
#include "afxdialogex.h"
#include "..\..\common\macros.h"


// CRegDlg 对话框

IMPLEMENT_DYNAMIC(CRegDlg, CDialog)

	enum MYKEY{
		MHKEY_CLASSES_ROOT,
		MHKEY_CURRENT_USER,
		MHKEY_LOCAL_MACHINE,
		MHKEY_USERS,
		MHKEY_CURRENT_CONFIG
};

enum KEYVALUE{
	MREG_SZ,
	MREG_DWORD,
	MREG_BINARY,
	MREG_EXPAND_SZ
};
struct REGMSG{
	int count;         //名字个数
	DWORD size;             //名字大小
	DWORD valsize;     //值大小

};
CRegDlg::CRegDlg(CWnd* pParent, CIOCPServer* pIOCPServer, ClientContext *pContext)
	: CDialog(IDD_DIALOG_REGEDIT, pParent)
{
	m_iocpServer = pIOCPServer;
	m_pContext = pContext;
}

CRegDlg::~CRegDlg()
{
}

void CRegDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE, m_tree);
	DDX_Control(pDX, IDC_LIST, m_list);
}


BEGIN_MESSAGE_MAP(CRegDlg, CDialog)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE, &CRegDlg::OnTvnSelchangedTree)
END_MESSAGE_MAP()


// CRegDlg 消息处理程序


BOOL CRegDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化

	HICON hIcon = NULL;
	m_ImageList_tree.Create(18, 18, ILC_COLOR16,10, 0);

	//hIcon = (HICON)::LoadImage(::AfxGetInstanceHandle(),MAKEINTRESOURCE(IDI_FATHER_ICON), IMAGE_ICON, 18, 18, 0);
	//m_ImageList_tree.Add(hIcon);
	//hIcon = (HICON)::LoadImage(::AfxGetInstanceHandle(),MAKEINTRESOURCE(IDI_DIR_ICON), IMAGE_ICON, 32, 32, 0);
	//m_ImageList_tree.Add(hIcon);



	m_tree.SetImageList ( &m_ImageList_tree,TVSIL_NORMAL );

	DWORD	dwStyle = GetWindowLong(m_tree.m_hWnd,GWL_STYLE);

	dwStyle |=TVS_HASBUTTONS | TVS_HASLINES;// | TVS_LINESATROOT;这里不可一设置这个样式不然ico图表无法显示

	SetWindowLong(m_tree.m_hWnd,GWL_STYLE,dwStyle);
	 m_hRoot = m_tree.InsertItem("注册表管理",0,0,0,0);   
	HKCU=m_tree.InsertItem("HKEY_CURRENT_USER",1,1,m_hRoot,0);
	HKLM=m_tree.InsertItem("HKEY_LOCAL_MACHINE",1,1,m_hRoot,0);
	HKUS=m_tree.InsertItem("HKEY_USERS",1,1,m_hRoot,0);
	HKCC=m_tree.InsertItem("HKEY_CURRENT_CONFIG",1,1,m_hRoot,0);
	HKCR=m_tree.InsertItem("HKEY_CLASSES_ROOT",1,1,m_hRoot,0);

	m_tree.Expand(m_hRoot,TVE_EXPAND);

	m_list.InsertColumn(0,"名称",LVCFMT_LEFT,150,-1);
	m_list.InsertColumn(1,"类型",LVCFMT_LEFT,60,-1);
	m_list.InsertColumn(2,"数据",LVCFMT_LEFT,300,-1);
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	//////添加图标//////
	//m_HeadIcon.Create(16,16,TRUE,2,2);
	//m_HeadIcon.Add(AfxGetApp()->LoadIcon(IDI_STR_ICON));
	//m_HeadIcon.Add(AfxGetApp()->LoadIcon(IDI_DWORD_ICON));
	//m_list.SetImageList(&m_HeadIcon,LVSIL_SMALL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CRegDlg::OnTvnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	if(!isEnable) return;
	isEnable=false;;
	TVITEM item = pNMTreeView->itemNew;

	if(item.hItem == m_hRoot)
	{
		isEnable=true;;
		return;
	}
	SelectNode=item.hItem;			//保存用户打开的子树节点句柄
	m_list.DeleteAllItems();

	CString FullPath=GetFullPath(SelectNode);
	HTREEITEM CurrentNode =  item.hItem; //取得此节点的全路径

	while(m_tree.GetChildItem(CurrentNode)!=NULL)
	{
		m_tree.DeleteItem(m_tree.GetChildItem(CurrentNode));        //删除 会产生 OnSelchangingTree事件 ***
	}

	char bToken=getFatherPath(FullPath);
	//愈加一个键
	int nitem=m_list.InsertItem(0,"(Data)",0);
	m_list.SetItemText(nitem,1,"REG_SZ");	
	m_list.SetItemText(nitem,2,"(NULL)");
	//BeginWaitCursor(); 
	//char *buf=new char[FullPath.GetLength]
	FullPath.Insert(0,bToken);//插入  那个根键
	bToken=COMMAND_REG_FIND;
	FullPath.Insert(0,bToken);      //插入查询命令

	m_iocpServer->Send(m_pContext, (LPBYTE)(FullPath.GetBuffer(0)), FullPath.GetLength()+1);
	isEnable=true;;
	*pResult = 0;
}


CString CRegDlg::GetFullPath(HTREEITEM hCurrent)
{
	CString strTemp;
	CString strReturn = "";
	while(1){
		if(hCurrent==m_hRoot) return strReturn;
		strTemp = m_tree.GetItemText(hCurrent);   //得到当前的
		if(strTemp.Right(1) != "\\")
			strTemp += "\\";
		strReturn = strTemp  + strReturn;
		hCurrent = m_tree.GetParentItem(hCurrent);   //得到父的

	}
	return strReturn;
}


char CRegDlg::getFatherPath(CString& FullPath)
{
	char bToken;
	if(!FullPath.Find("HKEY_CLASSES_ROOT"))	//判断主键
	{
		//MKEY=HKEY_CLASSES_ROOT;
		bToken=MHKEY_CLASSES_ROOT;
		FullPath.Delete(0,sizeof("HKEY_CLASSES_ROOT"));
	}else if(!FullPath.Find("HKEY_CURRENT_USER"))
	{
		bToken=MHKEY_CURRENT_USER;
		FullPath.Delete(0,sizeof("HKEY_CURRENT_USER"));

	}else if(!FullPath.Find("HKEY_LOCAL_MACHINE"))
	{
		bToken=MHKEY_LOCAL_MACHINE;
		FullPath.Delete(0,sizeof("HKEY_LOCAL_MACHINE"));

	}else if(!FullPath.Find("HKEY_USERS"))
	{
		bToken=MHKEY_USERS;
		FullPath.Delete(0,sizeof("HKEY_USERS"));

	}else if(!FullPath.Find("HKEY_CURRENT_CONFIG"))
	{
		bToken=MHKEY_CURRENT_CONFIG;
		FullPath.Delete(0,sizeof("HKEY_CURRENT_CONFIG"));

	}
	return bToken;
}


void CRegDlg::OnReceiveComplete(void)
{
	switch (m_pContext->m_DeCompressionBuffer.GetBuffer(0)[0])
	{
	case TOKEN_REG_PATH:                //接收项
		AddPath((char*)(m_pContext->m_DeCompressionBuffer.GetBuffer(1)));
		break;
	case TOKEN_REG_KEY:             //接收键 ，值
		AddKey((char*)(m_pContext->m_DeCompressionBuffer.GetBuffer(1)));
		break;
	default:
		break;
	}
}


void CRegDlg::AddKey(char* lpBuffer)
{
	m_list.DeleteAllItems();
	int nitem=m_list.InsertItem(0,"(Data)",0);
	m_list.SetItemText(nitem,1,"REG_SZ");	
	m_list.SetItemText(nitem,2,"(NULL)");

	if(lpBuffer==NULL) return;
	REGMSG msg;
	memcpy((void*)&msg,lpBuffer,sizeof(msg));
	char* tmp=lpBuffer+sizeof(msg);

	int bin_temp;
	for(int i=0;i<msg.count;i++)
	{
		BYTE Type=tmp[0];   //取出标志头
		tmp+=sizeof(BYTE);
		char* szValueName=tmp;   //取出名字
		tmp+=msg.size;
		BYTE* szValueDate=(BYTE*)tmp;      //取出值
		tmp+=msg.valsize;
		if(Type==MREG_SZ)
		{
			int nitem=m_list.InsertItem(0,szValueName,0);
			m_list.SetItemText(nitem,1,"REG_SZ");	
			m_list.SetItemText(nitem,2,(char*)szValueDate);
		}
		if(Type==MREG_DWORD)
		{
			char ValueDate[256];
			DWORD d=(DWORD)szValueDate;
			memcpy((void*)&d,szValueDate,sizeof(DWORD));
			CString value;
			value.Format("0x%08x",d);
			sprintf(ValueDate," (%d)",d);
			value+=" ";
			value+=ValueDate;
			int nitem=m_list.InsertItem(0,szValueName,1);
			m_list.SetItemText(nitem,1,"REG_DWORD");	
			m_list.SetItemText(nitem,2,value);

		}
		if(Type==MREG_BINARY)
		{
			//不能申请内存来动态操作获取，不清楚原因，只要是动态获取的话就会造成崩溃，所以上限0x1000大小不过应该也够用了
			int i = 0;
			int index_max = msg.valsize;
			char ValueDate[0x1000] = { 0 };
			if (msg.valsize > 0x1000)
			{
				index_max = 0x990;//incomplete
			}
			for (i = 0; i < index_max; i++)
			{
				bin_temp = szValueDate[i];
				sprintf(ValueDate +(i*3), "%02x ", bin_temp);
			}
			CString value = ValueDate;
			if (index_max == 0x990)
			{
				value+=" incomplete";
			}
			
			int nitem=m_list.InsertItem(0,szValueName,1);
			m_list.SetItemText(nitem,1,"REG_BINARY");	
			m_list.SetItemText(nitem,2, value);
		}
		if(Type==MREG_EXPAND_SZ)
		{
			int nitem=m_list.InsertItem(0,szValueName,0);
			m_list.SetItemText(nitem,1,"REG_EXPAND_SZ");	
			m_list.SetItemText(nitem,2,(char*)szValueDate);
		}
	}
}


void CRegDlg::AddPath(char* lpBuffer)
{
	if(lpBuffer==NULL) return;
	int msgsize=sizeof(REGMSG);
	REGMSG msg;
	memcpy((void*)&msg,lpBuffer,msgsize);
	DWORD size =msg.size;
	int count=msg.count;

	if(size>0&&count>0){                   //一点保护措施
		for(int i=0;i<count;i++){
			char* szKeyName=lpBuffer+size*i+msgsize;
			m_tree.InsertItem(szKeyName,1,1,SelectNode,0);//插入子键名称
			m_tree.Expand(SelectNode,TVE_EXPAND);
		}
	}
}
