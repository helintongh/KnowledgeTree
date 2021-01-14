# Linux开发核心知识

最清晰的层级结构去总结那些对C++后台开发最为核心的内容。

我现在越发觉得多不一定好，看再多东西没有理解透彻都是白搭，把最常用的每天过一遍才是最有效的。开发中我们经常用缓存来提高吞吐率，学习知识何不也给自己加个Cache呢？

# 快速索引

上面我们提到了Cache来缩小知识范围，但是即使是被压缩过的知识依旧很多，我们怎么能够在脑海中快速检索它们呢？结合查找算法，Hash无疑是最快的，但又有多少人能够给一个"key"立马对应上"value"呢？所以，最适合人类认知的方式是通过索引 + 树状结构，在整理这份笔记时，我划分了很多级索引用来将各部分知识点划分到相应的模块中，检索任意一个知识点最多5级深度，不仅检索速度上去了还可以对整个知识体系有宏观认识。


下面是我总结的api的路径:

* [Linux使用基础](README.md):
	* [Linux目录结构](linux_basic/01Linux目录结构.md)
	* [Linux文件](linux_basic/02Linux文件.md)
	* [Linux用户权限管理](linux_basic/03Linux用户权限管理.md)
	* [Linux文件查找和检索](linux_basic/04Linux文件查找和检索.md)
	* [Linux压缩包管理](linux_basic/05Linux压缩包管理.md)
	* [Linux动态库静态库怎么生成的](06Linux源代码变成库.md)

* [Linux基础编程](README.md):
	* [文件IO](docs/文件IO.md)
		* [POSIX](README.md)
			* [open](docs/open.md)
			* [read](docs/read.md)
			* [write](docs/write.md)
			* [lseek](docs/lseek.md)
			* [fcntl](docs/fcntl.md)   可用来修改文件描述符为非阻塞
	* [文件操作](docs/文件操作.md)
		* [chown](docs/chown.md)
		* [rename](docs/rename.md)
		* [stat](docs/stat.md)
		* [dirname](docs/basename.md)
		* [basename](docs/basename.md)
		* [目录操作](docs/目录操作.md)
		* [getcwd](docs/getcwd.md)
* [Linux Socket网络编程](docs/网络编程.md):

	| -------- | ----------|
	|[套接字结构](docs/套接字结构.md)|[套接字函数](docs/套接字函数.md)|
	|[字节序转换](docs/字节序转换函数.md)|[字节序转换](docs/字节序转换函数.md)|
	|[主机](docs/主机.md)|[字节序转换]|[服务](docs/服务.md)|

	* [带外数据](docs/带外数据.md)
	  * [sockatmark](README.md)
	* [文章总结1]()
	* [文章总结2]()