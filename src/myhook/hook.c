#include <windows.h>
#include <stdio.h>

// 目标函数的原型，用于定义函数指针类型和获取目标函数的地址
typedef int(*TargetFunction)(int);

// 自定义的函数，用于被目标函数调用
int CustomFunction(int param) {
    printf("Custom function called with parameter: %d\n", param);
    return param * 2; // 可以在这里进行自定义的处理逻辑
}

// 定义一个指向自定义函数的函数指针
TargetFunction customFuncPtr = CustomFunction;

// API挂钩函数，用于在目标进程函数被调用时自动调用自定义函数
int HookedFunction(int param) {
    printf("Hooked function called with parameter: %d\n", param);
    return customFuncPtr(param); // 自动调用自定义函数，并返回其返回值
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <PID>\n", argv[0]);
        return 1;
    }

    DWORD dwPID = atoi(argv[1]);

    // 获取目标进程中要进行API挂钩的函数地址
    HMODULE hModule = GetModuleHandle(NULL);
    TargetFunction targetFunc = (TargetFunction)GetProcAddress(hModule, "TargetFunctionName");
    if (targetFunc == NULL) {
        printf("Failed to get address of target function.\n");
        return 1;
    }

    // 使用API挂钩将目标函数替换为自定义函数
    DWORD oldProtect;
    VirtualProtect((LPVOID)targetFunc, sizeof(TargetFunction), PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(targetFunc, &HookedFunction, sizeof(TargetFunction));
    VirtualProtect((LPVOID)targetFunc, sizeof(TargetFunction), oldProtect, &oldProtect);

    printf("API hooking successful!\n");

    // 等待用户输入任意字符，然后恢复原始函数并退出
    getchar();

    VirtualProtect((LPVOID)targetFunc, sizeof(TargetFunction), PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(targetFunc, &customFuncPtr, sizeof(TargetFunction));
    VirtualProtect((LPVOID)targetFunc, sizeof(TargetFunction), oldProtect, &oldProtect);

    printf("API hooking removed. Exiting...\n");

    return 0;
}