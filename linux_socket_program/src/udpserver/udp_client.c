/*
udp 客户端程序
编译命令: gcc -o udpc udp_client.c
执行:
./udpc ip port
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>  
#include <unistd.h>
#include <string.h>

#define BUF_LEN 128 

char server_ip[50] = {0}; // 保存服务器ip
unsigned short server_port = 0; // 服务器端口号
int client_socket = 0; // 保存客户端的socket文件描述符


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
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(client_socket == 0)
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

    // udp客户端的发送请求到服务器
    int data_len = 0;
    char data_buffer[BUF_LEN];
    while(1)
    {
        memset(data_buffer, 0, BUF_LEN);
        strcpy(data_buffer, "I am udp client");
        addr_len = sizeof(struct sockaddr_in);
        data_len = sendto(client_socket, data_buffer, strlen(data_buffer), 0, (struct sockaddr_in *)&server_address, addr_len);

        printf("send len:%d\n", data_len);
        sleep(2);
    }

    return 0;
}