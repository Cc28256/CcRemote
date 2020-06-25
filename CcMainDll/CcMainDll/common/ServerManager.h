// ServerManager.h: interface for the CServerManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVERMANAGER_H__BAE27F8C_8A1C_4D5B_89F6_FA138B65470E__INCLUDED_)
#define AFX_SERVERMANAGER_H__BAE27F8C_8A1C_4D5B_89F6_FA138B65470E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Manager.h"

class CServerManager : public CManager  
{
public:
	CServerManager(CClientSocket *pClient);
	virtual ~CServerManager();

protected:
	void SendServicesList();
	LPBYTE getServerList();
};

#endif // !defined(AFX_SERVERMANAGER_H__BAE27F8C_8A1C_4D5B_89F6_FA138B65470E__INCLUDED_)
