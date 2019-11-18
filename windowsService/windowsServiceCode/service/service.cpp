// service.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>

#define SERVICE_NAME "srv_demo"
SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hServiceStatusHandle;

void WINAPI service_main(int argc, char** argv);
void WINAPI ServiceHandler(DWORD fdwControl);
BOOL sign = TRUE;

int main()
{
	SERVICE_TABLE_ENTRYA ServiceTable[2];
	ServiceTable[0].lpServiceName = (LPSTR)SERVICE_NAME;
	ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTIONA)service_main;

	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;

	//启动服务的控制分派线程
	BOOL ret = StartServiceCtrlDispatcherA(ServiceTable);
	if (ret == FALSE)
	{
		DWORD nError = GetLastError();
		char info[MAX_PATH] = { 0 };
		sprintf_s(info, MAX_PATH, "xiaobo,%s-0x%x\n", __FUNCTION__, nError);
		OutputDebugStringA(info);
		return 0 ;
	}
	return 0;
}

void WINAPI service_main(int argc, char** argv)
{
	ServiceStatus.dwServiceType = SERVICE_WIN32;
	ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
	ServiceStatus.dwWin32ExitCode = 0;
	ServiceStatus.dwServiceSpecificExitCode = 0;
	ServiceStatus.dwCheckPoint = 0;
	ServiceStatus.dwWaitHint = 0;
	hServiceStatusHandle = RegisterServiceCtrlHandlerA(SERVICE_NAME, ServiceHandler);
	if (hServiceStatusHandle == 0)
	{
		DWORD nError = GetLastError();
		char info[MAX_PATH] = { 0 };
		sprintf_s(info, MAX_PATH, "xiaobo,%s-0x%x\n", __FUNCTION__,nError);
		OutputDebugStringA(info);
		return;
	}
	//add your init code here

	// Initialization complete - report running status 
	ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	ServiceStatus.dwCheckPoint = 0;
	ServiceStatus.dwWaitHint = 9000;
	if (!SetServiceStatus(hServiceStatusHandle, &ServiceStatus))
	{
		DWORD nError = GetLastError();
		char info[MAX_PATH] = { 0 };
		sprintf_s(info, MAX_PATH, "xiaobo,%s-0x%x\n", __FUNCTION__, nError);
		OutputDebugStringA(info);
		return;
	}
	else {
		while (sign)
		{
			//执行服务的功能函数，
			Sleep(4000);
		}
	}
}
void WINAPI ServiceHandler(DWORD fdwControl)
{
	switch (fdwControl)
	{
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		ServiceStatus.dwWin32ExitCode = 0;
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		ServiceStatus.dwCheckPoint = 0;
		ServiceStatus.dwWaitHint = 0;

		//add you quit code here
		sign = FALSE;
		break;
	default:
		return;
	};
	if (!SetServiceStatus(hServiceStatusHandle, &ServiceStatus))
	{
		DWORD nError = GetLastError();
	}
}
