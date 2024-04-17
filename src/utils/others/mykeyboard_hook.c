#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

HHOOK hHook = NULL;

// 钩子回调函数
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
        if (wParam == WM_KEYDOWN) {
            printf("key down\n");
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

// 设置钩子
void SetHook() {
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (hHook == NULL) {
        printf("Failed to install hook!\n");
    }
}

// 移除钩子
void UnHook() {
    UnhookWindowsHookEx(hHook);
}

int main() {
    SetHook();
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnHook();
    return 0;
}