#include <stdio.h>
#include <winsock2.h>


int main() {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in serverAddr;
    const char* message = "Hello, UDP Server!";
    int serverAddrSize = sizeof(serverAddr);

    // 初始化 Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    // 创建 UDP 套接字
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        printf("Could not create socket : %d", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // 设置服务器地址
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 服务器的 IP 地址
    serverAddr.sin_port = htons(8888); // 服务器的端口号

    // 向服务器发送数据
    if (sendto(sock, message, strlen(message), 0, (struct sockaddr*)&serverAddr, serverAddrSize) == SOCKET_ERROR) {
        printf("sendto() failed with error code : %d", WSAGetLastError());
    } else {
        printf("Data sent\n");
    }

    // 关闭套接字
    closesocket(sock);

    // 清理 Winsock
    WSACleanup();

    return 0;
}