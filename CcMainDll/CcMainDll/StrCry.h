#pragma once

//返回的指针如果不用需要释放掉
//加密算法
//char* encryptionStr(char* str)
//{
//	int len = strlen(str);
//	char * a = (char *)operator new(len + 1);
//	a[0] = len;
//	for (size_t i = 1; i <= len; i++)
//	{
//		a[i] = str[i - 1] ^ (0xCC - i);
//	}
//	return a;
//}

#define STR_CRY_LENGTH			0	//加密字符串的长度

//解密算法
char* decodeStr(char* str);



//char 加密字符串[] = { 0x07,0xbc,0xa3,0xa7,0xbb,0xb3,0xa7,0xf5 };	//winsta0
//char* 解密出来的字符串指针 = decodeStr(winsta0);					//解密函数

//memset(lpszWinSta, 0, winsta0[STR_CRY_LENGTH]);					//填充0
//delete lpszWinSta;