// stringCryDemo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

char* crycode(char* str)
{
	printf("cry: %s \n", str);
	int len = strlen(str);
	char * a = (char *)operator new(len + 1);
	a[0] = len;
	printf("0x%02hhx,", a[0]);
	for (size_t i = 1; i <= len; i++)
	{
		a[i] = str[i - 1] ^ (0xCC - i);
		printf("0x%02hhx,", a[i]);
	}
	printf("\n");
	return a;
}


char* uncode(char* str)
{
	int len = str[0];
	char * uncodeStr = (char *)operator new(len + 1);
	for (size_t i = 1; i <= len; i++)
	{
		uncodeStr[i - 1] = str[i] ^ (0xCC - i);
		printf("%c", uncodeStr[i - 1]);
	}
	uncodeStr[len] = 0x00;
	printf("\n");
	return uncodeStr;
}




int main()
{
	char a[] = "#32770";
	char b[] = "VideoCapWindow";
	char c[] = "LyxInstaller.exe";
	char d[] = "%-24s %-15s 0x%x";
	char* s1 = crycode(a);
	char* s2 = crycode(b);
	char* s3 = crycode(c);
	char* s4 = crycode(d);
	uncode(s1);
	uncode(s2);
	uncode(s3);
	uncode(s4);
}