
#include <winsock2.h>
#include <error.h>
#include <fcntl.h>

#include "mysocket.h"

//#pragma comment(lib, "ws2_32.lib")

#ifndef PORT
#define PORT 8888
#endif

#ifndef SERVER_IP
#define SERVER_IP "127.0.0.1"
#endif

#define BUF_SIZE 128
#define TIMEOUT_SEC 5

SOCKET client_fd;
void* threads[2];
ULONG tids[2];
static u_char connected;
socket_cbs* cbs;
char gwinsock_init;

ULONG WINAPI waitReceive(LPVOID lpParam);
extern void DebugMsg(char* format, ...);

int setBlock(SOCKET fd, int flag) {
    u_long mode = flag; // o: blocking mod 1: non-blocking mode
    if (ioctlsocket(fd, FIONBIO, &mode) == SOCKET_ERROR) {
        printf("Failed to set socket to non-blocking mode\n");
        mysocket_clean();
        return 1;
    }
    return 0;
}

int ReadySocket(char* serverIp, int port) {
    if (connected) {
        return 0;
    }
    struct sockaddr_in server;
    WSADATA wsa;
    if (!gwinsock_init) {
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            printf("Initializing Winsock library failed\n");
            return 1;
        }
        gwinsock_init = 1;
    }

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        return 1;
    }
    if (port == 0) {
        port = PORT;
    }
    if (serverIp == NULL) {
        serverIp = SERVER_IP;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(serverIp);
    server.sin_port = htons(port);

    setBlock(client_fd, 1);
    int rst = connect(client_fd, (struct sockaddr *)&server, sizeof(server));
    int wsa_code = WSAGetLastError();
    if (rst < 0) {
        if (wsa_code != WSAEWOULDBLOCK) {
            printf("Connection to port %d failed with error %d\n", port, wsa_code);
            mysocket_clean();
            return 1;
        }
        printf("Connecting to server %s:%d\n", serverIp, port);
    } else {
        printf("%s:%d connected\n", serverIp, port);
    }

   // Wait for socket to become writable within the timeout period
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(client_fd, &writeSet);
    struct timeval timeout = {TIMEOUT_SEC, 0};
    int readyNum = select(0, 0, &writeSet, 0, &timeout);
    if (readyNum <= 0) { // 0: timeout
        DebugMsg("connect failed: %d", WSAGetLastError());
        mysocket_clean();
        return 1;
    }
    setBlock(client_fd, 0);
    connected = 1;
    if (cbs->stateChanged) {
        cbs->stateChanged(1);
    }
    return 0;
}

int socketWrite(char* data, int len) {
    return send(client_fd, data, len, 0);
}

void runReceive() {
    threads[0] = CreateThread(NULL, 0, waitReceive, 0, 0, &tids[1]);
}

ULONG WINAPI waitReceive(LPVOID lpParam) {
    char buffer[BUF_SIZE] = {0};
    int num = 0;
    fd_set readSet;
    char* msg;
    struct timeval timeout = {150, 0};
    int eno = 0;
    while(1) {
        FD_ZERO(&readSet);
        FD_SET(client_fd, &readSet);
        int readyNum = select(0, &readSet, 0, 0, &timeout);
        if (readyNum == 0) { // timeout
            msg = "No data received from server, disconnect from server";
            DebugMsg(msg);
            break;
        }
        if (readyNum < 0) {
            eno = WSAGetLastError();
            DebugMsg("select failed: %d, errno: %d", readyNum, eno);
            break;
        }

        num = recv(client_fd, buffer, BUF_SIZE - 2, 0);
        if (num > 0) {
            if (!FD_ISSET(client_fd, &readSet)) {
                DebugMsg("connection closed");
                break;
            }
            timeout.tv_sec = 0;
            FD_ZERO(&readSet);
            FD_SET(client_fd, &readSet);
            int hasNext = select(0, &readSet, 0, 0, &timeout);
            timeout.tv_sec = 30;
            if (hasNext == 0) {
                buffer[num] = 13, buffer[num + 1] = 10;
                num += 2;
            }
            buffer[num] = 0;
            if (cbs->dataReceived) {
                cbs->dataReceived(buffer, num);
            }
        } else if (num == 0) {
            DebugMsg("The other peer closed");
            break;
        } else {
            eno = WSAGetLastError();
            if (eno == WSAENOTSOCK) {
                DebugMsg("client closed connection");
            } else {
                DebugMsg("read error, %d", eno);
            }
            break;
        }
    }
    // ExitThread(0);
    mysocket_clean();
    if (gwinsock_init) {
        WSACleanup();
        gwinsock_init = 0;
    }
}

void SetCallBacks(socket_cbs * ccbs) {
    cbs = ccbs;
}

void mysocket_clean() {
    connected = 0;
    if (client_fd >= 0)
        closesocket(client_fd);
   
    if (cbs->stateChanged) {
        cbs->stateChanged(0);
    }
}
