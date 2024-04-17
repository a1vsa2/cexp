#include <windows.h>



int main() {
    // 获取目标进程ID，在这里假设你已有目标进程ID
    DWORD processId = 0; // TOD FindProcess

    // 打开目标进程
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        return 1;
    }

    // 分配内存，用于写入DLL的路径
    void* pDllPath = VirtualAllocEx(hProcess, 0, MAX_PATH, MEM_COMMIT, PAGE_READWRITE);

    // 写入DLL路径
    char dllPath[] = "D:\\myhook.dll"; // 你的DLL路径
    WriteProcessMemory(hProcess, pDllPath, (LPVOID)dllPath, strlen(dllPath) + 1, NULL);

    // 获取LoadLibraryA的地址
    LPTHREAD_START_ROUTINE pLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");

    // 创建远程线程，加载DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pLoadLibrary, pDllPath, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);

    // 清理
    VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    
    return 0;
}