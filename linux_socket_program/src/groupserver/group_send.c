/*
组播发送程序
编译: gcc -o groups group_send.c
执行: ./groups port
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

#define BUF_LEN  128
char data_buffer[BUF_LEN] = "group send message";

//组播IP地址
char group_ip[30]="224.10.10.1";


int group_socket = 0;  //组播socket
unsigned short  group_port = 0;//保存组播端口号

struct sockaddr_in group_addr;  //组播地址

struct sockaddr_in peer_addr;   //对方IP地址

int main(int argc, int *argv[])
{
    int ret = 0;
    
    if(argc != 2)
    {
        printf("parameter error!\n");
    	printf("usage: ./groups port\n");
    	return 0;
    }

    group_port = (unsigned short)atoi(argv[1]);

    group_socket = socket(AF_INET ,SOCK_DGRAM, 0);
    if(group_socket < 0)
    {
        printf("create group_socket error!\n");

    	return 0;
    }

    // 初始化组播ip和端口号
    memset(&group_addr, 0, sizeof(struct sockaddr_in));

    group_addr.sin_family = AF_INET;
    group_addr.sin_port = htons(group_port);
    group_addr.sin_addr.s_addr = inet_addr(group_ip);

    int addr_len = sizeof(struct sockaddr_in);
    int data_len = 0;

    // 组播发送数据
    while(1)
    {
        strcpy(data_buffer, "group send message!");
        data_len = sendto(group_socket, data_buffer, strlen(data_buffer), 0, (struct sockaddr *)&group_addr, addr_len);

        printf("group send len:%d\n", data_len);
        
        // 接收数据
        data_len = recvfrom(group_socket, data_buffer, BUF_LEN, 0, (struct sockaddr*)&peer_addr, &addr_len);

        printf("group recv len:%d  data:%s\n", data_len, data_buffer);

        sleep(2);

    }

    return 0;
}