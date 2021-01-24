## 函数原型

mmap函数是用来创建文件映射的内存映射区的函数。

```c
#include <sys/mman.h>

void *mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset)
```

|     参数   |     描述   |
| --------- | ---------- |
| addr | 映射区首地址,一般为NULL|
| length|映射区的大小|
|prot|映射区的权限,这里要与通过open()创建的文件保持一致|
|flags|标志位参数|
|fd|文件描述符|
|offset|映射文件的偏移量|

成员解读:

* length	
	* 如果分配100bytes--实际分配4k(仅能分配4k或4k倍数大小的空间)
	* 不能为0
	* 一般文件多大,length就多大
* prot
	* PROT_READ 映射区必须有读权限
	* PROT_WRITE 映射区写权限
	* PROT_READ | PROT_WRITE 读写权限皆有
* flags
	* MAP_SHARED 修改了内存数据会同步到磁盘
	* MAP_PRIVATE 修改了内存数据不会同步到磁盘
* fd
	* 通过open()得到文件描述符
* offset
	* 映射的时候文件指针的偏移量
	* 必须为4k倍数
	* 0(从头开始,一般取此值)

## 返回值

调用成功,返回映射区的首地址。

调用失败,返回-1并设置errno为MAP_FAILED

