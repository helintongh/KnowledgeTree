//TCP 客户端程序

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

//.tcpclient  ServerIP  Port

const int buffer_len = 128;

char server_ip[50] = {0}; // 保存serverip命令行参数的值
unsigned short server_port = 0; // 保存port命令行参数的值
int client_socket = 0; // 连接socket,用来保存客户端(accept函数)的返回值

int main(int argc, char *argv[])
{
 	int ret = 0;

	if(argc != 3)
	{
		printf("parameter number error\n");
		printf("usage: ./tcpserver serverIP port\n");

		return 0;
	}

	strcpy(server_ip, argv[1]);
	server_port = atoi(argv[2]);

	//创建socket
	client_socket = socket(AF_INET,SOCK_STREAM,0);
	if(client_socket == 0)
	{
		printf("client create socket error\n");
		return 0;
	}


	struct sockaddr_in server_address;
	int addr_len = sizeof(struct sockaddr_in);
	memset(&server_address,0, sizeof(struct sockaddr_in));

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(server_port);
	server_address.sin_addr.s_addr = inet_addr(server_ip);

	ret = connect(client_socket, (struct sockaddr *)&server_address, addr_len);

	if(ret < 0)
	{
		printf("connect error\n");
		return 0;
	}

	printf("connect server ok\n");


	int data_len = 0;
	char data_buffer[buffer_len];

	//客户端连接成功，开始发送和接收数据,服务器先接收数据
	while(1)
	{

		memset(data_buffer, 0, buffer_len);
		strcpy(data_buffer, "I am client");
		data_len = send(client_socket,data_buffer, strlen(data_buffer),0);

		printf("send return :%d\n", data_len);

	
		memset(data_buffer, 0, buffer_len);
		data_len = recv(client_socket,data_buffer, buffer_len,0);

		printf("client receive data:%s  recv return:%d\n", data_buffer, buffer_len);
		
		sleep(2);
	   
	}
	
}