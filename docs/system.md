### 函数原型

```c
#include <stdlib.h>

int system(const char *command);
```

使用用例:

`system(ls);` 该代码实际上是通过fork创建子进程执行ls指令。

在执行`system()`函数(命令)期间，SIGCHLD信号将被阻塞在调用的过程中，SIGINT和SIGQUIT信号将被忽略。