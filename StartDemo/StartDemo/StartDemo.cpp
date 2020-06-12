

#include <time.h> 

#include <windows.h> 

#include <iostream>

#include <stdio.h>  

#include <tchar.h>  

#include <fstream>

#include <queue>


#include  <io.h>

#include "CRegOperate.h"


int _tmain(int argc, _TCHAR* argv[])

{
	
	//64位注册表
	TCHAR N1path[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
	//32位注册表
	TCHAR N3path[] = "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run";
	
	TCHAR N2path[] = "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run";
	//HKEY_CURRENT_USER
	CRegOperate test_reg(HKEY_CURRENT_USER, N1path);
	//HKEY_CURRENT_USER

	char* a;
	FILE *p2file;
	for (int i = 0; i < test_reg.vecKeyName.size(); i++) {
	
		a = (char*)(test_reg.vecKeyName[i].c_str());
		if (a[0] == 0x22)
		{
			a = a + 1;
			for (size_t i = 0; i < strlen(a); i++)
			{
				if (a[i] == 0x22)
				{
					a[i] = 0x00;
					a[i+1] = 0x00;					
					break;
				}
			}
			
			if (_access(a, 0)==0)
			{
				//rename(a, newname);
			}
		}
		
	}

	//int _access(const char *pathname, int mode);位于头文件<io.h>中
	/*
	返回0存在 -1不存在
	mode的值和含义如下所示：
	00——只检查文件是否存在
	02——写权限
	04——读权限
	06——读写权限
	*/

	char s[] = "C:\\Program Files\\Realtek\\Audio\\HDA\\RtkAudioService64.exe";
	if (_access(s, 0)==0)
	{
		_tprintf(TEXT("%s，%s\n"), s, "存在");
	}

	system("pause");

	return 0;

}
