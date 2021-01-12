/*
广播发送消息程序
编译指令: gcc -o bsend broad_send.c
执行指令: ./bsend port
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

char data_buffer[BUF_LEN] = "broad send message";

int broad_socket = 0; // 保存广播的发送socket
unsigned short broad_port = 0; // 保存广播的端口号

struct sockaddr_in broad_addr; // 广播地址
struct sockaddr_in  peer_addr;   //对方IP地址

char broad_ip[30]="192.168.0.255"; // 广播地址 -> 广播地址要通过ifconfig指令查看究竟是哪个地址然后自行进行设置

int main(int argc, char *argv[])
{
    int ret = 0;
	
    if(argc !=2 )
    {
    	printf("parameter error!\n");
    	printf("usage: ./broadSend port\n");
    	return 0;
    }
    broad_port = (unsigned short)atoi(argv[1]);
    // 创建广播socket
    broad_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(broad_socket < 0)
    {
    	printf("create socket error!\n");
    	return 0;
    }
    
    memset(&broad_addr,0, sizeof(struct sockaddr_in));

    //绑定socket
    broad_addr.sin_family = AF_INET;
    broad_addr.sin_port = htons(broad_port);
    broad_addr.sin_addr.s_addr = INADDR_BROADCAST;//inet_addr(broad_ip); //inet_addr(strHostIP);

    // 设置socket广播属性
    int en_broadcast = 1; // 支持广播
    ret = setsockopt(broad_socket, SOL_SOCKET, SO_BROADCAST, &en_broadcast, sizeof(int));
    if(ret < 0)
    {
        printf("setsockopt error\n");
        return 0;
    }

    int addr_len = sizeof(struct sockaddr_in);

    int data_len = 0;
    // 发送广播报文
    while(1)
    {
        strcpy(data_buffer, "broad send message");
        // 发送广播报文
        data_len = sendto(broad_socket,data_buffer, strlen(data_buffer),0,(struct sockaddr *)&broad_addr, addr_len);
        printf("send len:%d\n", data_len);

        memset(data_buffer, 0, BUF_LEN);
        data_len = recvfrom(broad_socket,data_buffer, BUF_LEN, 0,(struct sockaddr *)&peer_addr, &addr_len);


        printf("recv len:%d data:%s\n",data_len, data_buffer);
        sleep(1);
    }

    return 0;
}


