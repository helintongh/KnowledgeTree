## 线程中的信号量

信号量可以叫做高级的互斥锁。它能一次性给出多把锁。

## 信号量类型

```c
#include <semaphore.h>

sem_t sem;
```

## 主要函数

- 初始化信号量

```c
#include <semaphore.h>

int sem_init(sem_t *sem, int pshared, unsigned int value);
```


|形参|描述
|---|-----
|sem|指向要初始化的信号量指针
|pshared|指示是否要使用这个信号量在进程的线程之间或进程之间共享。为0则是线程同步,为1是进程同步
|value|value指定信号量的初始值。最多几个线程可同时共享数据

- 销毁信号量

```c
#include <semaphore.h>

int sem_destroy(sem_t *sem);
```

- 加锁

```c
#include <semaphore.h>

int sem_wait(sem_t *sem); 
```
调用一次相当于对sem做了--操作(value--)
如果sem(sem中的value)值为0,线程会阻塞

- 尝试加锁

```c
#include <semaphore.h>

int sem_trywait(sem_t *sem);
```

如果sem值为0,加锁失败,不阻塞,直接返回。

- 限时尝试加锁

```c
#include <semaphore.h>

int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
```

- 解锁

```c
#include <semaphore.h>

int sem_post(sem_t *sem);
```

对sem做++操作。