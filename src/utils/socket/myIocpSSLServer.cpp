#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

// #include "openssl/applink.c"


#include "mysocket.h"
#include "xqueue.h"


// #pragma comment(lib, "ws2_32.lib") for visual studio

#define MAX_CONNS 2
#define BUF_SIZE 512
#define NUM_THREADS 1   // not greate than processor num


#define SERVER_CERT_FILE "d:/ztestfiles/security/server.crt"
#define SERVER_KEY_FILE "d:/ztestfiles/security/server.key"
#define ROOT_CA_FILE "d:/ztestfiles/security/rootCA.crt"
SSL_CTX *create_context();
void configure_context(SSL_CTX *ctx);
int SSLServerThread();
SSL_CTX *ctx;
SSL* ssl;

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
    socket_cbs fcbs = {0};
    cbs = &fcbs;
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

    ctx = create_context();
    configure_context(ctx);

    while(1) {
        HANDLE_CONTEXT* clientCtx = (HANDLE_CONTEXT*)calloc(1, sizeof(HANDLE_CONTEXT));
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

        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, new_socket);
        SSL_accept(ssl);

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

        ecode = GetQueuedCompletionStatus(hCompletionPort, &transferredNum, (PULONG_PTR)&clientCtx, \
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
        char *edata = (char*)malloc(transferredNum + 1);
        memcpy(edata, clientCtx->buffer, transferredNum);


        BIO* mem_bio = BIO_new(BIO_s_mem());
        SSL_set_bio(ssl, mem_bio, mem_bio);
        int bytes_written = BIO_write(mem_bio, edata, transferredNum);
        char ddata[128];
        int numRead = SSL_read(ssl, ddata, 128);
        ddata[numRead] = 0;
        printf("received decrypted data:%s\n", ddata);


        //enqueue(gQueue, ddata, transferredNum);
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
    return 0;
}

void SetCallBacks(socket_cbs* cbss) {
    cbs = cbss;
}

int socketWrite(char* data, int len) {
    if (g_cSocket) {
        return send(g_cSocket, data, len, 0);
    }
    return 0;
}


void mysocket_clean() {
    closesocket(g_sSocket);
    g_sSocket = INVALID_SOCKET;
}

SSL_CTX *create_context() {
    // SSL_library_init();
    // OpenSSL_add_all_algorithms();
    OpenSSL_add_ssl_algorithms();
    SSL_load_error_strings();
    // OPENSSL_Applink();     
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    method = TLS_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void configure_context(SSL_CTX *ctx) {
    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, SERVER_CERT_FILE, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, SERVER_KEY_FILE, SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    SSL_CTX_check_private_key(ctx);
    // 验证客户端证书
    // SSL_CTX_load_verify_locations(ctx, ROOT_CA_FILE, NULL);
    // SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
}



int createSocket(int port) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    int serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (serverSocket == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        return 0;
    }

    struct sockaddr_in sAddr;
    sAddr.sin_family = AF_INET;
    sAddr.sin_addr.s_addr = INADDR_ANY;
    sAddr.sin_port = htons(8888);
    if (bind(serverSocket, (struct sockaddr *)&sAddr, sizeof(sAddr)) == SOCKET_ERROR) {
        printf("Bind failed\n");
        return 1;
    }
    // pending connection queue 设置1来测试
    if (listen(serverSocket, 1)) {
        closesocket(serverSocket);
        printf("listen failed, return code: %d\n", errno); 
        return 1;
    }

    unsigned long mode = 1;
    ioctlsocket(serverSocket, FIONBIO, &mode);
    return serverSocket;
}

int main() {
    startIocpServer(8888);
}