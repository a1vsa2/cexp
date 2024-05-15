#include <iostream>
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define SERVER_CERT_FILE "d:/ztestfiles/security/server.crt"
#define SERVER_KEY_FILE "d:/ztestfiles/security/server.key"
#define ROOT_CA_FILE "d:/ztestfiles/security/rootCA.crt"

DWORD WINAPI serverAccpetThread(LPVOID lparam);

SSL_CTX *ctx;
SSL *ssl;
BIO *accept_bio;

SSL_CTX *create_context() {
    // // 初始化 OpenSSL 库
    // SSL_library_init();
    // OpenSSL_add_all_algorithms();
    // SSL_load_error_strings();
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
    // 加载证书和私钥
    if (SSL_CTX_use_certificate_file(ctx, SERVER_CERT_FILE, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, SERVER_KEY_FILE, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    // 验证客户端证书
    // SSL_CTX_load_verify_locations(ctx, ROOT_CA_FILE, NULL);
    // SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
}

int blockingAccept(const char* port) {
    // 创建 BIO
    accept_bio = BIO_new_accept(port);
    if (!accept_bio) {
        std::cerr << "Error creating accept BIO" << std::endl;
        return -1;
    }

    // 设置 BIO
    if (BIO_do_accept(accept_bio) <= 0) {
        std::cerr << "Error setting up accept BIO" << std::endl;
        return -1;
    }

    HANDLE th = CreateThread(0, 0, serverAccpetThread, 0, 0, 0);
    WaitForSingleObject(th, INFINITE);
    return 0;
}
int startSSLServer(const char* port) {
    ctx = create_context();
    configure_context(ctx);

    blockingAccept("8888");
    return 0;
}


DWORD WINAPI serverAccpetThread(LPVOID lparam) {
    BIO *client_bio;
    char buf[256] = {0};
    while (1) {
        // 等待客户端连接
        if (BIO_do_accept(accept_bio) <= 0) {
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }

        // 获取连接的 SSL 对象
        client_bio = BIO_pop(accept_bio);
        if (!client_bio) {
            std::cerr << "Error getting client BIO" << std::endl;
            continue;
        }

        // 创建 SSL 对象
        ssl = SSL_new(ctx);
        if (!ssl) {
            std::cerr << "Error creating SSL object" << std::endl;
            continue;
        }

        // 将 SSL 与 BIO 关联
        SSL_set_bio(ssl, client_bio, client_bio);

        // 建立 SSL 连接
        if (SSL_accept(ssl) <= 0) {
            std::cerr << "Error accepting SSL connection" << std::endl;
            SSL_free(ssl);
            continue;
        }
        SSL_read(ssl, buf, 256);
        printf("recv msg: %s\n", buf);
        // CreateThread(0, 0, taskThread, 0, 0, 0);
        SSL_write(ssl, "HTTP/1.1 200 OK\nContent-Length: 5\n\nhello\n", 42);
        // 关闭 SSL 连接
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }

    // 关闭 accept_bio 和 ctx
    BIO_free(accept_bio);
    SSL_CTX_free(ctx);
}

int main() {
    startSSLServer("8888");
    return 0;
}