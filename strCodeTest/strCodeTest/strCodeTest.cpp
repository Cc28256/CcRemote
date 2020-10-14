// stringCryDemo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <windows.h>
#include <time.h>

#define SIZE 256




bool CreateMyFile(const char* strFilePath, unsigned char *lpBuffer, size_t dwSize)
{
	DWORD dwWritten;

	HANDLE hFile = CreateFile(strFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile != NULL)
	{
		WriteFile(hFile, (LPCVOID)lpBuffer, dwSize, &dwWritten, NULL);
	}
	else
	{
		return false;
	}
	CloseHandle(hFile);
	return true;
}


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





FILE * pFile;
long lSize;
unsigned char * buffer;
size_t result;
bool LoaderFile()
{
		// 一个不漏地读入整个文件，只能采用二进制方式打开
		pFile = fopen(".\\..\\..\\bin\\server\\CcMainDll.dll", "rb");

		if (pFile == NULL)
		{
			fputs("File error", stderr);
			printf("open file fail");
			return false;
		}

		// 获取文件大小
		fseek(pFile, 0, SEEK_END);
		lSize = ftell(pFile);
		rewind(pFile);
		// 分配内存存储整个文件
		buffer = (unsigned char*)malloc(sizeof(char)*lSize);

		if (buffer == NULL)
		{
			fputs("Memory error", stderr);
			printf("Memory alloc falil");
			return false;

		}
		// 将文件拷贝到buffer中
		result = fread(buffer, 1, lSize, pFile);
		if (result != lSize)
		{
			fputs("Reading error", stderr);
			printf("Load file to memory falil");
			return false;
		}
		return true;
}



//------------------------------------------------------------
//加密前的密码表
// Size    : 256 (0x100)
//------------------------------------------------------------
unsigned char EncryptTable[256] = {0};

//------------------------------------------------------------
//加密后的密码表，可用于解密时的校验
// Size    : 256 (0x100)
//------------------------------------------------------------
unsigned char ChcekTable[256] = {0};


static inline void Swap(unsigned char *a, unsigned char *b) {
	// 如果它们恰好是数组中的相同元素，不要交换它们，否则它会被归零
	if (a != b) {
		*a ^= *b;
		*b ^= *a;
		*a ^= *b;
	}
}

int InitEncryptTable(void)
{
	int i;
	// 用顺序递增的数字初始化数组
	for (i = 0; i < SIZE; ++i)
		EncryptTable[i] = i;

	// 初始化随机种子
	srand(time(NULL));

	// 将数组中的每个元素与另一个随机元素交换
	for (i = 0; i < SIZE; ++i)
		Swap(&EncryptTable[i], &EncryptTable[rand() % SIZE]);

	return 0;
}
//-------------加密函数-------------
//参数说明：参数1：被加密数组，参数2：密码表数组，参数三：加密长度
//备注：使用unsigned 为了防止异或结果错误。作为测试只加密前0x200字节
//返回值与：无 对参数影响：无
//-------------异或加密-------------
void EncryptFunc(unsigned char *SourceBytes, unsigned char *EncryptBytes, DWORD nLength)
{
	DWORD nOffsetNum = 0, nTargetNum = 0, nLastNum = 0;
	unsigned char TargetCode = '\x0', OffsetCode = '\x0', LastCode = '\x0';
	for (DWORD i = 0; i < nLength; i++)
	{
		//取密码表标志位Code
		TargetCode = EncryptBytes[((i + 1) % 0x100)];
		//取偏移Code的偏移
		nOffsetNum = (TargetCode + nOffsetNum) % 0x100;
		//取密码表偏移Code
		OffsetCode = EncryptBytes[nOffsetNum];
		//交换密码表数值
		EncryptBytes[nOffsetNum % 0x100] = EncryptBytes[((i + 1) % 0x100)];
		EncryptBytes[((i + 1) % 0x100)] = OffsetCode;
		//取最终加密Code偏移
		nLastNum = (TargetCode + OffsetCode) % 0x100;
		//获取异或用的字符串
		LastCode = EncryptBytes[nLastNum];
		//取被加密的字符,异或
		SourceBytes[i] ^= LastCode;
	}
	//在此下断观察SourceBytes和CryptData
	return;
}



int RC4Test()
{
	//加密
	DWORD nLength = 0;
	//nLength = sizeof(SourceData);
	//加密后EncryptTable会变成ChcekTable，由于加密解密使用的Key一样，因此解密时判断CheckTable一致即可
	memcpy(ChcekTable, EncryptTable, 0x100);
	EncryptFunc(buffer, EncryptTable, result);

	
	
	unsigned char * buffers = (unsigned char*)malloc(sizeof(char)*result + SIZE + SIZE);
	
	memcpy(buffers,ChcekTable,SIZE);
	
	memcpy(buffers + SIZE,EncryptTable,SIZE);
	
	memcpy(buffers +SIZE + SIZE,buffer,result);
	
	CreateMyFile(".\\..\\..\\bin\\hijack\\Cc28256.dat", buffers, result + SIZE + SIZE);
	
	//解密
	EncryptFunc(buffer, ChcekTable, result);
	
	free(buffers);

	free(buffer);
	
	return 0;
}

bool ChangeAsmJmpExp()
{
	// .10000000: 4D                             dec          ebp                       
	// .10000001: 5A                             pop          edx                       
	// .10000002: E800000000                     call        .010000007 --↓1            
	// .10000007: 5B                            1pop          ebx                       
	// .10000008: 52                             push         edx                       
	// .10000009: 45                             inc          ebp                       
	// .1000000A: 55                             push         ebp                       
	// .1000000B: 8BEC                           mov          ebp,esp                   
	// .1000000D: 81C3F9AA0000                   add          ebx,00000AA29 ;  导出函数的偏移
	// .10000013: FFD3                           call         ebx                       
	// .10000015: C9                             leave                                  
	// .10000016: C3                             retn ; 
	char HardCode[] = {0x4D,0x5A,0xE8,0x00,0x00,0x00,0x00,0x5B,0x52,0x45,0x55,0x8B,0xEC,0x81,0xC3,0x29,0xAA,0x00,0x00,0xFF,0xD3,0xC9,0xC3};
	int CodeLen = 0x15;
	memcpy(buffer,HardCode,CodeLen);
	return true;
}


int main()
{
	InitEncryptTable();
	if (LoaderFile())
	{
		ChangeAsmJmpExp();
		RC4Test();
	
	}
	
	char a[] = "kernel32";
	char b[] = "GetModuleFileNameA";
	char c[] = "\\Cc28256.dat";
	char d[] = "REG_MULTI_SZ";
	char* s1 = crycode(a);
	char* s2 = crycode(b);
	char* s3 = crycode(c);
	char* s4 = crycode(d);
	uncode(s1);
	uncode(s2);
	uncode(s3);
	uncode(s4);
}