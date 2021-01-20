# Linux开发核心知识

知识是学不完的,但是Linux编程的核心知识体系是可以建立起来的,为何不建一个自己的Cache来强化自己的知识图谱呢?

# 快速索引

通过Cache来缩小知识范围，但是即使是被压缩过的知识依旧很多，我们怎么能够在脑海中快速检索它们呢？结合查找算法，Hash无疑是最快的，但又有多少人能够给一个"key"立马对应上"value"呢？所以，最适合人类认知的方式是通过索引 + 树状结构，在整理这份笔记时，我划分了很多级索引用来将各部分知识点划分到相应的模块中，不仅检索速度上去了还可以对整个知识体系有宏观认识。


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
			* [dup](docs/dup.md)
	* [文件操作](docs/文件操作.md)
		* [chown](docs/chown.md)
		* [rename](docs/rename.md)
		* [stat](docs/stat.md)
		* [dirname](docs/basename.md)
		* [basename](docs/basename.md)
		* [目录操作](docs/目录操作.md)
		* [getcwd](docs/getcwd.md)
* [Linux Socket网络编程](docs/网络编程.md):

	|[套接字结构](docs/套接字结构.md)	|[套接字函数](docs/套接字函数.md)|
	| -------- | ----------|
	|[字节序转换](docs/字节序转换函数.md)|[字节序转换](docs/字节序转换函数.md)|
	|[主机](docs/主机.md)|[服务](docs/服务.md)|

	* [带外数据](docs/带外数据.md)
	  * [sockatmark](README.md)
	* [文章总结1]()
	* [文章总结2]()