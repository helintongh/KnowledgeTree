#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>  
#include <unistd.h>
#include <string.h>

/*TCP 服务器程序
编译指令: gcc -o tcpserver
使用如下:
./tcpserver serverip  port

*/

const int buffer_len = 128;

char server_ip[50] = {0}; // 保存serverip命令行参数的值
unsigned short server_port = 0; // 保存port命令行参数的值
int server_socket = 0; // 服务socket,用来接收客户端的连接请求
int connect_socket = 0; // 连接socket,用来保存客户端(accept函数)的返回值

int main(int argc, char *argv[])
{
    int ret = 0;

    if(argc != 3) // 需要三个参数，参数小于3说明出错了
    {
        printf("parameter number error\n");
		printf("usage: ./tcpserver serverIP port\n");

		return 0;
    }

    strcpy(server_ip, argv[1]); // 将输入的参数2拷贝到server_ip中
    server_port = atoi(argv[2]);// 字符串转换从整数,参数3传入server_port

    // 创建socket
    server_socket = socket(AF_INET,SOCK_STREAM,0);
    if(server_socket == 0) // 创建socket失败的处理
    {
        printf("create socket error\n");
		return 0;
    }
    
    // 给服务器需要的socket地址等信息分配空间
    struct sockaddr_in server_address; // 保存服务器的socket地址
    int address_len = sizeof(struct sockaddr_in); // 获取地址长度
    memset(&server_address, 0, sizeof(struct sockaddr_in)); // 初始化为0

    // 初始化服务器的IP地址和Port
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    server_address.sin_addr.s_addr = inet_addr(server_ip);

    // 绑定socket
    ret = bind(server_socket, (struct sockaddr *)&server_address, address_len);
    if(ret < 0)
    {
        printf("bind error\n");
		return 0;
    }

    // 监听socket
    ret = listen(server_socket, 20);
    if(ret < 0)
    {
        printf("listen error\n");
        return 0;
    }

    printf("server is listening!!!\n");

    // 用来保存客户端的socket地址
    struct sockaddr_in client_address;
    memset(&client_address, 0, sizeof(struct sockaddr_in));
    
    char client_ip[50] = {0}; // 保存客户端socket的IP
    unsigned short client_port = 0; // 保存客户端的port
    // 调用accpet函数来处理客户端连接请求
    connect_socket = accept(server_socket, (struct sockaddr*)&client_address, &address_len);

    client_port = ntohs(client_address.sin_port);
    strcpy(client_ip, inet_ntoa(client_address.sin_addr));

    printf("new client connect:IP: %s   Port:%d\n",client_ip,client_port);

    if(connect_socket < 0) // 如果小于0说明处理客户端连接请求失败
    {
        printf("accept error\n");
		return 0;
    }

    int data_len = 0;

    char data_buffer[buffer_len];

    // 客户端连接成功,开始发送和接受数据,进入服务器的处理流程
    while(1)
    {
        memset(data_buffer, 0, buffer_len);
        data_len = recv(connect_socket, data_buffer, buffer_len, 0);

        printf("server receive data:%s  recv return:%d\n", data_buffer, data_len);

        memset(data_buffer, 0, buffer_len);
		strcpy(data_buffer, "I am Server");
		data_len = send(connect_socket,data_buffer, strlen(data_buffer),0);
    }

    return 0;

}