#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <Psapi.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <wctype.h>
#include <io.h>
#include <fcntl.h>
#pragma comment(lib, "Psapi.lib")
using namespace std;


bool IsSameName(LPCTSTR a, LPCTSTR b)
{
    wstring sa(a), sb(b);
    transform(sa.begin(), sa.end(), sa.begin(), ::towlower);
    transform(sb.begin(), sb.end(), sb.begin(), ::towlower);
    return sa == sb;
}

void RedirectIOToConsole()
{
    AllocConsole();

    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
}

DWORD GetProcIDFromName(LPCTSTR lpName)
{
    DWORD aProcId[1024], dwProcCnt, dwModCnt;
    HMODULE hMod;
    TCHAR szPath[MAX_PATH] = { 0 };

    if (!EnumProcesses(aProcId, sizeof(aProcId), &dwProcCnt))
        return 0;

    for (DWORD i = 0; i < dwProcCnt; ++i)
    {
        HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, aProcId[i]);
        if (NULL != hProc)
        {
            if (EnumProcessModules(hProc, &hMod, sizeof(hMod), &dwModCnt))
            {
                GetModuleBaseName(hProc, hMod, szPath, MAX_PATH);
                if (IsSameName(szPath, lpName))
                {
                    CloseHandle(hProc);
                    return aProcId[i];
                }
            }
            CloseHandle(hProc);
        }
    }
    return 0;
}

int CrashProgram(wstring process)
{
    if (process.find(L".") == wstring::npos)
        process += L".exe";

    LPCTSTR ProcessName = process.c_str();

    string a;
    DWORD pid = GetProcIDFromName(ProcessName);
    if (pid == 0)
    {
        std::cout << "Fail to find process" << std::endl;
        return GetLastError();
    }

    HANDLE hprocess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
    if (!hprocess) {
        std::cout << "Fail to open process" << std::endl;
        return GetLastError();
    }

    HMODULE kernel32 = GetModuleHandle(TEXT("kernel32.dll"));
    if (!kernel32)
    {
        std::cout << "Fail to get handle of module" << std::endl;
        return GetLastError();
    }

    PTHREAD_START_ROUTINE pfnStartAddress = (PTHREAD_START_ROUTINE)GetProcAddress(kernel32, "LoadLibraryW");
    if (!pfnStartAddress) {
        std::cout << "Fail to get address of function" << std::endl;
        return GetLastError();
    }

    HANDLE hThread = CreateRemoteThreadEx(hprocess, NULL, NULL, pfnStartAddress, (LPVOID)"no", NULL, NULL, NULL);
    if (!hThread) {
        std::cout << "Fail to create thread" << std::endl;
        return GetLastError();
    }

    std::cout << "Success" << std::endl;
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);

    if (argv != NULL && argc <= 1)
    {
        RedirectIOToConsole();

        wstring process;
        wcout << "Input the name of target process: ";
        wcin >> process;
        int res = CrashProgram(process.c_str());
        Sleep(3000);
        return res;
    }
    else
        return CrashProgram(argv[1]);
}