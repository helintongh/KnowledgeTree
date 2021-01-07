/*
udp 服务器程序
编译命令: gcc -o udps udp_server.c
执行:
./udps ip port
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>  
#include <unistd.h>
#include <sys/wait.h>

#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>


#define BUF_LEN 128 

char server_ip[50] = {0}; // 保存服务器ip
unsigned short server_port = 0; // 服务器端口号
int server_socket = 0; // 保存服务器的socket文件描述符


int main(int argc, int *argv[])
{
    int ret = 0;

    if(argc != 3)
    {
        printf("parameter number error\n");
		printf("usage: ./upds ip port\n");
		return 0;
    }

    strcpy(server_ip, argv[1]);
    server_port = atoi(argv[2]);

    // 创建udp socket
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_socket == 0)
    {
        printf("create udp socket error\n");
        return 0;
    }

    struct sockaddr_in server_address;
    int addr_len = sizeof(struct sockaddr_in);
    memset(&server_address, 0, sizeof(struct sockaddr_in));

    // 填充sockaddr_in
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    server_address.sin_addr.s_addr = inet_addr(server_ip);

    // 绑定socket
    ret = bind(server_socket, (struct sockaddr_in*)&server_address, addr_len);
    if(ret < 0)
    {
        printf("bind error\n");
		return 0;
    }

    // 接收udp客户端请求
    int data_len = 0;
    char data_buffer[BUF_LEN];
    char client_ip[50] = {0};
    unsigned short client_port = 0;
    struct sockaddr_in client_address;
    memset(&client_address, 0, sizeof(struct sockaddr_in));

    while(1)
    {
        memset(data_buffer, 0, BUF_LEN);

        data_len = recvfrom(server_socket, data_buffer, BUF_LEN, 0, (struct sockaddr*)&client_address, &addr_len);
        
        client_port = ntohs(client_address.sin_port); // 获取连接到服务器的客户端的端口号

        strcpy(client_ip, inet_ntoa(client_address.sin_addr)); // 获取客户端的ip转为点分十进制
        
        printf("recv len:%d data: %s from ip %s:%d \n", data_len, data_buffer, client_ip, client_port);
    }

    return 0;
}