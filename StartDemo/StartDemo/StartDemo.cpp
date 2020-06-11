

#include <time.h> 

#include <windows.h> 

#include <iostream>

#include <stdio.h>  

#include <tchar.h>  

#include <fstream>

#include <queue>


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


	for (int i = 0; i < test_reg.vecKeyName.size(); i++) {
		_tprintf(TEXT("%s\n"),test_reg.vecKeyName[i].c_str());
	}

	system("pause");

	return 0;

}
