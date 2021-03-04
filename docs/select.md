## 函数原型
```c
#include <sys/select.h>

int select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout);
```
## 参数
### nfds
指定被监听的文件描述符的总数。它通常被设置为select()监听的最大文件描述符加1（文件描述符从0计数）
### readfds、writefds、exceptfds
分别指向可读、可写和异常事件对应的文件描述符集合。这三个参数都是[fd_set](#fd_set)类型。 

* readfds是用来检测输入是否就绪的文件描述符集合
* writefds是用来检测输出是否就绪的文件描述符集合
* exceptfds是用来检测异常情况是否发生的文件描述符集合。(会有两种情况,流式套接字上接收到了带外数据,连接到处于信包模式下的伪终端主设备上的从设备状态发生了改变)

这三个参数是**值-结果**参数，在select()返回的的时候，将把就绪的fd更新到对应的集合中。
### timeout
设置select()超时时间。
- NULL: select()将无限等待。
- 非NULL: select()等待相应秒数，若无就绪则超时返回
  - **特例**非NULL，但其值为0秒： select()检查完fd集合后立即返回。(简单轮询)

这是个**值-结果参数**，select()成功返回时，内核将修改它的值为**剩余**的时间。
但当该调用失败的时候，它返回的值是不确定的。该结构体（timeval）提供微秒级的分辨率。
## 返回值
- 成功时返回就绪（可读、可写和异常）文件描述符的总数。每个返回的文件描述符集合都需要检查(通过FD_ISSET())以此找出发生的IO事件是什么。因为有可能同一个fd在readfds,writefds,exceptfds中同时被指定。
- 如果在超时时间内没有任何文件描述符就绪，select()将返回0。
- 失败时返回-1，并设置errno。

如果在select()等待期间，程序接收到信号，则select()立即返回-1，并设置errno为**EINTR**。

----
### fd_set
fd_set以掩码形式实现,使用不需要直到细节。
一个结构体名（被typedef命名，所以使用时不加struct）。实际上里面只包含一个整型（long int）数组。  
该数组的每一位标记一个文件描述符。可以通过以下**函数宏**来访问fd_set结构体中的位。

| 返回值|函数宏|描述
|-----|----|----- 
void|**FD_ZERO**(fd_set *set)|将fdset所指向的集合初始化为空
void|**FD_SET**(int fd, fd_set *set)|将文件描述符fd添加到由fdset所指向的集合中
void|**FD_CLR**(int fd, fd_set *set)|将文件描述符fd从fdset所指向的集合中移除
int |**FD_ISSET**(int fd, fd_set *set)|如果文件描述符fd是fdset所指向的集合中的成员,FD_ISSET()返回true

在使用fd_set类型的变量的时候，一定要用`FD_ZERO`初始化。
文件描述符集合有一个最大容量的限制,由常量FD_SETSIZE来决定通常为1024。