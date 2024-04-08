#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
// #include <windowsx.h>

#include "mysocket.h"

//#pragma comment(lib, "ws2_32.lib") // 链接 Winsock 库
// http://www.jasinskionline.com/WindowsApi/ref/


#define IDC_IP_TEXT 101
#define IDC_PORT_TEXT 102
#define IDC_CONN_BTN 103
#define IDC_DISPLAY_EDIT 104
#define IDC_SEND_EDIT 105
#define IDC_CLEAR_BTN 106

#define SEND_BUF_SIZE 1024
#define SHOW_SEND 0

HWND g_main_hwnd;
static HWND hwnd_send;
static HWND hwnd_display;
char send_buf[SEND_BUF_SIZE];
static int g_display_count;
HANDLE g_mutex;
WNDPROC g_originalEditProc;
char g_serverIp[16] ={0};
char g_portStr[6] = {0};
static char connected;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CustomEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int GetText(HWND hwnd, char* buf, int id);
void DisplayMessage(char* msg, int len);
void DebugMsg(char* format, ...);
int SendMessageToServer(HWND hwnd);


void OnReceived(char* data, int len) {
    DisplayMessage(data, len);
}
void OnStatChanged(int state) {
    connected = state;
    HWND sub = GetDlgItem(g_main_hwnd, IDC_CONN_BTN);
    if (connected) {
        runReceive();
        SetWindowText(sub, "disconnect");
    } else {
        SetWindowText(sub, "connect");
    }
}

