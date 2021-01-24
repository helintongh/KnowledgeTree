## 函数原型

```c
#include <sys/mman.h>
int munmap(void *addr, size_t length);
```

函数作用是释放内存映射区

* addr -- mmap的返回值,映射区首地址
* length -- mmap的第二个参数,映射区长度 