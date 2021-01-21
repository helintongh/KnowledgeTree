创建一个特殊或普通的文件。(进程通信中创建有名管道文件的函数)

## 函数原型

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int mknod(const char *pathname, mode_t mode, dev_t dev);
```

## 参数

- pathname: 创建的文件结点(可以是文件, 设备驱动文件或者有名管道)

- mode: 文件访问权限

- dev: 如果文件类型为**S_IFCHR**或**S_IFBLK**，则dev指定新创建的设备专用文件的主、副编号(makedev(3)可能用于为dev构建值);否则**它将被忽略**。

## 使用样例

```c
mknod("myfifo", S_IFIFO | 0644 , 0);
/*
在上面的例子中，FIFO文件将被称为“myfifo”。

第二个参数是创建模式，它用于告诉mknod()创建一个FIFO(s_iffifo)并设置该文件的访问权限(octal 644，或rw-r——r——)，也可以通过将sys/stat.h中的宏集合在一起来设置。这个权限与使用chmod命令设置的权限类似。

最后，传递一个设备号。在创建FIFO时，这将被忽略，因此可以在其中放入任何想要的内容。
*/
```