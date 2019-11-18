#pragma once
#include"pch.h"
#include <windows.h>
#include<xstring>
class InstallService
{
public:
	InstallService(const char* serviceName,const char* szPath);
	~InstallService();
public:
	BOOL StartService();
	BOOL StopService();
	BOOL Install();
	BOOL Uninstall();
	DWORD QueryService();
private:
	std::string m_ServiceName;
	std::string m_szPath;
};

