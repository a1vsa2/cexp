#include <stdio.h>
#include <winsock2.h>

#include <openssl/ssl.h>
#include <openssl/err.h>


void my_keylog_callback(const SSL *ssl, const char *line) {
    // 将对称密钥记录到文件
    FILE *fp = fopen("d:/ztestfiles/security/ssl-key.log", "a");
    if (fp) {
        fprintf(fp, "%s\n", line);
        fclose(fp);
    }
}

int main() {
    // 初始化Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    // 创建socket
    SOCKET sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == INVALID_SOCKET) {
        fprintf(stderr, "Error creating socket\n");
        WSACleanup();
        return 1;
    }

    // 设置SSL库
    SSL_library_init();
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    if (!ctx) {
        fprintf(stderr, "Error creating SSL context\n");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    // 创建SSL对象
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);

    // 连接到服务器
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(7777);
    connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    // 进行SSL握手
    if(SSL_connect(ssl) != 1) {
        fprintf(stderr, "Error during SSL handshake\n");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }
    SSL_CTX_set_keylog_callback(ctx, my_keylog_callback);
    
    Sleep(5000);
    // 发送数据
    char *msg = \
        "GET / HTTP/1.1\r\n\
        Host: localhost\r\n\
        \r\n";
    SSL_write(ssl, msg, strlen(msg));

    // 接收数据
    char buffer[1024];
    int bytes = SSL_read(ssl, buffer, sizeof(buffer));
    buffer[bytes] = '\0';
    printf("Received: %s\n", buffer);


    // 清理资源
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    closesocket(sockfd);
    WSACleanup();

    return 0;
}