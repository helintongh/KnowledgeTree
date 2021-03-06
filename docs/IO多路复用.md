## IO模型

Linux有5种IO,阻塞IO,非阻塞IO,多路复用IO,信号驱动IO以及异步IO(AIO)。

什么是IO?IO就是输入输出操作,具体一点是从文件里读取数据和向输出文件里写数据

注:Linux一切皆文件,这里的文件也可理解为设备

阻塞IO: 单个进程每次只在一个文件描述符上执行IO操作,每次系统调用都会阻塞直到完成数据传输。比如从一个socket fd种读取数据时,如果该fd恰好没有数据,那么`read()`会阻塞。而如果fd中没有足够的空间保存待写入的数据时,`write()`也会阻塞。

非阻塞IO:可以通过设置socket属性,将其变为非阻塞。当从一个fd读取数据时,如果没有准备好不会阻塞进程而是直接返回一个error。用户进程判断是一个error时,它就能知道数据还没有准备好,于是可能再次进行`read()`操作。非阻塞IO需要不断轮询某个文件描述符是否可执行IO操作。这种轮询会造成严重的延时。

信号驱动IO:当有输入或者数据可以写道指定的文件描述符上时,内核向请求数据的进程发送一个信号。进程可以处理其他的任务,当IO操作可执行时通过接收信号来获得通知。当同时检查大量的文件描述符时,信号驱动比select和poll有巨大性能提升(边沿触发)

IO多路复用:进程同时检查多个文件描述符以找出它们中的任何一个是否可执行IO操作。(epoll有的书不放在IO多路复用里我认为epoll依然属于IO多路复用思想)

由此可见,IO多路复用和信号驱动以及epoll都是用来实现同一个目标的技术--同时检查多个文件描述符,看它们是否准备好了执行IO操作(这里更准确的是检查IO系统调用是否可以非阻塞地执行)。

文件描述符的就绪态转化是通过IO事件来触发的。比如输入数据到达,套接字连接建立完成,或者是之前满载的套接字发送缓冲区在TCP将队列中的数据传送到对端之后有了剩余空间。

需要明确这些技术不会执行实际的IO操作。它们只是告诉我们某个文件描述符已经处于就绪状态了。这时候就需要其他的系统调用来完成实际的IO操作。

epoll和信号驱动IO可以让应用程序高效地检查大量的文件描述符,epoll的优势在于:

- 避免了处理信号的复杂性
- 可以指定想要检查的事件类型(即,读就绪或者写就绪)
- 可以选择水平触发或者边缘触发

### 水平触发和边缘触发

水平触发通知: 如果文件描述符上可以非阻塞地执行IO系统调用,此时认为它已经就绪。

边缘触发通知: 如果文件描述符自上次状态检查以来有了新的IO活动(比如新的输入),此时需要触发通知。

|IO模式|水平触发|边缘触发
|------|------|-------
|select,poll| | :heavy_check_mark: 
|信号驱动IO|   | :heavy_check_mark:	
|epoll| :heavy_check_mark: | :heavy_check_mark:

水平触发程序设计方式: 可以在任意时刻检查文件描述符的就绪状态。这表示当确定了文件描述符处于就绪态时(比如存在有输入数据),就可以对其执行一些IO操作,然后重复检查文件描述符,看看是否仍然处于就绪态(比如还有更多输入数据),此时就能执行更多的IO。

由于水平触发模式允许任意时刻重复检查IO状态,没有必要每次当文件描述符就绪后需要尽可能多地执行IO。(尽可能多地读取字节)

边缘触发程序设计方式: 只有当IO事件发生时才会收到通知。在另一个IO事件到来前不会收到任何新的通知。另外,当文件描述符收到IO事件通知时,通常并不知道要处理多少IO(例如有多少字节可读)。因此其设计如下

1. 在接收到一个IO事件通知后,程序在某个时刻应该在相应的文件描述符上尽可能多地执行IO。如果没这么做,就可能会失去执行IO的机会。直到另一个IO事件为止,在此之前程序都不会再接收到通知了。

2. 如果程序采用循环来对文件描述符执行尽可能多的IO,而文件描述符又被设置为阻塞的,那么最终当没有更多的IO可执行时,IO系统的调用就会阻塞。基于此,每个被检查的文件描述符通常都应该设置为非阻塞模式,在得到IO事件通知后重复执行IO操作,直到相应的系统调用(比如`read()`,`write()`)以及错误码EAGAIN或EWOULDBLOCK的形式失败。

补充非阻塞IO与之配合的IO模型:

- 非阻塞IO通常和提供有边缘触发通知机制的IO模型一起使用。
- 如果多个进程(或线程)在同一个打开的文件描述符上执行IO操作,那么从某个特定进程的角度来看,文件描述符的就绪状态可能会在通知就绪和执行后续IO调用之间发生改变。结果就是一个阻塞式的IO调用将阻塞,从而放置进程检查其他的文件描述符。(无论采取什么触发)
- 水平触发模式通知我们流式套接字的文件描述符已经写就绪了,如果在当个`write()`和`send()`调用中写入足够大块的数据,那么该调用将阻塞。
- 有时水平触发型API比如`select()`或`poll()`,会返回虚假的就绪通知。它们会错误地通知文件描述符就绪了。

[select](./select.md)
[poll](./poll.md)
[epoll](./epoll.md)

select和poll存在的问题:

1. 每次调用select或poll,内核都必须检查所有被指定的文件描述符,看它们是否处于就绪态。检查大量处于密集范围内的文件描述符时,将花费大量的时间。

2. 每次调用select或poll,程序都必须传递一个表示所有需要被检查的文件描述符的数据结构到内核,内核检查郭描述符后,修改这个数据结构并返回给程序(select还需要在每次调用前初始化这个数据结构。)对于poll来说,随着待检查文件的文件描述符增多,传给内核的会越来越大占用大量cpu资源。

3. select或poll调用完成后,程序必须检查返回的数据结构中的每个元素,以此查明哪个文件描述符处于就绪态了。

设置socket为非阻塞:

```c
#include <unistd.h>
#include <fcntl.h>

int fcntl(int fd, int cmd, ... /* arg */ );
// fd为文件描述符,cmd为操作命令,...是供命令使用的参数
// 设置socket为非阻塞
int flag = 0;
flag = fcntl(socket_a, F_GETFL, 0); // 获取socket_a的属性
flag = flag | O_NONBLOCK;
fcntl(socket_a, F_SETFL, flag);
```