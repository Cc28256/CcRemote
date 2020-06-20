#include "..\pch.h"
#include <windows.h>
#include "..\StrCry.h"
//去除字符串类型前面的空格
char *DelSpace(char *szData)
{
	int i=0 ;
	while(1)
	{
		if(strnicmp(szData+i," ",1))
			break;
		i++;			
	}
	return (szData+i);
} 

//设置注册表键读取的权限(KEY_READ||KEY_WRITE||KEY_ALL_ACCESS)
int SetKeySecurityEx(HKEY MainKey,LPCTSTR SubKey,DWORD security) 
{    
   HKEY  hKey; 
   SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY; 
   PSID pSystemSid              = NULL; 
   PSID pUserSid                = NULL; 
   SECURITY_DESCRIPTOR sd; 
   PACL    pDacl                = NULL; 
   DWORD   dwAclSize; 
   int     iResult              = 0;

   __try
   {  	   
	   if(RegOpenKeyEx(MainKey, SubKey, 0, WRITE_DAC, &hKey)!= ERROR_SUCCESS) 
		   __leave; 
       if(!AllocateAndInitializeSid(&sia,1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &pSystemSid )) 
           __leave;
       if(!AllocateAndInitializeSid( &sia, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,0, 0, 0, 0, 0, 0, &pUserSid))  
           __leave; 
       dwAclSize = sizeof(ACL) + 2 * ( sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) ) + GetLengthSid(pSystemSid) + GetLengthSid(pUserSid) ; 
       pDacl = (PACL)HeapAlloc(GetProcessHeap(), 0, dwAclSize); 
       if(pDacl == NULL) 
		   __leave; 
       if(!InitializeAcl(pDacl, dwAclSize, ACL_REVISION)) 
           __leave; 
       if(!AddAccessAllowedAce( pDacl, ACL_REVISION, KEY_ALL_ACCESS, pSystemSid )) 
           __leave; 
       if(!AddAccessAllowedAce( pDacl, ACL_REVISION, (unsigned long)security, pUserSid )) 
           __leave; 
       if(!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) 
           __leave; 
       if(!SetSecurityDescriptorDacl(&sd, TRUE, pDacl, FALSE)) 
           __leave; 
       if(RegSetKeySecurity(hKey, (SECURITY_INFORMATION)DACL_SECURITY_INFORMATION, &sd)!= ERROR_SUCCESS)
		   __leave;
	   iResult =1;
   }
   __finally
   {  
	   RegCloseKey(MainKey); 
	   RegCloseKey(hKey); 
	   
	   if(pDacl !=NULL)         
		   HeapFree(GetProcessHeap(), 0, pDacl);  
       if(pSystemSid !=NULL)
	       FreeSid(pSystemSid);
	   if(pUserSid !=NULL)
           FreeSid(pUserSid); 
   }

   return iResult;
}
//读取注册表的指定键的数据(Mode:0-读键值数据 1-牧举子键 2-牧举指定键项 3-判断该键是否存在)
int  ReadRegEx(HKEY MainKey,LPCTSTR SubKey,LPCTSTR Vname,DWORD Type,char *szData,LPBYTE szBytes,DWORD lbSize,int Mode)
{   
	//strcry -----------------------
	char* pDecodeStr;
	char char_REG_SZ[] = { 0x06,0x99,0x8f,0x8e,0x97,0x94,0x9c };	//REG_SZ
	char char_REG_EXPAND_SZ[] = { 0x0d,0x99,0x8f,0x8e,0x97,0x82,0x9e,0x95,0x85,0x8d,0x86,0x9e,0x93,0xe5 };	//REG_EXPAND_SZ
	char char_REG_BINARY[] = { 0x0a,0x99,0x8f,0x8e,0x97,0x85,0x8f,0x8b,0x85,0x91,0x9b };	//REG_BINARY
	char char_REG_MULTI_SZ[] = { 0x0c,0x99,0x8f,0x8e,0x97,0x8a,0x93,0x89,0x90,0x8a,0x9d,0x92,0x9a };	//REG_MULTI_SZ
	char char_REG_DWORD[] = { 0x09,0x99,0x8f,0x8e,0x97,0x83,0x91,0x8a,0x96,0x87 };	//REG_DWORD
	//------------------------------
	
	HKEY   hKey;  
	int    ValueDWORD,iResult=0;
	char*  PointStr;  
	char   KeyName[32],ValueSz[MAX_PATH],ValueTemp[MAX_PATH];	
	DWORD  szSize,KnSize,dwIndex=0;	 

	memset(KeyName,0,sizeof(KeyName));
	memset(ValueSz,0,sizeof(ValueSz));
	memset(ValueTemp,0,sizeof(ValueTemp));
	 
	__try
	{
	//	 SetKeySecurityEx(MainKey,SubKey,KEY_ALL_ACCESS);
		if(RegOpenKeyEx(MainKey,SubKey,0,KEY_READ,&hKey) != ERROR_SUCCESS)
		{
            iResult = -1;
			__leave;
		}
		switch(Mode)		 
		{
		case 0:
			switch(Type)
			{
			case REG_SZ:
			case REG_EXPAND_SZ:				 
				szSize = sizeof(ValueSz);
				if(RegQueryValueEx(hKey,Vname,NULL,&Type,(LPBYTE)ValueSz,&szSize) == ERROR_SUCCESS)
				{
					strcpy(szData,DelSpace(ValueSz));
					iResult =1;
				}
				break;
			case REG_MULTI_SZ:	
				szSize = sizeof(ValueSz);
				if(RegQueryValueEx(hKey,Vname,NULL,&Type,(LPBYTE)ValueSz,&szSize) == ERROR_SUCCESS)
				{
					for(PointStr = ValueSz; *PointStr; PointStr = strchr(PointStr,0)+1)
					{
					
						strncat(ValueTemp,PointStr,sizeof(ValueTemp));
					    strncat(ValueTemp," ",sizeof(ValueTemp));
					}
					strcpy(szData,ValueTemp);
					iResult =1;
				}
				break;				 			
			case REG_DWORD:
				szSize = sizeof(DWORD);
				if(RegQueryValueEx(hKey,Vname,NULL,&Type,(LPBYTE)&ValueDWORD,&szSize ) == ERROR_SUCCESS)						
				{
					wsprintf(szData,"%d",ValueDWORD);
					iResult =1;
				}
				break;
            case REG_BINARY:
                szSize = lbSize;
				if(RegQueryValueEx(hKey,Vname,NULL,&Type,szBytes,&szSize) == ERROR_SUCCESS)
					iResult =1;
				break;
			}
			break;
		case 1:
			while(1)
			{				 
				memset(ValueSz,0,sizeof(ValueSz));
				szSize   = sizeof(ValueSz);
                if(RegEnumKeyEx(hKey,dwIndex++,ValueSz,&szSize,NULL,NULL,NULL,NULL) != ERROR_SUCCESS)
					break;
                wsprintf(ValueTemp,"[%s]\r\n",ValueSz);
				strcat(szData,ValueTemp);
				iResult =1;
			}			 
			break;
		case 2:			  
			while(1)
			{				 
				memset(KeyName,0,sizeof(KeyName));
				memset(ValueSz,0,sizeof(ValueSz));
				memset(ValueTemp,0,sizeof(ValueTemp));
				KnSize = sizeof(KeyName);
                szSize = sizeof(ValueSz);
                if(RegEnumValue(hKey,dwIndex++,KeyName,&KnSize,NULL,&Type,(LPBYTE)ValueSz,&szSize) != ERROR_SUCCESS)
					break;
				switch(Type)				 				
				{				     
				case REG_SZ:	
					pDecodeStr = decodeStr(char_REG_SZ);							//解密函数

					wsprintf(ValueTemp,"%-24s  %-15s %s \r\n",KeyName, pDecodeStr,ValueSz);

					memset(pDecodeStr, 0, char_REG_SZ[STR_CRY_LENGTH]);					//填充0
					delete pDecodeStr;
					pDecodeStr = NULL;
					break;
				case REG_EXPAND_SZ:    
					pDecodeStr = decodeStr(char_REG_EXPAND_SZ);

					wsprintf(ValueTemp,"%-24s  %-15s %s \r\n",KeyName, pDecodeStr,ValueSz);

					memset(pDecodeStr, 0, char_REG_EXPAND_SZ[STR_CRY_LENGTH]);					//填充0
					delete pDecodeStr;
					pDecodeStr = NULL;
					break;
				case REG_DWORD:
					pDecodeStr = decodeStr(char_REG_DWORD);

					wsprintf(ValueTemp,"%-24s  %-15s 0x%x(%d) \r\n",KeyName, pDecodeStr,ValueSz,int(ValueSz));

					memset(pDecodeStr, 0, char_REG_DWORD[STR_CRY_LENGTH]);					//填充0
					delete pDecodeStr;
					pDecodeStr = NULL;

					break;
				case REG_MULTI_SZ:
					pDecodeStr = decodeStr(char_REG_MULTI_SZ);

                    wsprintf(ValueTemp,"%-24s  %-15s \r\n",KeyName, pDecodeStr);

					memset(pDecodeStr, 0, char_REG_MULTI_SZ[STR_CRY_LENGTH]);					//填充0
					delete pDecodeStr;
					pDecodeStr = NULL;
					break;
			    case REG_BINARY:
					pDecodeStr = decodeStr(char_REG_BINARY);

					wsprintf(ValueTemp,"%-24s  %-15s \r\n",KeyName, pDecodeStr);

					memset(pDecodeStr, 0, char_REG_BINARY[STR_CRY_LENGTH]);					//填充0
					delete pDecodeStr;
					pDecodeStr = NULL;

					break;
				}
				lstrcat(szData,ValueTemp);
				iResult =1;
			}
			break;
		case 3:
            iResult =1;
			break;
		}
	}
	__finally
	{
        RegCloseKey(MainKey);
		RegCloseKey(hKey);
	}
     
	return iResult;
}
//写注册表的指定键的数据(Mode:0-新建键数据 1-设置键数据 2-删除指定键 3-删除指定键项)
int WriteRegEx(HKEY MainKey,LPCTSTR SubKey,LPCTSTR Vname,DWORD Type,char* szData,DWORD dwData,int Mode)
{
	HKEY  hKey; 
	DWORD dwDisposition;    
	int   iResult =0;
	
	__try
	{	
	//	SetKeySecurityEx(MainKey,Subkey,KEY_ALL_ACCESS);
		switch(Mode)		
		{			
		case 0:
			if(RegCreateKeyEx(MainKey,SubKey,0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,&dwDisposition) != ERROR_SUCCESS)
				__leave;		 
		case 1:	
			if(RegOpenKeyEx(MainKey,SubKey,0,KEY_READ|KEY_WRITE,&hKey) != ERROR_SUCCESS)					 
				__leave;		 		 			 
			switch(Type)
			{		 
			case REG_SZ:		 
			case REG_EXPAND_SZ:			 			 
				if(RegSetValueEx(hKey,Vname,0,Type,(LPBYTE)szData,strlen(szData)+1) == ERROR_SUCCESS) 				 
					iResult =1;				 			
				break;
		    case REG_DWORD:
                if(RegSetValueEx(hKey,Vname,0,Type,(LPBYTE)&dwData,sizeof(DWORD)) == ERROR_SUCCESS)  
		            iResult =1;				 			 
			    break;
		    case REG_BINARY:
			    break;
			}
			break;				
		case 2:
            if(RegOpenKeyEx(MainKey,SubKey,NULL,KEY_READ|KEY_WRITE,&hKey) != ERROR_SUCCESS)				
				__leave;                
			if (RegDeleteKey(hKey,Vname) == ERROR_SUCCESS)		        
				iResult =1;
			break;		
		case 3:
            if(RegOpenKeyEx(MainKey,SubKey,NULL,KEY_READ|KEY_WRITE,&hKey) != ERROR_SUCCESS)				
				__leave;                
			if (RegDeleteValue(hKey,Vname) == ERROR_SUCCESS)		        
				iResult =1;
			break;
		}
	}
	__finally 
	{
	   RegCloseKey(MainKey);		
	   RegCloseKey(hKey);
	}
	return iResult;
}