#include"pch.h"
#include "InstallService.h"

InstallService::InstallService(const char* serviceName, const char* szPath)
{
	m_ServiceName = serviceName;
	m_szPath = szPath;
}

InstallService::~InstallService()
{
}

BOOL InstallService::StartService()
{
	bool bResult = false;
	SC_HANDLE sc_Manager = ::OpenSCManagerA(NULL, NULL, GENERIC_EXECUTE);
	if (sc_Manager)
	{
		SC_HANDLE sc_service = ::OpenServiceA(
			sc_Manager,
			m_ServiceName.c_str(),
			SERVICE_ALL_ACCESS
		);
		if (sc_service)
		{
			SERVICE_STATUS_PROCESS service_status;
			ZeroMemory(&service_status, sizeof(SERVICE_STATUS_PROCESS));
			DWORD dwpcbBytesNeeded = sizeof(SERVICE_STATUS_PROCESS);
			if (::QueryServiceStatusEx(sc_service, SC_STATUS_PROCESS_INFO,
				(LPBYTE)&service_status,
				dwpcbBytesNeeded,
				&dwpcbBytesNeeded))
			{
				if (service_status.dwCurrentState == SERVICE_STOPPED)
				{
					if (!::StartService(sc_service, NULL, NULL))
					{
						::CloseServiceHandle(sc_service);
						::CloseServiceHandle(sc_Manager);
						return bResult;
					}
					while (::QueryServiceStatusEx(sc_service, SC_STATUS_PROCESS_INFO,
						(LPBYTE)&service_status,
						dwpcbBytesNeeded,
						&dwpcbBytesNeeded))
					{
						Sleep(service_status.dwWaitHint);
						if (service_status.dwCurrentState == SERVICE_RUNNING)
						{
							bResult = true;
							break;
						}
					}
				}
			}
			::CloseServiceHandle(sc_service);
		}
		::CloseServiceHandle(sc_Manager);
	}
	return bResult;
}

BOOL InstallService::StopService()
{
	bool bResult = false;
	SC_HANDLE sc_Manager = ::OpenSCManagerA(NULL, NULL, GENERIC_EXECUTE);
	if (sc_Manager)
	{
		SC_HANDLE sc_service = ::OpenServiceA(
			sc_Manager,
			m_ServiceName.c_str(), 
			SERVICE_ALL_ACCESS);
		if (sc_service)
		{
			SERVICE_STATUS_PROCESS service_status;
			ZeroMemory(&service_status, sizeof(SERVICE_STATUS_PROCESS));
			DWORD dwpcbBytesNeeded = sizeof(SERVICE_STATUS_PROCESS);
			if (::QueryServiceStatusEx(sc_service, SC_STATUS_PROCESS_INFO,
				(LPBYTE)&service_status,
				dwpcbBytesNeeded,
				&dwpcbBytesNeeded))
			{
				SERVICE_CONTROL_STATUS_REASON_PARAMSA service_control_status;
				DWORD dwerror = NULL;
				ZeroMemory(&service_control_status, sizeof(SERVICE_CONTROL_STATUS_REASON_PARAMSA));
				if (service_status.dwCurrentState == SERVICE_RUNNING)
				{
					service_control_status.dwReason = SERVICE_STOP_REASON_FLAG_PLANNED | SERVICE_STOP_REASON_MAJOR_APPLICATION | SERVICE_STOP_REASON_MINOR_NONE;;
					if (!::ControlServiceExA(sc_service, SERVICE_CONTROL_STOP, SERVICE_CONTROL_STATUS_REASON_INFO, &service_control_status))
					{
						dwerror = ::GetLastError();
						::CloseServiceHandle(sc_service);
						::CloseServiceHandle(sc_Manager);
						return bResult;
					}
					while (::QueryServiceStatusEx(sc_service, SC_STATUS_PROCESS_INFO,
						(LPBYTE)&service_status,
						dwpcbBytesNeeded,
						&dwpcbBytesNeeded))
					{
						Sleep(service_status.dwWaitHint);
						if (service_status.dwCurrentState == SERVICE_STOPPED)
						{

							bResult = true;
							break;
						}
					}
				}
				else {
					if (service_status.dwCurrentState == SERVICE_STOPPED)
					{
						bResult = TRUE;
					}
				}
			}
			::CloseServiceHandle(sc_service);
		}
		::CloseServiceHandle(sc_Manager);
	}
	return bResult;
}

BOOL InstallService::Install()
{
	SC_HANDLE schSCManger;
	SC_HANDLE schService;
	//获取SCM数据库的句柄。
	schSCManger = ::OpenSCManagerA(
		NULL,
		NULL,
		SC_MANAGER_ALL_ACCESS
	);
	if (!schSCManger) {
		return FALSE;
	}
	//[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services]
	//创建服务
	schService = ::CreateServiceA(schSCManger, m_ServiceName.c_str(),
		m_ServiceName.c_str(),
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_AUTO_START,
		SERVICE_ERROR_NORMAL,
		m_szPath.c_str(),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL);
	if (!schService)
	{
		::CloseServiceHandle(schSCManger);
		return FALSE;
	}
	else
	{
		::CloseServiceHandle(schService);
		::CloseServiceHandle(schSCManger);
		return TRUE;
	}
}

BOOL InstallService::Uninstall()
{
	BOOL nResult = FALSE;
	SC_HANDLE serviceHandle;
	SC_HANDLE scmHandle = OpenSCManagerW(
		NULL, 
		NULL, 
		SC_MANAGER_ALL_ACCESS
	);
	if (scmHandle)
	{
		serviceHandle = OpenServiceA(
			scmHandle,
			m_ServiceName.c_str(),
			SERVICE_ALL_ACCESS);
		if (serviceHandle)
		{
			BOOL ret = StopService();
			if (ret == TRUE)
			{
				DeleteService(serviceHandle);
			}
			CloseServiceHandle(scmHandle);
			CloseServiceHandle(serviceHandle);
			nResult = TRUE;
		}
		else {
			CloseServiceHandle(scmHandle);
		}
	}
	return nResult;
}

DWORD InstallService::QueryService()
{
	DWORD nResult = 0;
	SC_HANDLE sc_Manager = ::OpenSCManagerA(NULL, NULL, GENERIC_EXECUTE);
	if (sc_Manager)
	{
		SC_HANDLE sc_service = ::OpenServiceA(
			sc_Manager,
			m_ServiceName.c_str(),
			SERVICE_ALL_ACCESS
		);
		if (sc_service)
		{
			SERVICE_STATUS_PROCESS service_status;
			ZeroMemory(&service_status, sizeof(SERVICE_STATUS_PROCESS));
			DWORD dwpcbBytesNeeded = sizeof(SERVICE_STATUS_PROCESS);
			if (::QueryServiceStatusEx(sc_service, SC_STATUS_PROCESS_INFO,
				(LPBYTE)&service_status,
				dwpcbBytesNeeded,
				&dwpcbBytesNeeded))
			{
				nResult = service_status.dwCurrentState;
			}
			::CloseServiceHandle(sc_service);
		}
		::CloseServiceHandle(sc_Manager);
	}
	return nResult;
}
