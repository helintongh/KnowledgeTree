epoll是红黑树结构,是Linux特有的API。max_user_watches上限是内核提供一个接口来定义每个用户可以注册到epoll实例上的文件描述符总数。
## 结构体
### epoll_event
```c
typedef union epoll_data {
    void *ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;

struct epoll_event {
    __uint32_t events; /* Epoll events */
    epoll_data_t data; /* User data variable */
};
```
epoll_create生成的epfd，是内核中epoll结构的唯一标识。**epoll结构不直接面向应用程序员**。它维持着每个epoll处理要监视的fd及其感兴趣事件。要修改它只能通过epoll_ctl。

## 主要函数

|函数|描述
|----|----
|[epoll_create](#epoll_create)|创建一个epoll的文件描述符
|[epoll_ctl](#epoll_ctl)|epoll的事件控制函数
|[epoll_wait](#epoll_wait)|收集在epoll监控的事件中已经发送的事件

### epoll_create
系统调用epoll_create创建一个新的epoll实例,其对应的兴趣列表初始化为空。
```c
#include <sys/epoll.h>
int epoll_create(int size);
```
* 参数:
	* size:epoll上能关注的文件描述符个数(不是上限,而且告诉内核如何为内部数据结构划分初始大小)

* 返回
	* 新创建的epoll实例的文件描述符。(epoll树根结点)不用后要使用`close()`关闭。

### epoll_ctl
系统调用epoll_ctl能够修改由文件描述符epfd所代表的epoll实例中的感兴趣列表。(可以注册,修改,删除某个epoll fd)
```c
#include <sys/epoll.h>
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *ev);
```

* 参数:
	* epfd:epoll_create生成的epoll专用fd
	* fd:指明了要修改兴趣列表中的哪一个文件描述符。(可以是pipe,fifo,socket,POSIX消息队列等等)
	* op:用来指定需要执行的操作
		* EPOLL_CTL_ADD 将fd注册到epoll实例epfd的兴趣列表中
		* EPOLL_CTL_MOD 修改描述符fd设定的时间,需要用到由ev所指向的结构体的信息
		* EPOLL_CTL_DEL 将fd从epfd的兴趣列表中移除。
	* ev:指向结构体epoll_event的指针,结构体上面介绍了一般不直接面向程序员
		* 结构体epoll_event中的events字段是一个位掩码,指定为待检查的描述符fd上所感兴趣的事件集合。
		* data字段是一个联合体,当fd成为就绪态时,联合体的成员可用来指定传回给调用进程的信息。
* 返回:
	* epoll实例的文件描述符。(epoll树根结点)
```c
// 使用epoll_create和epoll_ctl
int epfd;
struct epoll_event ev;

epfd = epoll_create(5);
if(epfd == -1)
	printf("epoll_create error\n");

ev.data.fd = fd;
ev.events = EPOLLIN;
if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, ev) == -1)
	printf("epoll_ctl error\n");
```

### epoll_wait
系统调用epoll_wait返回epoll实例中处于就绪态的文件描述符信息。单个epoll_wait调用能返回多个就绪态文件描述符信息。
```c
#include <sys/epoll.h>
int epoll_wait(int epfd, struct epoll_event *evlist, int maxevents, int timeout);
```
* 参数:
	* epfd:要检测的epoll实例
	* evlist:指向的结构体数组中返回的是有关就绪态文件描述符的信息。
	* maxevents:数组evlist的空间由调用者负责申请,所包含的元素个数在参数maxevents中指定。
	* timeout:超时时间
		* -1 永久阻塞,直到感兴趣列表中的文件描述符有事件产生。
		* 0 立即返回。执行一次非阻塞的检测,看兴趣列表中的文件描述符上产生了哪个事件。
		* >0 调用将阻塞至多timeout毫秒,直到文件描述符上有事件发生。
* 返回:
	* 调用成功后,数组evlist中的元素个数。如果在timeout超时事件间隔内没有任何文件描述符处于就绪态,返回0。出错时返回-1,并设置errno。

## epoll事件

当调用epoll_ctl时可以在ev.events中指定的位掩码以及epoll_wait返回的evlist[].events中的值在下标

|位掩码|作为epoll_ctl()的输入|由epoll_wait()返回|描述
|------------|-------------|------------------|--------
|EPOLLIN     | :heavy_check_mark:    |   :heavy_check_mark:    |可读取非高优先级的数据
|EPOLLPRI    |  :heavy_check_mark:   |  :heavy_check_mark:     |可读取高优先级数据
|EPOLLRDHUP  | :heavy_check_mark:    |  :heavy_check_mark:     |套接字对端关闭
|EPOLLOUT    |  :heavy_check_mark:   |   :heavy_check_mark:    |普通数据可写
|EPOLLET     |  :heavy_check_mark:   |                         |采用边缘触发事件通知
|EPOLLONESHOT| :heavy_check_mark:    |                         |在完成事件通知之后禁用检查
|EPOLLERR    |                       |   :heavy_check_mark:    |有错误发生
|EPOLLHUP    |                       |  :heavy_check_mark:     |出现挂断

epoll快的原因: 

1. 只有IO操作执行使得fd变为就绪态时,内核才在epoll描述符就绪列表中添加一个元素。(一开始不检查所有)
2. epoll通过epoll_ctl在内核空间建立数据结构,稍后的epoll_wait调用不需要传递任何与文件描述符有关的信息给内核。

## 边缘触发

```c
struct epoll_event ev;
ev.data.fd = fd;

ev.events = EPOLLIN | EPOLLET;
if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, ev) == -1)
	// 差错处理
```

水平和边缘触发区别:
假设我们使用epoll来监视一个套接字上的输入(EPOLLIN)

1. 套接字上有输入到来。
2. 调用一次epoll_wait()。无论采取水平还是边缘触发通知,该调用都会告诉我们套接字已经处于就绪态。
3. 再次调用epoll_wait().
如果是水平触发,那么第二个epoll_wait()调用将告诉我们套接字处于就绪态。而如果采用边缘触发通知,那么第二次epoll_wait()将阻塞,因为自从上一次调用epoll_wait()以来并没有新的输入到来。

边缘触发通常和非阻塞的文件描述符使用。因而,采用epoll边缘触发通知机制的程序基本框架如下:
1. 让所有待监视的文件描述符都成为非阻塞的。
2. 通过epoll_ctl()构建epoll的兴趣列表(指定边缘触发)
3. 通过如下循环处理IO事件:
	* 通过epoll_wait()取得处于就绪态的描述符列表
	* 针对每一个处于就绪态的文件描述符,不断进行IO处理直到相关的系统调用(例如read(),write(),recv(),send()或accept())返回EAGAIN或EWOULDBLOCK错误。

epoll带来了饥饿问题解决:

1. 设置timeout为0或大于0

2. 只在已经注册为就绪态的fd上进行一定限度的IO操作。
