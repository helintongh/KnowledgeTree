## 函数原型

```c
#include <signal.h>

int sigaltstack(const stack_t *ss, stack_t *old_ss);
```

作用是:设置和/或获取信号堆栈上下文

此函数不常用,但需要了解。