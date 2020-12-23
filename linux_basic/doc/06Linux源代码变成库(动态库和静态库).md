### 源代码变成库

本文主要讲解:静态库和动态库的制作和使用。

要搞清楚库要先弄明白三个问题?

1. 库是什么?

- 二进制文件

- 将源代码编译 -> 二进制格式的源代码

- 加密后的源代码

2. 如何使用库?

- 引入头文件

- 要制作出库(库有分为静态和动态,其中动态库使用更多)

动态库在linux中是`.so` 后缀的文件,而静态库是`.a`后缀的文件

#### 静态库的制作和使用

- 命名规则
	- lib作为前缀
	
	- 中间为库的名字
	
	- `.a`为后缀

比如:`libtest.a` 就是在linux下很明显的一个静态库。

制作静态库的步骤:

- 1. 准备原材料: 源代码,.c,.cpp等源文件

- 2. 将.c文件生成 .o `gcc a.c b.c -c`

- 3. 将.o文件打包

	- `ar rcs 静态库名字 原材料`

	- `ar rcs libtest.a a.o b.o`

假设有路径为下的项目
```
----lib/
|
|---include/
|---main.c
|---src/
```

src和include路径下的文件是函数的实现,main.c是主函数。lib是存放src和include打包后静态库的路径。

```Shell
gcc *.c -c -I ../include # -I是指定头文件路径,该语句是把当前路径下的.c文件编译为.o

ar rcs libmycalc.a *.o # 把所有.o打包成静态库libmycalc.a

nm libmycalc.a # 查看静态库的内容
# 在编成静态库后去掉src目录依然运行
```

库的使用如下:

```Shell
gcc main.c -I ./include -L ./lib -l mycalc -o app
```

其中大写的-L是指定静态库的目录,而小写-l是指定静态库名称。

下面是总结

- `-L`参数是指定库的路径

- `-l`参数是库名字(去掉了lib和.a)

#### 动态库的制作和使用

- 命名规则
	- libxxx.so

- 制作动态库步骤

	- 1. 将源文件生成.o `gcc *.o -c -fpic` 
	- 2. 打包所有.o `gcc -shared -o libxxx.so *.o`

- 库的使用
	- 1. 引入头文件a.h
	- 2. 动态库libxxx.so
	- 3. 编译时链接该动态库

```Shell
# -I后面是指明头文件, -L指明动态库路径 -l是指明动态库名(动态库名去掉了前缀lib和后缀.so)
gcc main.c -I ./ -L ./ -l xxx -o app
```

来看一个具体的例子:

示例代码都放在 https://github.com/helintongh/KnowledgeTree/tree/master/linux_basic/src/calculator

总共有三个代码sum.h sum.c main.c,源代码如下:

```C
// sum.h
#ifndef _SUM_H
#define _SUM_H

int sum(int a, int b);

#endif

// sum.c
#include "sum.h"

int sum(int a, int b)
{
    return a+b;
}

// main.c
#include "sum.h"
#include <stdio.h>

int main()
{
    int k = sum(1, 3);
    printf("%d", k);
    return 0;
}
```

依次执行如下命令:

```Shell
gcc *.c -c -fpic				
gcc -shared -o libsum.so *.o  # 生成了libsum.so动态库
rm sum.c 		# 删除sum.c才能看到效果,so能够隐藏源代码,main.c中的sum函数实现放入到了libsum.so中
gcc main.c -I ./ -L ./ -l sum -o app # 编译main.c为app
export LD_LIBRARY_PATH=./libsum.so:$LD_LIBRARY_PATH # 把libsum.so临时加入到动态链接库中,否则编译完成后app无法运行
./app          # 输出为4可以看到运行良好
```

- 动态库的使用补充: 加载动态库

下面有几条指令可以查看动态链接库

```Shell
ldd app # 查看app链接的库
env # 查看全部环境变量
echo $
```

这里要明白elf格式的可执行程序,是由`ld-linux.so` 动态加载器来完成的。

所以需要让动态加载器能找到你生成的so,那么编译完成后的app才能运行。

总共有三种办法:

1. 使用环境变量
	- 临时设置:
		在Shell终端设置
```Shell
export LD_LIBRARY_PATH=动态库路径:$LD_LIBRARY_PATH
```
	其中动态库路径最好的环境变量。

2. 永久设置	

	- 用户级别修改
		修改./bashrc 配置后重启

	- 系统级别修改
		修改`/etc/profile`

在/etc/profile最后加上
```Shell
export LD_LIBRARY_PATH=动态库路径:$LD_LIBRARY_PATH
```

3. 使用dlopen和dlclose以及dlsym函数,通过函数导入so的实现。