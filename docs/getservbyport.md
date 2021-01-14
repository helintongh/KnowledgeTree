## 函数原型
```c
#include <netdb.h>

struct servent *getservbyport(int port, const char *proto);
```
## 描述
查询服务数据库，通过服务的端口来获取服务的信息。返回servent结构
## 参数
|参数|类型|描述|
|----|----|-----
|port|int|端口号
|proto|const char*|协议名（tcp或udp），如果该参数为NULL则自动匹配
## 返回值
错误或读到/etc/services文件结尾则返回NULL，成功返回对应的servent指针