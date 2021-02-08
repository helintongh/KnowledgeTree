用来设置线程属性的一系列函数。
## 函数原型
```c
pthread_attr_t attr; // 用来设置线程属性的类型
```
## 线程属性操作函数
### 对线程属性变量的初始化
```c
int pthread_attr_init(pthread_attr_t* attr);
```
### 设置线程属性函数
```c
int pthread_attr_setdetachstate(pthread_attr_t* attr, int detachstate);
```
参数:
attr: 线程属性
detachstate:
	- **PTHREAD_CREATE_DETACHED** 分离属性
	- **PTHREAD_CREATE_JOINABLE** 阻塞属性

### 释放线程属性设置函数
```c
int pthread_attr_destroy(pthread_attr_t* attr);
```