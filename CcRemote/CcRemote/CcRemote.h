
// CcRemote.h: PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// 主符号
#include "IniFile.h"

// CCcRemoteApp:
// 有关此类的实现，请参阅 CcRemote.cpp
//

class CCcRemoteApp : public CWinApp
{
public:
	CCcRemoteApp();

	CIniFile m_IniFile;//配置文件对象
// 重写
public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern CCcRemoteApp theApp;
