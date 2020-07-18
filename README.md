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


最后喜欢的话点个Star哦

![Image text](https://github.com/Cc28256/CcRemote/blob/master/readme/help.png)
