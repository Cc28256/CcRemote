#pragma once

//列表枚举
enum
{
	ONLINELIST_IP = 0,          //IP的列顺序
	ONLINELIST_ADDR,          //地址
	ONLINELIST_COMPUTER_NAME, //计算机名/备注
	ONLINELIST_OS,           //操作系统
	ONLINELIST_CPU,          //CPU
	ONLINELIST_VIDEO,       //摄像头
	ONLINELIST_PING          //PING
};

//用于主界面列表的结构
typedef struct
{
	char	*title;   //列表的名称
	int		nWidth;   //列表的宽度
}COLUMNSTRUCT;

//自定义消息枚举
enum
{
	UM_ICONNOTIFY = WM_USER + 0x100,
};

