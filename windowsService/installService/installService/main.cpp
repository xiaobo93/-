// installService.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include "InstallService.h"
//
//安装服务程序
//
int main()
{
	InstallService tmp("srv_test","c:\\hellworld\n");
	BOOL RET = tmp.Install();
	BOOL RET = tmp.Uninstall();
	//服务名称 可执行程序路径
    std::cout << "Hello World!\n"; 
}


