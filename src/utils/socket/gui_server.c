#include <stdio.h>
#include <windows.h>

#include "mysocket.h"

#define IDC_IP_TEXT 101
#define IDC_PORT_EDIT 102
#define IDC_START_BTN 103
#define IDC_RECE_EDIT 104
#define IDC_SEND_EDIT 105
#define IDC_SEND_BTN 106
#define IDC_CLEAR_BTN 107

#define SEND_BUF_SIZE 1024

HWND g_hwnd_main;
HWND g_hwnd_recv;
HWND g_hwnd_send;

WNDPROC g_originalSendProc;
// HANDLE g_mutex;
static CRITICAL_SECTION lock;

static socket_cbs cbs;
char g_buf[SEND_BUF_SIZE];
int g_recv_count;
int state; // 0: stopped 1: running

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CustomEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int GetText(HWND hwnd, char* buf, int id);
void DisplayMessage(char* msg, int len);
void DebugMsg(char* format, ...);
int SendMessageToClient(HWND hwnd);
void ClearRecvArea();
void ToggleServer();
extern int startIocpServer(int port);

void OnReceived(char* data, int len) {
    DisplayMessage(data, len);
}

void OnStatChanged(int runState) {
    state = runState;
    HWND sub = GetDlgItem(g_hwnd_main, IDC_START_BTN);
    if (!runState) {
        SetWindowText(sub, "start");
    } else {
        SetWindowText(sub, "stop");
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    MSG msg;
    WNDCLASS wc = {0};

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
    wc.lpszClassName = TEXT("ServerApp");

    if (!RegisterClass(&wc))
        return -1;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int wwidth = 500;  // 窗口宽度
    int wheight = 400;  // 窗口高度
    int posX = (screenWidth - wwidth) / 2;
    int posY = (screenHeight - wheight) / 2;

    g_hwnd_main = CreateWindow(wc.lpszClassName, TEXT("Server Application"),
                        WS_OVERLAPPEDWINDOW , posX, posY, wwidth, wheight,
                        NULL, NULL, hInstance, NULL);

    if (g_hwnd_main == NULL)
        return -1;

    ShowWindow(g_hwnd_main, nCmdShow);
    UpdateWindow(g_hwnd_main);
    
    InitializeCriticalSection(&lock);
    socket_cbs cbs = {0};
    cbs.dataReceived = &OnReceived;
    cbs.stateChanged = OnStatChanged;
    SetCallBacks(&cbs);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    return (int) msg.wParam;
}

ULONG WINAPI startServerThred(LPVOID lpParam) {
    int *port = (int*)lpParam;
    startIocpServer(*port);
}

char* getLocalIp() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    char hostname[256];
    struct hostent *host;
    gethostname(hostname, sizeof(hostname));
    host = gethostbyname(hostname);
    WSACleanup();
    return inet_ntoa(*(struct in_addr *)*host->h_addr_list);
}

void ToggleServer(HWND hwnd) {
    if (state) {
        mysocket_clean(); 
    } else {
        char portStr[6] = {0};
        GetText(hwnd, portStr, IDC_PORT_EDIT);
        int port = atoi(portStr);
        if (port <= 0 || port > 65535) {
            MessageBox(0, "port error", "FAIL", MB_OK);
            return;
        } 
        char* ipStr = getLocalIp();
        HWND sub = GetDlgItem(hwnd, IDC_IP_TEXT);
        SetWindowText(sub, ipStr);
        HANDLE ht = CreateThread(0, 0, startServerThred, (LPVOID)&port, 0, 0);
        CloseHandle(ht);
    }

}

