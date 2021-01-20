### 函数原型
```c
#include <sys/types.h>
#include <sys/wait.h>

pid_t waitpid(pid_t pid,int * status,int options);
```

### 函数作用

同wait函数,但是可以指定回收某个进程。

### 返回值
|-1 |错误|
|---|---|
|>0|被终止的子进程的id|
|=0|参数options设置未WNOHANG,此时函数为非阻塞。且子进程正在运行|

参数pid和status同wait函数,options设置为0是阻塞函数, `WNOHANG`时是非阻塞函数。