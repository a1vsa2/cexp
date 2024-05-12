
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#define MAX_CONNS 2

#define SERVER_CERT_FILE "d:/ztestfiles/security/server.crt"
#define SERVER_KEY_FILE "d:/ztestfiles/security/server.key"
#define ROOT_CA_FILE "d:/ztestfiles/security/rootCA.crt"
SSL_CTX *create_context();
void configure_context(SSL_CTX *ctx);
int SSLServerThread();
SSL_CTX *ctx;


int createSocket(int port);
DWORD WINAPI ServerThread(LPVOID param);


int main() {
    ctx = create_context();
    configure_context(ctx);
    SSLServerThread();
    // HANDLE th = CreateThread(0, 0, SSLServerThread, 0, 0, 0);
    // WaitForSingleObject(th, INFINITE);
    return 0;
}


DWORD WINAPI ServerThread(LPVOID param) {
    SOCKET server_fd = createSocket(8888);

    const int BUF_SIZE = 256;
    SOCKET clients[2] = {0};
    char buffer[256] = {0};
    static int count = 0;
    
    int numRead = 0;
    int addrLen = 0;
    struct sockaddr_in cAddr; // ipv4
    SOCKET client = 0;
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(server_fd, &readSet);
    while (1) {
        // FD_ZERO(&readSet);
        // FD_SET(server_fd, &readSet);
        // if (count < MAX_CONNS) { // 会阻塞连接队列的连接不能释放,直到重新放入selector
        //     FD_SET(server_fd, &readSet);
        // }   
        // for (int i = 0; i < MAX_CONNS; i++) {
        //     client = clients[i];
        //     if (client > 0) {
        //         FD_SET(client, &readSet);
        //     }
        // }

        int readyNum = select(0, &readSet, 0, 0, 0); // nfds: ignored for windows
        if ((readyNum < 0) && (errno != EINTR)) { // interrupt
            printf("select error: %d\n", WSAGetLastError());
        }
        
        addrLen = sizeof(cAddr);
        SOCKET new_socket = INVALID_SOCKET;
        if (FD_ISSET(server_fd, &readSet)) { // incoming connection
            new_socket = accept(server_fd, (struct sockaddr *)&cAddr, &addrLen);
            if (new_socket == INVALID_SOCKET) {
                printf("Accept failed\n");
                break;
            }
            if (count == MAX_CONNS) {
                printf("以达到最大连接数,拒绝此次连接\n");
                closesocket(new_socket);
            } else {
                FD_SET(client, &readSet);
                count++;
                for (int i = 0; i < MAX_CONNS; i++) {
                    if (clients[i] == 0) {
                        clients[i] = new_socket;      
                        break;
                    }
                }
                printf("New connection,%s:%d\n", inet_ntoa(cAddr.sin_addr) , ntohs(cAddr.sin_port));
            }  
        }

        // 处理已有连接
        for (int i = 0; i < MAX_CONNS; i++) {
            client = clients[i];
            if (client && FD_ISSET(client, &readSet)) {
                numRead = recv(client, buffer, BUF_SIZE, 0);
                printf("read %d bytes\n", numRead);
                if (numRead == 0 || numRead == SOCKET_ERROR) {
                    getpeername(client , (struct sockaddr*)&cAddr , &addrLen);
                    printf("Host disconnected , port %d, %d\n" , ntohs(cAddr.sin_port), WSAGetLastError());    
                    closesocket(client);
                    clients[i] = 0;
                    FD_CLR(client, &readSet);
                    count--;
                } else if (numRead > 0) { // 假设一次读取完整
                    buffer[numRead] = '\0';
                    printf("received msg: %s\n", buffer);
                    send(client, "HTTP/1.1 200 OK\nContent-Length: 5\n\nhello\n", 42, 0);
                    if (numRead >= 4 && strcmp(buffer + (numRead - 4), "exit") == 0) {
                        goto endloop;
                    }
                }
            }
        }
    }

    endloop:
    for (int i = 0; i < MAX_CONNS; i++) {
        client = clients[i];
        if (client && client != INVALID_SOCKET) {
            closesocket(client);
        }
    }
    closesocket(server_fd);
    WSACleanup();
    return 0;
}

