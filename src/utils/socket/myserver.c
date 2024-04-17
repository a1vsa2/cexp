#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

#include "mysocket.h"
#include "xqueue.h"

// #pragma comment(lib, "ws2_32.lib") for visual studio


#define BUF_SIZE 512
#define NUM_THREADS 1   // not greate than processor num

typedef struct {
    SOCKET cSocket;
    SOCKADDR_IN cAddr;
    long cid;
    char buffer[BUF_SIZE];
    WSABUF wsaBuf;
} HANDLE_CONTEXT;

typedef struct {
    OVERLAPPED overlapped;
    // char buffer[BUF_SIZE];
    // WSABUF wsaBuf;
} IO_CONTEXT;

static socket_cbs *cbs;
HANDLE g_hcp;
int winsock_init;
static long id;
static int ecode;
static int wsacode;
static Queue* gQueue;
static SOCKET g_sSocket;
static SOCKET g_cSocket;

DWORD WINAPI CompletionThread(LPVOID pComPort);
DWORD WINAPI HandleReceivedThread(LPVOID lpParam);

int startIocpServer(int port) {
    wsacode = ReadySocket(0, port);
    if (wsacode) {
        if (cbs->stateChanged) {
            cbs->stateChanged(wsacode + 1);
        }
        return wsacode;
    }
    int addrLen = sizeof(SOCKADDR_IN);
    int flags = 0;
    HANDLE hts[NUM_THREADS];
    g_hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (ULONG_PTR)0, 0);

    for (int i = 0; i < NUM_THREADS; i++) {
        hts[i] = CreateThread(NULL, 0, CompletionThread, g_hcp, 0, NULL);
    }
    
    HANDLE ht = CreateThread(NULL, 0, HandleReceivedThread, 0, 0, NULL);

    gQueue = createQueue(0);
    if (cbs->stateChanged) {
        cbs->stateChanged(1);
    }

    while(1) {
        HANDLE_CONTEXT *clientCtx = calloc(1, sizeof(HANDLE_CONTEXT));
        // IO_CONTEXT *ioCtx = calloc(1, sizeof(IO_CONTEXT));

        printf("waiting accept...\n");
        SOCKET new_socket = accept(g_sSocket, (struct sockaddr*)(&clientCtx->cAddr), &addrLen);
        if (new_socket == INVALID_SOCKET) {
            break;
        }
        clientCtx->cSocket = new_socket;
        clientCtx->wsaBuf.len = BUF_SIZE;
        clientCtx->wsaBuf.buf = clientCtx->buffer;
        flags = 0;
        g_cSocket = new_socket;

        CreateIoCompletionPort((HANDLE)new_socket, g_hcp, (ULONG_PTR)clientCtx, 0);
        OVERLAPPED *ovp = (OVERLAPPED*)calloc(1, sizeof(OVERLAPPED));
        ecode = WSARecv(new_socket, &(clientCtx->wsaBuf), 1, NULL, (LPDWORD)&flags, ovp, NULL);
        if (ecode == SOCKET_ERROR && (wsacode = WSAGetLastError()) != ERROR_IO_PENDING) {
            free(ovp);
            closesocket(new_socket);
            g_cSocket = (ULONG_PTR)NULL;
            printf("WSARecv error:%d\n", wsacode);
            break;
        }
    }
    // __debugbreak();
    for (int i = 0; i< NUM_THREADS; i++) {
        PostQueuedCompletionStatus(g_hcp, 0, (ULONG_PTR)0, 0);
    }
    WaitForMultipleObjects(NUM_THREADS, hts, TRUE, 5000);
    WaitForSingleObject(ht, 2000);
    if (g_sSocket != INVALID_SOCKET) {
        closesocket(g_sSocket);
    }
    CloseHandle(g_hcp);
    // for (int i = 0; i < NUM_THREADS; i++) {
    //     CloseHandle(hts[i]);
    // }
    if (winsock_init) {
        WSACleanup();
        winsock_init = 0;
    } 
    if (cbs->stateChanged) {
        cbs->stateChanged(0);
    }
    return 0;
}

// int main() {
//     startIocpServer(8888);
//     mysocket_clean();
//     return 0;
// }

int ReadySocket(char* serverIp, int port) {
    WSADATA wsa;
    if (!winsock_init) {
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            printf("WSAStartup failed\n");
            wsacode = WSAGetLastError();
            return wsacode;
        }
        winsock_init = 1;
    }
    // TODO set reuse
    g_sSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP); // 会创建支持重叠 I/O 操作作为默认行为的套接字
    SOCKADDR_IN sAddr;
    sAddr.sin_family = AF_INET;
    sAddr.sin_addr.s_addr = INADDR_ANY;
    sAddr.sin_port = htons(port);
    ecode = bind(g_sSocket, (SOCKADDR*)&sAddr, sizeof(sAddr));
    if (ecode == SOCKET_ERROR) {
        wsacode = WSAGetLastError();
        return wsacode;
    }
    listen(g_sSocket, 2);
    return 0;
}


