#pragma once


#include <time.h> 
#include <windows.h> 
#include <iostream>
#include <stdio.h>  
#include <tchar.h>  
#include <fstream>
#include <queue>
#include <vector>
using namespace std;


class CRegOperate
{
public:
	CRegOperate(HKEY beginKey, TCHAR* path);
	~CRegOperate();

private:
#define MAX_KEY_LENGTH 255  
#define MAX_VALUE_NAME 16383  

	DWORD dwType;
	queue<string> keystack;
	
	//²éÑ¯¼üÖµ
	void Query(HKEY rootKey, const char* path);
public:
	vector<string> vecKeyName;
};

