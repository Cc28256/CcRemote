#include "StrCry.h"
#include "pch.h"
//返回的指针如果不用需要释放掉
char* decodeStr(char* str)
{
	int len = str[0];
	char * uncodeStr = (char *)operator new(len + 1);
	for (size_t i = 1; i <= len; i++)
	{
		uncodeStr[i - 1] = str[i] ^ (0xCC - i);
	}
	uncodeStr[len] = 0x00;
	return uncodeStr;
}