DWORD WINAPI CompletionThread(LPVOID pComPort) {
    HANDLE hCompletionPort = (HANDLE)pComPort;
    DWORD transferredNum = 0;
    HANDLE_CONTEXT *clientCtx = NULL;
    // IO_CONTEXT *ioCtx = NULL;
    OVERLAPPED *overlapped_ptr = 0;
    DWORD flags = 0;
    int ecode;
    int wascode;
    while (1) {

        ecode = GetQueuedCompletionStatus(hCompletionPort, &transferredNum, (PLONG_PTR)&clientCtx, \
            (LPOVERLAPPED*)&overlapped_ptr, INFINITE);
        if (!ecode) {
            break;
        }

        if (transferredNum == 0) {
            printf("client socket closed\n");
            free(overlapped_ptr);
            if (clientCtx) {
                closesocket(clientCtx->cSocket);
                g_cSocket = (ULONG_PTR)NULL;
                free(clientCtx);
                clientCtx = NULL;
                continue;
            } else {
                break;
            } 
        }

        // WSASend(clientData->cSocket, &(clientData->wsaBuf), 1, NULL, 0, NULL, NULL);
        char *data = malloc(transferredNum + 1);
        memcpy(data, clientCtx->buffer, transferredNum);
        data[transferredNum] = 0;
        enqueue(gQueue, data, transferredNum);
        // memset(overlapped_ptr, 0, sizeof(OVERLAPPED));

        flags = 0;
        ecode = WSARecv(clientCtx->cSocket, &(clientCtx->wsaBuf), 1, NULL, &flags, overlapped_ptr, NULL);
        if (ecode == SOCKET_ERROR && (wsacode = WSAGetLastError()) != ERROR_IO_PENDING) {
            printf("WSARecv failed:%d\n", wsacode);
            break;
        }
    }
    if (clientCtx) {
        closesocket(clientCtx->cSocket);
        g_cSocket = (ULONG_PTR)NULL;
        free(clientCtx);
    }
    free(overlapped_ptr);
    return 0;
}

DWORD WINAPI HandleReceivedThread(LPVOID lpParam) {

    char* data;
    int unused = 0;
    while(data = (char*)dequeue(gQueue, &unused)) {
        if (cbs->dataReceived) {
            cbs->dataReceived(data, strlen(data));
        }
        free(data);
    }
}

void SetCallBacks(socket_cbs* cbss) {
    cbs = cbss;
}

int socketWrite(char* data, int len) {
    if (g_cSocket) {
        return send(g_cSocket, data, len, 0);
    }
}


void mysocket_clean() {
    closesocket(g_sSocket);
    g_sSocket = INVALID_SOCKET;
}


// DWORD WINAPI ServerThread(LPVOID param) {
//     struct sockaddr_in sAddr, cAddr; // ipv4
//     fd_set readSet;
    
//     SOCKET sd = 0;
//     int numRead = 0;
//     int addrLen = 0;
//     struct timeval timeout = {300, 0};

//     if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == INVALID_SOCKET) {
//         printf("Socket creation failed\n");
//         return 1;
//     }

//     sAddr.sin_family = AF_INET;
//     sAddr.sin_addr.s_addr = INADDR_ANY;
//     sAddr.sin_port = htons(PORT);

//     if (bind(server_fd, (struct sockaddr *)&sAddr, sizeof(sAddr)) == SOCKET_ERROR) {
//         printf("Bind failed\n");
//         return 1;
//     }

//     // pending connection queue 
//     if (listen(server_fd, 2)) {
//         closesocket(server_fd);
//         printf("listen failed, return code: %d\n", errno); 
//         return 1;
//     }
//     printf("Server started at port %d\n", PORT);SOCKET *ps = (SOCKET*)clients;
//     while (1) {
//         FD_ZERO(&readSet);
//         FD_SET(server_fd, &readSet);
//         for (int i = 0; i < MAX_CONNS; i++) {
//             sd = clients[i];
//             if (sd > 0) {
//                 FD_SET(sd, &readSet);
//             }
//         }
//         printf("selecting...\n");
//         int readyNum = select(0, &readSet, 0, 0, 0); // nfds: ignored for windows
//         if ((readyNum < 0) && (errno != EINTR)) { // interrupt
//             printf("select error: %d\n", WSAGetLastError());
//         }
       
//         addrLen = sizeof(cAddr);
//         if (FD_ISSET(server_fd, &readSet)) { // incoming connection
//             SOCKET new_socket = accept(server_fd, (struct sockaddr *)&cAddr, &addrLen);
//             if (new_socket == INVALID_SOCKET) {
//                 printf("Accept failed\n");
//                 break;
//             }
//             if (count == MAX_CONNS) {
//                 printf("已达到最大连接数,拒绝此次连接\n");
//                 closesocket(new_socket);
//             } else {
//                 count++;
//                 printf("New connection , socket fd is %d , %s:%d\n" , new_socket , \
//                 inet_ntoa(cAddr.sin_addr) , ntohs(cAddr.sin_port));
//                 for (int i = 0; i < MAX_CONNS; i++) {
//                     if (clients[i] == 0) {
//                         ps[i] = new_socket;         
//                         break;
//                     }
//                 }
//             }
//         }

//         for (int i = 0; i < MAX_CONNS; i++) {
//            sd = clients[i];
//            if (sd && FD_ISSET(sd, &readSet)) {
//                 numRead = recv(sd, buffer, BUF_SIZE, 0);
//                 printf("read %d bytes\n", numRead);
//                 if (numRead == 0 || numRead == SOCKET_ERROR) {
//                     getpeername(sd , (struct sockaddr*)&cAddr , &addrLen);
//                     printf("Host disconnected , port %d, %d\n" , ntohs(cAddr.sin_port), WSAGetLastError());    
//                     closesocket(sd);
//                     clients[i] = 0;
//                     count--;
//                 } else if (numRead > 0) {
//                     buffer[numRead] = '\0';
//                     printf("received msg: %s\n", buffer);
//                     send(sd, "server received", 15, 0);
//                     if (strcmp(buffer, "exit") == 0) {
//                         goto endloop;
//                     }
//                 }
//            }
//         }
//     }
//     endloop:
//     for (int i = 0; i < MAX_CONNS; i++) {
//         sd = clients[i];
//         if (sd > 0) {
//             closesocket(sd);
//         }
//     }
//     closesocket(server_fd);
//     return 0;
// }