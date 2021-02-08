为了回收资源，主线程会等待子线程结束。该函数就是用来等待线程终止的。类似与进程中的wait函数。  
此函数将阻塞等待线程退出并获取线程退出状态，直到此线程退出。  
## 函数原型
```c
#include <pthread.h>

int pthread_join(pthread_t thread, void **retval);
```
## 参数
### thread
被等待要回收的子线程的ID
### retval
如果此值非NULL，此值保存线程退出时携带的状态信息
	* void \*ptr
	* `pthread_join(pthid, &ptr);` ptr为传出参数
	* 指向的内存和`pthread_exit`参数指向同一块内存地址

## 返回值
成功返回0，失败返回非0
