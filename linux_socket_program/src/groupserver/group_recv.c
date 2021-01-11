/*
组播接收程序
编译: gcc -o groupc group_recv.c
执行: ./groupc port
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
char data_buffer[BUF_LEN]="group recv message";

//组播IP地址
char group_ip[30]="224.10.10.1";


int group_socket = 0;  //组播socket
unsigned short  group_port = 0;//保存组播端口号

struct sockaddr_in group_addr;  //组播地址

struct sockaddr_in peer_addr;   //对方IP地址

int main(int argc, char *argv[])
{
    int ret = 0;
	
    if(argc !=2 )
    {
    	printf("parameter error!\n");
    	printf("usage: ./groupc port\n");
    	return 0;
    }

    group_port = (unsigned short)atoi(argv[1]);

    // 创建组播socket
    group_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(group_socket < 0)
    {
        printf("create group socket error!\n");
    	return 0;
    }
    // 绑定组播的ip和端口号(ip是224.10.10.1)
    memset(&group_addr, 9, sizeof(struct sockaddr_in));

    group_addr.sin_family = AF_INET;
    group_addr.sin_port = htons(group_port);
    group_addr.sin_addr.s_addr = inet_addr(group_ip);

    int addr_len = sizeof(struct sockaddr_in);
    
    ret = bind(group_socket, (struct sockaddr*)&group_addr, addr_len);
    if(ret < 0)
    {
        printf("bind error \n");
        return 0;
    }

    // 将该网络接口加入多播组
    struct ip_mreq mreq;
    memset((unsigned char *)&mreq,0, sizeof(struct ip_mreq));

    mreq.imr_multiaddr.s_addr = inet_addr(group_ip);/*多播地址*/
    mreq.imr_interface.s_addr = INADDR_ANY; /*将本机任意网络接口*/

    /*改变group_socket的属性,将其加入多播组*/
    ret = setsockopt(group_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    if(ret < 0)
    {
        perror("setsockopt():IP_ADD_MEMBERSHIP");
        return -4;
    }

    int data_len = 0;

    // 接收组播数据
    while(1)
    {
        memset(data_buffer, 0, BUF_LEN);
        // 从组播客户端peer_addr接收数据
        data_len = recvfrom(group_socket, data_buffer, BUF_LEN, 0, (struct sockaddr*)&peer_addr, &addr_len);

        printf("len:%d data:%s\n", data_len, data_buffer);

        strcpy(data_buffer, "group recv message");
        // 发送数据到peer_addr
        data_len = sendto(group_socket, data_buffer, strlen(data_buffer), 0,(struct sockaddr*)&peer_addr, addr_len);
        printf("send len:%d\n",data_len);
    }

    return 0;
}