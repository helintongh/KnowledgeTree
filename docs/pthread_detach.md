每生成一个线程就要回收它是不是很麻烦了，pthread还提供了一种分离线程的办法。分离线程无法被回收和杀死,它的资源在它终止时由系统自动释放。
分离一个线程。
## 函数原型
```c
#include <pthread.h>

int pthead_detach(pthread_t thread);
```
## 参数
### thread
要分离的子线程的ID
## 返回值
成功返回0，失败返回非0

注:

- 调用该函数后不需要pthead_join或pthead_exit

- 子线程会自动回收自己的pcb