LRESULT CALLBACK CustomSendEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CHAR:
        if (wParam == VK_RETURN) {
            if ((GetKeyState(VK_SHIFT) & 0x8000) == 0) {
                // SendMessageToClient(hwnd);
                return 0;
            }
        }
        break;
    }
    return CallWindowProc(g_originalSendProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    long CF2 = WS_VISIBLE | WS_CHILD;
    long CF3 = CF2 | WS_BORDER;
    switch (msg) {
        case WM_CREATE:
            CreateWindow(TEXT("static"), TEXT(""),
                         CF2, 20, 20, 90, 20, 
                         hwnd, (HMENU)IDC_IP_TEXT, NULL, NULL);
                         
            CreateWindow(TEXT("edit"), TEXT("8888"),
                         CF3, 120, 20, 90, 20,
                         hwnd, (HMENU)IDC_PORT_EDIT, NULL, NULL);
            // SendMessage(hw_port, EM_SETCUEBANNER, 0, (LPARAM)TEXT("port"));
            
            CreateWindow(TEXT("button"), TEXT("start"),
                         CF3 | BS_PUSHBUTTON, 240, 20, 60, 20,
                         hwnd, (HMENU)IDC_START_BTN, NULL, NULL);

            g_hwnd_recv = CreateWindow(TEXT("edit"), TEXT(""),
                         CF3 | ES_MULTILINE | WS_VSCROLL | ES_READONLY, 
                         20, 60, 450, 170,
                         hwnd, (HMENU)IDC_RECE_EDIT, NULL, NULL);

            g_hwnd_send = CreateWindow(TEXT("edit"), TEXT(""),
                         CF3 | ES_MULTILINE | WS_VSCROLL, 
                         20, 250, 450, 50,
                         hwnd, (HMENU)IDC_SEND_EDIT, NULL, NULL);

    HFONT hFont = CreateFont(15, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE, DEFAULT_CHARSET,\
    OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "楷体");
    SendMessage(g_hwnd_recv, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(g_hwnd_send, WM_SETFONT, (WPARAM)hFont, TRUE);
    g_originalSendProc = (WNDPROC)SetWindowLongPtr(g_hwnd_send, GWLP_WNDPROC, (LONG_PTR)CustomSendEditProc);

            CreateWindowEx(
                        0, TEXT("EDIT"),   // 预定义的编辑控件类
                        TEXT("host@port"),        // 默认文本
                        CF3,
                        20, 320, 120, 20, // 控件的位置和大小
                        hwnd,        // 父窗口句柄
                        NULL, // 控件的ID
                        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                        NULL);             
            CreateWindow(TEXT("button"), TEXT("Send"),
                         CF2 | BS_PUSHBUTTON ,
                         160, 320, 60, 20,
                         hwnd, (HMENU)IDC_SEND_BTN, NULL, NULL);
            CreateWindow(TEXT("button"), TEXT("clear"),
                         CF2 | BS_PUSHBUTTON ,
                         240, 320, 60, 20,
                         hwnd, (HMENU)IDC_CLEAR_BTN, NULL, NULL);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_START_BTN:
                    ToggleServer(hwnd);
                    break;
                case IDC_SEND_BTN:
                    
                    break;
                case IDC_CLEAR_BTN:
                    ClearRecvArea();
                    break;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void ClearRecvArea() {
    SetWindowText(g_hwnd_recv, "");
    SendMessage(g_hwnd_recv, EM_SETSEL, 0, 0);
    EnterCriticalSection(&lock);
    g_recv_count = 0;
    LeaveCriticalSection(&lock);
}


int GetText(HWND hwnd, char* buf, int id) {
    HWND sub = GetDlgItem(hwnd, id);
    int len = GetWindowTextLength(sub);
    GetWindowText(sub, buf, len + 1); // append '\0'
    return len;
}

void DisplayMessage(char* msg, int len) {
    EnterCriticalSection(&lock);
    SendMessage(g_hwnd_recv, EM_SETSEL, (WPARAM)g_recv_count, (LPARAM)g_recv_count);
    SendMessage(g_hwnd_recv, EM_REPLACESEL, 0, (LPARAM)msg); // not add undo list
    g_recv_count += len;
    LeaveCriticalSection(&lock);
}

void DebugMsg(char* format, ...) {
    char tmp[64]= {0};
    va_list aptr;
    va_start(aptr, format);
    int len = vsprintf(tmp, format, aptr);
    va_end(aptr);
    tmp[len] = 13, tmp[len+1] = 10;
    DisplayMessage(tmp, strlen(tmp));
}

