#include "CRegOperate.h"



CRegOperate::CRegOperate(HKEY beginKey, TCHAR* path)
{

	dwType = REG_BINARY | REG_DWORD | REG_EXPAND_SZ | REG_MULTI_SZ | REG_NONE | REG_SZ;


	//Begin to get HKEY of path.
	Query(beginKey, path);

	//test if queue is empty测试队列是否为空
	while (!keystack.empty())
	{

		string newPath = keystack.front();

		keystack.pop();

		Query(beginKey, newPath.c_str());

	}

	//Release.
	RegCloseKey(beginKey);
}


CRegOperate::~CRegOperate()
{


}




void CRegOperate::Query(HKEY rootKey, const char* path)

{
	vecKeyName.clear();
	vecKeyName.reserve(200);

#ifdef COMMAND_OUTPUT

	_tprintf(TEXT("\nProcess: %s :\n"), path);

#endif

	HKEY hKey;
	if (RegOpenKeyEx(rootKey, path, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return;
	}



	TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name  
	DWORD    cbName;                   // size of name string   
	TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name   
	DWORD    cchClassName = MAX_PATH;  // size of class string   
	DWORD    cSubKeys = 0;               // number of subkeys   
	DWORD    cbMaxSubKey;              // longest subkey size   
	DWORD    cchMaxClass;              // longest class string   
	DWORD    cValues;              // number of values for key   
	DWORD    cchMaxValue;          // longest value name   
	DWORD    cbMaxValueData;       // longest value data   
	DWORD    cbSecurityDescriptor; // size of security descriptor   
	FILETIME ftLastWriteTime;      // last write time   

	DWORD i, retCode;


	TCHAR  achValue[MAX_VALUE_NAME];
	DWORD cchValue = MAX_VALUE_NAME;


	// Get the class name and the value count.   

	retCode = RegQueryInfoKey(
		hKey,                    // key handle   
		achClass,                // buffer for class name   
		&cchClassName,           // size of class string   
		NULL,                    // reserved   
		&cSubKeys,               // number of subkeys   
		&cbMaxSubKey,            // longest subkey size   
		&cchMaxClass,            // longest class string   
		&cValues,                // number of values for this key   
		&cchMaxValue,            // longest value name   
		&cbMaxValueData,         // longest value data   
		&cbSecurityDescriptor,   // security descriptor   
		&ftLastWriteTime);       // last write time   



	// Enumerate the subkeys, until RegEnumKeyEx fails.  

	if (cSubKeys)
	{

#ifdef COMMAND_OUTPUT

		printf("Number of subkeys: %d\n", cSubKeys);

#endif

		for (i = 0; i < cSubKeys; i++)
		{
			cbName = MAX_KEY_LENGTH;

			retCode = RegEnumKeyEx(hKey, i,
				achKey,
				&cbName,
				NULL,
				NULL,
				NULL,
				&ftLastWriteTime);

			if (retCode == ERROR_SUCCESS)
			{

#ifdef COMMAND_OUTPUT

				_tprintf(TEXT("(%d) %s\n"), i + 1, achKey);

#endif

				//use achKey to build new path and input it into stack.

				std::string newPath = "";

				newPath.append(path);
				newPath.append("\\");
				newPath.append(achKey);
				keystack.push(newPath);

			}
		}
	}

	// Enumerate the key values.   

	if (cValues)
	{
#ifdef COMMAND_OUTPUT

		printf("Number of values: %d\n", cValues);

#endif

		for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++)
		{
			cchValue = MAX_VALUE_NAME;

			achValue[0] = '\0';

			unsigned char vari[70];

			retCode = RegEnumValue(hKey, i,
				achValue,
				&cchValue,
				NULL,
				NULL,
				NULL,
				NULL);

			if (retCode == ERROR_SUCCESS)

			{
				std::string filePath = "";
				TCHAR szBuffer[255] = { 0 };
				DWORD dwNameLen = 255;
				DWORD rQ = RegQueryValueEx(hKey, achValue, 0, &dwType, (LPBYTE)szBuffer, &dwNameLen);

				if (rQ == ERROR_SUCCESS)
				{
					filePath.append(szBuffer);
					//获取到的键值保存在vector中
					vecKeyName.push_back(filePath);
					//_tprintf(TEXT("(%d) %s %s\n"), i + 1, achValue, szBuffer);

				}
			}
		}
	}
	//release.
	RegCloseKey(hKey);
}

