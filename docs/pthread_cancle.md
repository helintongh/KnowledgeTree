杀死(取消)线程
## 函数原型

```c
#include <pthread.h>

int pthread_cancel(pthread_t thread);
```
## 参数
thread是要杀死的线程的id号
## 能够杀死线程的时间点
取消点:有系统调用的地方(`write, open, printf`).

注: 要杀死的子线程对应的处理的函数的内部至少做过一次系统调用 `pthread_testcancle()`函数可用来设置取消点

