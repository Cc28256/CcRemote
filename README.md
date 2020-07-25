# CcRemote
这是一个基于gh0st远程控制的项目，使自己更深入了解远控的原理，来编写一款自己的远控(正在编写)，项目采用VS2017

这是基于gh0st更改的项目，其中加入了大量注释以及思维导图提供帮助，代码的框架思想非常值得学习，越看越觉得项目得精妙设计。

#### 通讯框架

通讯被控端采用socket，主控端采用的是IOCP完成端口,它可以高效地将I/O事件通知给应用程序,能够处理较多连接,处理逻辑我做成了xmind，一张图来了解通讯框架

![Image text](https://github.com/Cc28256/CcRemote/blob/master/readme/gh0stAnalyze.png)

#### 主界面

![Image text](https://github.com/Cc28256/CcRemote/blob/master/readme/1594463810.jpg)

## 各个功能实现的方法


#### 1 shell控制
  shell管理用到匿名管道，创建CMD子进程实现进程间通信达到操作控制的目的：
   管道pipe 用于进程间通讯的一段共享内存。创建管道的进程称为服务器，连接到一个管道的进为管道客户机。一个进程在想管道写入数据有，另一个进程就可以从瓜岛的另一端将其读取出来。匿名管道Anonymous Pipes 是在父进程和子进程单向传输数据的一种未命名的管道，只能在本地计算机中是同，不能用于网络间的通讯。

如何使用的匿名管道进行通信
    匿名管道主要用于父进程与子进程之间的的通信，首先父进程创建匿名管道，创建成功后可以获取这个匿名管道进行读写句柄，然后再创建一个子进程，子进程必须继承和使用父进程的一些公开句柄，创建子进程的时候必须将标准输入、标准输出句柄设置为父进程创建管道的管道句柄，然后就可以进行通讯了。
    
###### 创建匿名管道

```c
BOOL WINAPI CreatePipe(
          __out   PHANDLE hReadPipe,                        // __out 读取句柄
          __out   PHANDLE hWritePipe,                       // __out 写入句柄
          __in    LPSECURITY_ATTRIBUTES lpPipeAttributes,   // __in SECURITY_ATTRIBUTES结构体指针 加测返回的句柄是否能够被子进程继承，为NULL不能继承 匿名管道必须有这个结构体
          __in    DWORD nSize );                            // 缓冲区大小，参数为0时使用默认大小
          
typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

```
    lpPipeAttributes指向一个SECURITY_ATTRIBUTES 的结构体指针，其检测返回的句柄是否能够被子进程继承，如果参数为NULL，表明不能被继承
    子进程与父进程之间的通信必须构建一个这样的结构体，并且该结构体的的第三个成员变量参数必须设置为True
    这样子进程才可以进程父进程所创建的匿名管道句柄。
    
###### 创建子进程

```c
BOOL  CreateProcess( 
        LPCWSTR pszImageName,                   // 指向程序名称以NULL结尾的字符串
        LPCWSTR pszCmdLine,                     // 命令行
        LPSECURITY_ATTRIBUTES psaProcess,       // 创建进程对象设置安全性
        LPSECURITY_ATTRIBUTES psaThread,        // 该进程主线程设置安全性
        BOOL fInheritHandles,                   // *指定父进程创建的子进程是否能够继承父进程对象句柄
        DWORD fdwCreate,                        // 指定控件优先级类和进程创建的附加标记
        LPVOID pvEnvironment,                   // 只想环境块的指针
        LPWSTR pszCurDir,                       // 用来指定子进程当前的路径
        LPSTARTUPINFOW psiStartInfo,            // *指向 StartUpInfo 的结构体的指针，用来指定新进程的主窗口如何显示
        LPPROCESS_INFORMATION pProcInfo );      // ROCESS_INFORMATION 结构体的指针，用来接收关于新进程的标识信息
        
typedef struct _STARTUPINFOA {
    DWORD   cb;
    LPSTR   lpReserved;
    LPSTR   lpDesktop;
    LPSTR   lpTitle;
    DWORD   dwX;
    DWORD   dwY;
    DWORD   dwXSize;
    DWORD   dwYSize;
    DWORD   dwXCountChars;
    DWORD   dwYCountChars;
    DWORD   dwFillAttribute;
    DWORD   dwFlags;
    WORD    wShowWindow;
    WORD    cbReserved2;
    LPBYTE  lpReserved2;
    HANDLE  hStdInput;  // *
    HANDLE  hStdOutput; // *
    HANDLE  hStdError;  // *
} STARTUPINFOA, *LPSTARTUPINFOA;

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;
```
    创建进程时fInheritHandles字段我们需要设置为true，继承父进程句柄
    LPSTARTUPINFOW psiStartInfo 结构体中进行如下设置
    si.wShowWindow = SW_HIDE;                                 //隐藏CMD进程窗口
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW; //使用标准输出和标准错误输出句柄 | 控制CMD窗口隐藏
    si.hStdInput  = m_hReadPipeShell;                         // 将管道赋值 设置标准输入句柄
    si.hStdOutput = si.hStdError = m_hWritePipeShell;         // 将管道赋值 设置标准输出、标准错误句柄
    
然后通过PeekNamedPipe查询是否有新的数据，以及ReadFile进行读取管道中的内容进行读操作，WriteFile进行写入管道内容进行操作。
一般是使用while循环配套ReadFile函数。如果控制台程序暂时没有输出并且没有退出，ReadFile函数将一直等待，导致死循环。所以在使用ReadFile之前，加入PeekNamedPipe函数调用。



#### 2 进程监控

###### 进行进程枚举有很多方法

    A：CreateToolhelp32Snapshot()、Process32First()和Process32Next()
    B：EnumProcesses()、EnumProcessModules()、GetModuleBaseName()
    C：Native Api的ZwQuerySystemInformation
    D：wtsapi32.dll的WTSOpenServer()、WTSEnumerateProcess()

gh0st使用的最常见的方法A，通过建立进程快照进行遍历进程获取信息
```c
HANDLE
WINAPI
CreateToolhelp32Snapshot(
    DWORD dwFlags,      // 用来指定快照中需要返回的对象
    DWORD th32ProcessID // 一个进程ID号，为0可获取所有或当前快照
    );
```
通过函数CreateToolhelp32Snapshot获取的快照句柄使用Process32First、Process32Next遍历所有进程的PROCESSENTRY32信息
再通过GetProcessFullPath获取进程路径等信息。

###### 下面的方法可以获取进程内存列表、模块等信息，不过没有加入到项目中：

###### 获取进程模块信息使用到的API：

```c
    HANDLE WINAPI OpenProcess(
  __in          DWORD dwDesiredAccess,      // 打开的标识
  __in          BOOL bInheritHandle,        // 是否继承句柄
  __in          DWORD dwProcessId           // 被打开的进程句柄
);
    //枚举进程里的模块
   BOOL WINAPI EnumProcessModules(
  __in          HANDLE hProcess,            // 进程句柄
  __out         HMODULE* lphModule,         // 返回进程里的模块
  __in          DWORD cb,                   // 模块的个数
  __out         LPDWORD lpcbNeeded          // 存储的模块的空间大小
);  
  //得到模块的名字
  DWORD WINAPI GetModuleFileNameEx(
  __in          HANDLE hProcess,            // 进程的句柄
  __in          HMODULE hModule,            // 模块的句柄
  __out         LPTSTR lpFilename,          // 返回模块的名字
  __in          DWORD nSize                 // 缓冲区大小
);
```

###### 获取进程所有内存信息：

```c
//枚举指定进程所有内存块
//assert(hProcess != nullptr);
//参数:
//  hProcess:  要枚举的进程,需拥有PROCESS_QUERY_INFORMATION权限
//  memories:  返回枚举到的内存块数组
//返回:
//  成功返回true,失败返回false.
static bool EnumAllMemoryBlocks(HANDLE hProcess, OUT vector<MEMORY_BASIC_INFORMATION>& memories) {
	// 如果 hProcess 为空则结束运行
	assert(hProcess != nullptr);

	// 初始化 vector 容量
	memories.clear();
	memories.reserve(200);

	// 获取 PageSize 和地址粒度
	SYSTEM_INFO sysInfo = { 0 };
	GetSystemInfo(&sysInfo);
	/*
		typedef struct _SYSTEM_INFO {
		  union {
			DWORD dwOemId;							            // 兼容性保留
			struct {
			  WORD wProcessorArchitecture;			    // 操作系统处理器体系结构
			  WORD wReserved;						            // 保留
			} DUMMYSTRUCTNAME;
		  } DUMMYUNIONNAME;
		  DWORD     dwPageSize;						        // 页面大小和页面保护和承诺的粒度
		  LPVOID    lpMinimumApplicationAddress;	// 指向应用程序和dll可访问的最低内存地址的指针
		  LPVOID    lpMaximumApplicationAddress;	// 指向应用程序和dll可访问的最高内存地址的指针
		  DWORD_PTR dwActiveProcessorMask;			  // 处理器掩码
		  DWORD     dwNumberOfProcessors;			    // 当前组中逻辑处理器的数量
		  DWORD     dwProcessorType;				      // 处理器类型，兼容性保留
		  DWORD     dwAllocationGranularity;		  // 虚拟内存的起始地址的粒度
		  WORD      wProcessorLevel;				      // 处理器级别
		  WORD      wProcessorRevision;				    // 处理器修订
		} SYSTEM_INFO, *LPSYSTEM_INFO;
	*/

	//遍历内存
	const char* p = (const char*)sysInfo.lpMinimumApplicationAddress;
	MEMORY_BASIC_INFORMATION  memInfo = { 0 };
	while (p < sysInfo.lpMaximumApplicationAddress) {
		// 获取进程虚拟内存块缓冲区字节数
		size_t size = VirtualQueryEx(
			hProcess,								              // 进程句柄
			p,										                // 要查询内存块的基地址指针
			&memInfo,								              // 接收内存块信息的 MEMORY_BASIC_INFORMATION 对象
			sizeof(MEMORY_BASIC_INFORMATION32)		// 缓冲区大小
		);
		if (size != sizeof(MEMORY_BASIC_INFORMATION32)) { break; }

		// 内存块属性memInfo保存一些内存块信息可以从这里判断获取
		if (memInfo.Protect == PAGE_EXECUTE_READWRITE)
			if (memInfo.State == MEM_COMMIT)
				if (memInfo.Type == MEM_PRIVATE)
					memories.push_back(memInfo);// 将内存块信息追加到 vector 数组尾部

		// 移动指针
		p += memInfo.RegionSize;
	}

	return memories.size() > 0;
}
```



#### 3 键盘监控
###### 键盘钩子

windows系统是建立在事件驱动的机制上，整个系统都是通过消息传递来实现的，而钩子是windows系统中非常重要的系统接口，用它可以截获并处理发送给其他进程的消息来实现诸多功能，钩子种类很多，每种钩子可以截取相应的消息，例如键盘钩子截取键盘消息等等。

全局钩子运行机制，通过系统调用，将狗子挂入系统，每当特定消息发出，在消息没有到达目标窗口之前，钩子就会先行捕获到消息。这时钩子回调函数可以对消息进行操作，然后继续传递该消息，也可结束该消息的传递。每种类型的钩子都会由系统来维护一个钩子链，并且最后安装的钩子在链子的开始，最先安装的在最后。实现win32的系统钩子，必须调用API函数SetWindowsHookEx来安装这个函数

###### 安装钩子
```c
HHOOK WINAPI SetWindowsHookEx(
__in int idHook,            \\ 钩子类型
__in HOOKPROC lpfn,         \\ 回调函数地址 
__in HINSTANCE hMod,        \\ 实例句柄 (包含钩子函数的模块句柄)
__in DWORD dwThreadId);     \\ 线程ID (指定监视的线程,如果指定确定的线程，即为线程专用钩子；如果指定为空，即为全局钩子。)
```
几点需要说明的地方：
　　（1） 如果对于同一事件（如键盘消息）既安装了线程钩子又安装了系统钩子，系统会优先调用线程钩子，然后调用系统钩子。
　　（2） 对同一事件消息可安装多个钩子处理过程，这些钩子处理过程形成了钩子链。处理顺序是先安装的后处理，后安装的先处理。
　　（3） 钩子特别是系统钩子会消耗消息处理时间，降低系统性能。只有在必要的时候才安装钩子，在使用完毕后要及时卸载。
###### 定义钩子回调

    LRESULT CALLBACK HookProc(int nCode ,WPARAM wParam,LPARAM lParam) 

我们先在钩子函数中实现自定义的功能，然后调用函数 CallNextHookEx 把钩子信息传递给钩子链的下一个钩子函数。

    LRESULT CallNextHookEx( HHOOK hhk, int nCode, WPARAM wParam, LPARAM lParam )

参数 hhk是钩子句柄。nCode、wParam和lParam 是钩子函数。 
当然也可以通过直接返回TRUE来丢弃该消息，就阻止了该消息的传递。

当不再使用钩子时，必须及时卸载。简单地调用下面的函数即可。

    BOOL UnhookWindowsHookEx( HHOOK hhk)

值得注意的是线程钩子和系统钩子的钩子函数的位置有很大的差别。
线程钩子一般在当前线程或者当前线程派生的线程内，而系统钩子必须放在独立的动态链接库中。


#### active启动方式
win7 64下
     
     64位程序注册表位置 HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Active Setup\Installed Components
     32位重定位注册表位置 HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Active Setup\Installed Components

例如{052860C8-3E53-3D0B-9332-48A8B4971352}

Active Setup是微软使用此键来安装windows组件，可以在这个位置下看到已安装组件得列表，每个组件都有一个值，windows使用这些值来识别组件。其中StubPath是其中最重要的一项，它包含一个命令，windows每次启动都会执行这个命令。

创建一个（在64位位置，需要根据启动程序而定）{052860C8-3E53-3D0B-9332-48A8B4971352} StubPath 项为REG_EXPAND_SZ类型 calc.exe

1 重启计算机后，calc便会启动，但是启动后，程序执行会造成电脑卡住，无法进入系统，必须要退出程序才能执行。
2 并且再次启动calc不会再启动了这是因为在user同位置的active setup下有相同的guid，将其删除再次重启就会启动了。

所以每次执行要将user位置guid删除，并且程序通过再次启动自己或者注入到其他进程来解决上面的两个问题。


最后喜欢的话点个Star哦

![Image text](https://github.com/Cc28256/CcRemote/blob/master/readme/help.png)