int SSLServerThread() {
    SOCKET serverSocket = createSocket(8888);

    const int BUF_SIZE = 1024;
    char buffer[BUF_SIZE + 1];
    static int count = 0;

    SSL* rwConns[2] = {0};
    SOCKET clients[2] = {0};
    SSL* hConns[2] = {0};

    int numRead = 0;
    struct sockaddr_in cAddr; // ipv4
    int addrLen = sizeof(cAddr);
    SOCKET client = INVALID_SOCKET;
    SSL* sconn = 0;
    fd_set readSet;
    
    TIMEVAL to = {3, 0};
    while (1) {
        FD_ZERO(&readSet);
        FD_SET(serverSocket, &readSet);
        for(int i = 0; i < MAX_CONNS; i++) {
            if (clients[i]) {
                FD_SET(clients[i], &readSet);
            }
        }
        int readyNum = select(0, &readSet, 0, 0, &to); // nfds: ignored for windows
        if ((readyNum < 0) && (errno != EINTR)) { // interrupt
            //printf("select error: %d\n", WSAGetLastError());
            perror("select failed");
        }
        
        if (FD_ISSET(serverSocket, &readSet)) { // incoming connection
            client = accept(serverSocket, (struct sockaddr *)&cAddr, &addrLen);
            if (client == INVALID_SOCKET) {
                perror("Accept failed");
                break;
            }
            if (count == MAX_CONNS) {
                printf("以达到最大连接数,拒绝此次连接\n");
                closesocket(client);
            } else {
                count++;
                for (int i = 0; i < MAX_CONNS; i++) {
                    if (clients[i] == 0) {
                        clients[i] = client;
                        sconn = SSL_new(ctx); // ossl_ssl_connection_new
                        SSL_set_fd(sconn, client);
                        SSL_accept(sconn);        
                        hConns[i] = sconn;
                        break;
                    }
                }
                printf("New connection,%s:%d\n", inet_ntoa(cAddr.sin_addr) , ntohs(cAddr.sin_port));
            }  
        }
 
        // 处理ssl握手的连接
        for (int i = 0; i < MAX_CONNS; i++) {
            sconn = hConns[i];
            if (sconn) {
                int ret = SSL_accept(sconn);
                if (ret <= 0) {
                    if (ret < 0) {
                        int err = SSL_get_error(sconn, ret);
                        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                            // printf("正在握手中\n");
                            continue;
                        } 
                    } 
                    // else {
                    //     printf("ssl handshake success but connection closed\n");
                    // }    
                    printf("握手失败\n");
                    SSL_shutdown(sconn);
                    SSL_free(sconn);
                    hConns[i] = 0;
                    ERR_print_errors_fp(stderr);
                    closesocket(clients[i]);
                    FD_CLR(clients[i], &readSet);
                    clients[i] = 0;      
                    count--;
                } else {
                    printf("握手成功\n");
                    //FD_SET(clients[i], &readSet);
                    hConns[i] = 0;
                    rwConns[i] = sconn;
                }                        
            }
        }

        // 处理ssl连接的读写
        for (int i = 0; i < MAX_CONNS; i++) {
            sconn = rwConns[i];
            if (sconn == 0) {
                continue;
            }
            int num = SSL_pending(sconn);
            if (1) {
                printf("pending:%d\n", num);
                numRead = SSL_read(sconn, buffer, BUF_SIZE);               
                if (numRead <= 0) {
                    int se = SSL_get_error(sconn, numRead);
                    //printf("ssl read code:%d\n", se);
                    if (se == SSL_ERROR_WANT_READ) {
                        continue;
                    } else {
                        getpeername(clients[i] , (struct sockaddr*)&cAddr , &addrLen);
                        printf("ssl read failed , port %d, %d\n" , ntohs(cAddr.sin_port), se);
                        FD_CLR(clients[i], &readSet); 
                        closesocket(clients[i]);
                        clients[i] = 0;
                        rwConns[i] = 0;
                        count--;
                    }
                    
                } else if (numRead > 0) { // 假设一次读取完整
                    buffer[numRead] = '\0';
                    printf("received msg[%d]: \n%s", numRead, buffer);
                    SSL_write(rwConns[i], "HTTP/1.1 200 OK\nContent-Length: 5\n\nhello\n", 41);
                }
            }
        }
    }
    closesocket(serverSocket);
    WSACleanup();
    return 0;
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