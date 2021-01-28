结束一个线程
## 函数原型
```c
#include <pthread.h>

void pthread_exit(void *retval);
```
## 参数
retval用来保存线程退出状态。该指针必须为全局变量或者heap堆上的是传出参数。
## 返回值
为空。因为该函数**永远成功**。

注: 子线程退出不能使用`exit()` 会导致整个进程退出。