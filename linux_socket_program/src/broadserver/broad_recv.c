/*
广播接收端程序
编译指令: gcc -o brecv broad_recv.c
执行指令: ./brecv hostip port
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

char data_buffer[BUF_LEN]={0};


char host_ip[50] = {0};
unsigned short broad_port = 0;

struct sockaddr_in  host_addr;   //本机IP地址和端口号
struct sockaddr_in  peer_addr;   //对方IP地址

// 接收广播消息使用的socket
int recv_socket = 0;

int main(int argc, char *argv[])
{
    int ret = 0;
	
    if(argc !=3 )
    {
    	printf("parameter error!\n");
    	printf("usage: ./broadRecv HostIP  Port\n");
    	return 0;
    }

    memset(&host_addr,0, sizeof(struct sockaddr_in));
    memset(&peer_addr,0, sizeof(struct sockaddr_in));

    strcpy(host_ip, argv[1]);
    broad_port = (unsigned short)atoi(argv[2]);

    // 创建接收端socket
    recv_socket = socket(AF_INET, SOCK_DGRAM,0);
    if(recv_socket < 0)
    {
    	printf("create socket error!\n");

    	return 0;
    }

    //绑定socket
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(broad_port);
    host_addr.sin_addr.s_addr = INADDR_ANY; //inet_addr(strHostIP);

    int addr_len = sizeof(struct sockaddr_in);

    ret = bind(recv_socket, (struct sockaddr *)&host_addr, addr_len);
    if(ret < 0)
    {
    	printf("bind error\n");
    	return 0;
    }

    int data_len = 0;

    while(1)
    {
        memset(data_buffer, 0, BUF_LEN );
        data_len = recvfrom(recv_socket,data_buffer, BUF_LEN, 0,(struct sockaddr *)&peer_addr, &addr_len);


        printf("recv len:%d data:%s\n",data_len, data_buffer);



        //发送数据
        strcpy(data_buffer,"udp receiver");
        data_len = sendto(recv_socket,data_buffer, strlen(data_buffer),0,(struct sockaddr *)&peer_addr, addr_len);

        printf("send len:%d\n",data_len);
    }

    return 0;
}