void ToggleConnection(HWND hwnd) {
    if (connected) {
        mysocket_clean();
    } else {
        char serverIp[16] ={0};
        char portStr[6] = {0};
        
        GetText(hwnd, serverIp, IDC_IP_TEXT);
        GetText(hwnd, portStr, IDC_PORT_TEXT);
        if (connected && (strcmp(g_serverIp, serverIp) || strcmp(g_portStr, portStr))) {
            mysocket_clean();
        }
        int serverPort = atoi(portStr);
        ReadySocket(serverIp, serverPort);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    MSG msg;
    WNDCLASS wc = {0};

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
    wc.lpszClassName = "ClientApp";

    if (!RegisterClass(&wc))
        return -1;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int wwidth = 500;  // 窗口宽度
    int wheight = 400;  // 窗口高度
    int posX = (screenWidth - wwidth) / 2;
    int posY = (screenHeight - wheight) / 2;

    g_main_hwnd = CreateWindow(wc.lpszClassName, "Client Application",
                        WS_OVERLAPPEDWINDOW, posX, posY, wwidth, wheight,
                        NULL, NULL, hInstance, NULL);

    if (g_main_hwnd == NULL)
        return -1;

    g_mutex = CreateMutex(NULL, FALSE, NULL);
    socket_cbs cbs;
    cbs.dataReceived = &OnReceived;
    cbs.stateChanged = &OnStatChanged;
    SetCallBacks(&cbs);

    // EnableWindow(hwnd_display, 0);
    // EnableScrollBar(hwnd_display, SB_CTL, ESB_ENABLE_BOTH);
    ShowWindow(g_main_hwnd, nCmdShow);
    UpdateWindow(g_main_hwnd);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    mysocket_clean();
    return (int) msg.wParam;
}

void ClearDisplayArea() {
    SetWindowText(hwnd_display, "");
    SendMessage(hwnd_display, EM_SETSEL, 0, 0);
    WaitForSingleObject(g_mutex, 20000);
    g_display_count = 0;
    ReleaseMutex(g_mutex);
}

// 发送消息
int SendMessageToServer(HWND hwnd) {
    int len = GetWindowTextLength(hwnd);
    if (len > SEND_BUF_SIZE - 1) {
        MessageBox(0, TEXT("超出长度限制"), "", 1);
        return 0;
    }
    GetWindowText(hwnd, send_buf, len + 1); // append '\0'

    if (len > SEND_BUF_SIZE - 2) {
        send_buf[SEND_BUF_SIZE -2] = 13, send_buf[SEND_BUF_SIZE - 1] = 10;
        len = SEND_BUF_SIZE - 2;
    } else {
        send_buf[len] = 13, send_buf[len + 1] = 10;
    }
    // send ok
    int rst = socketWrite(send_buf, len);
    if (rst > 0) {
        if (SHOW_SEND)
            DisplayMessage(send_buf, len + 2);
        SetWindowText(hwnd_send, "");
        SendMessage(hwnd_send, EM_SETSEL, 0, 0);
    }
    memset(send_buf, 0, SEND_BUF_SIZE);
    return rst;
}

LRESULT CALLBACK CustomEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CHAR:
        if (wParam == VK_RETURN) {
            if ((GetKeyState(VK_SHIFT) & 0x8000) == 0) {
                SendMessageToServer(hwnd);
                return 0;
            }
        }
        break;
    }
    // 对于所有其他消息，调用原始的窗口过程
    return CallWindowProc(g_originalEditProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    long CF2 = WS_VISIBLE | WS_CHILD;
    long CF3 = CF2 | WS_BORDER;
    switch (msg) {
        case WM_CREATE:
            CreateWindow(TEXT("static"), TEXT("IP Address:"),
                         CF2, 20, 20, 90, 20, 
                         hwnd, NULL, NULL, NULL);
            CreateWindow(TEXT("edit"), TEXT("127.0.0.1"),
                         CF3, 120, 20, 140, 20,
                         hwnd, (HMENU)IDC_IP_TEXT, NULL, NULL);
                         
            CreateWindow(TEXT("static"), TEXT("Port:"),
                         CF2, 20, 50, 90, 20,
                         hwnd, NULL, NULL, NULL);
            CreateWindow(TEXT("edit"), TEXT("8888"),
                         CF3, 120, 50, 140, 20,
                         hwnd, (HMENU)IDC_PORT_TEXT, NULL, NULL);
                         
            CreateWindow(TEXT("button"), TEXT("Connect"),
                         CF2 | BS_PUSHBUTTON, 280, 20, 80, 50,
                         hwnd, (HMENU)IDC_CONN_BTN, NULL, NULL);
            hwnd_display = CreateWindowEx(
                        0, "EDIT",   // 预定义的编辑控件类
                        NULL,        // 默认文本
                        CF3 | ES_MULTILINE | WS_VSCROLL| ES_AUTOVSCROLL | ES_READONLY,
                        20, 100, 450, 150, // 控件的位置和大小
                        hwnd,        // 父窗口句柄
                        (HMENU)IDC_DISPLAY_EDIT, // 控件的ID
                        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                        NULL);

            hwnd_send = CreateWindow(TEXT("edit"), TEXT(""),
                         CF3 | ES_MULTILINE | WS_VSCROLL, 
                         20, 260, 450, 50,
                         hwnd, (HMENU)IDC_SEND_EDIT, NULL, NULL);

    HFONT hFont = CreateFont(15, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, \
    CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "楷体");
    SendMessage(hwnd_display, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hwnd_send, WM_SETFONT, (WPARAM)hFont, TRUE);
    g_originalEditProc = (WNDPROC)SetWindowLongPtr(hwnd_send, GWLP_WNDPROC, (LONG_PTR)CustomEditProc);
            CreateWindow(TEXT("button"), TEXT("clear"),
                         CF2 | BS_PUSHBUTTON ,
                         220, 320, 60, 20,
                         hwnd, (HMENU)IDC_CLEAR_BTN, NULL, NULL);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_CONN_BTN:
                    ToggleConnection(hwnd);
                    break;
                case IDC_CLEAR_BTN:
                    ClearDisplayArea();
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

int GetText(HWND hwnd, char* buf, int id) {
    HWND sub = GetDlgItem(hwnd, id);
    int len = GetWindowTextLength(sub);
    GetWindowText(sub, buf, len + 1); // append '\0'
    return len;
}

void DisplayMessage(char* msg, int len) {
    WaitForSingleObject(g_mutex, 20000);
    SendMessage(hwnd_display, EM_SETSEL, (WPARAM)g_display_count, (LPARAM)g_display_count);
    SendMessage(hwnd_display, EM_REPLACESEL, 0, (LPARAM)msg); // not add undo list
    g_display_count += len;
    ReleaseMutex(g_mutex);
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

// void initFont() {
//       HFONT hFont = CreateFont(
//         20,  // 字体高度
//         0,   // 字体宽度
//         0,   // 字体倾斜角度
//         0,   // 字体旋转角度
//         FW_NORMAL, // 字体粗细
//         FALSE,     // 是否为斜体
//         FALSE,     // 是否为下划线
//         FALSE,     // 是否为删除线
//         DEFAULT_CHARSET, // 字符集
//         OUT_DEFAULT_PRECIS, // 输出精度
//         CLIP_DEFAULT_PRECIS, // 剪裁精度
//         DEFAULT_QUALITY,     // 字体质量
//         DEFAULT_PITCH | FF_SWISS, // 字体间距和族
//         "Arial" // 字体名称
//     );
// }


/*    ====== debug code
void caret() {
    UINT start, end;
// 使用 SendMessage 函数获取选区
SendMessage(hwnd_display, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);      
DWORD dwPos;
SendMessage(hwnd_display, EM_GETSEL, (WPARAM)&dwPos, 0);
int nCaretPos = LOWORD(dwPos); // 光标的线性位置
// 获取光标所在的行号
int nLine = SendMessage(hwnd_display, EM_LINEFROMCHAR, nCaretPos, 0);
// 获取该行第一个字符的索引
int nLineIndex = SendMessage(hwnd_display, EM_LINEINDEX, nLine, 0);
// 计算列号（光标位置 - 该行第一个字符的位置）
int nColumn = nCaretPos - nLineIndex;
char msg[32] = {0};
sprintf(msg, "%d %d, %d %d %d %d", start, end, nLine, nColumn, nCaretPos, nLineIndex);
MessageBox(NULL, msg, "Success", MB_OK);
}

FILE *fp;
void record(char* format, ...) {
    if (fp == NULL)
        fp = fopen("d:/debug.txt", "w+");
    va_list vaptr;
    va_start(vaptr, format);
    vfprintf(fp, format, vaptr);
    va_end(vaptr);
    fflush(fp);
} 

*/