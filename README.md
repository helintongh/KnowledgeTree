## Linux开发知识图谱

知识是学不完的,但是Linux编程的核心知识体系是可以建立起来的,为何不建一个自己的Cache来强化自己的知识图谱呢?

[Linux编程使用基础](linux_basic/README.md) 

四篇文章掌握Linux编程基础图谱: [Linux进程间通信](ipc_guide/ipcguide.md)


下面是我总结的api的路径:

* [Linux基础编程](README.md):
	* [1.文件IO](docs/文件IO.md)
		* [POSIX](README.md)
			* [open](docs/open.md)
			* [read](docs/read.md)
			* [write](docs/write.md)
			* [lseek](docs/lseek.md)
			* [fcntl](docs/fcntl.md)   可用来修改文件描述符为非阻塞
			* [dup](docs/dup.md)
	* [2.文件操作](docs/文件操作.md)
		* [chown](docs/chown.md)
		* [rename](docs/rename.md)
		* [stat](docs/stat.md)
		* [dirname](docs/basename.md)
		* [basename](docs/basename.md)
		* [目录操作](docs/目录操作.md)
		* [getcwd](docs/getcwd.md)

* [Linux并发编程之进程相关](README.md)

	* 1. [进程控制](docs/进程控制.md)

	| [fork](docs/fork.md) |  [vfork](docs/vfork.md) |
	| ------ | ------ |
	|  [exec~](docs/exec.md) | [system](docs/system.md) |   <kbd>fork进程执行命令</kbd>>
	| [wait](docs/wait.md) | [waitpid](docs/waitpid.md) |	<kbd>进程回收</kbd>>

	[进程知识补充-守护进程,进程组,会话等知识](docs/进程知识补充.md)

* [IPC进程间通信](docs/进程通信.md)
	* 1. 管道

	| [pipe](docs/pipe.md) |  [mkfifo](docs/mkfifo.md) | [mknod](docs/mknod.md) |
	| ------ | ------ | -------- |


	* 2. [信号处理](docs/信号处理.md)
		* [所有信号类型](docs/信号类型.md)
		* [psignal](docs/psignal.md)
		* [kill](docs/kill.md)
		* [raise](docs/raise.md)
		* [signal](docs/signal.md)
		* [sigaction](docs/sigaction.md)
		* [信号阻塞](docs/信号阻塞.md)
		* [sigsuspend](docs/sigsuspend.md)
		* [sigaltstack](docs/sigaltstack.md)
	* 3.1 [IPC对象](docs/IPC.md) <kbd>System V</kbd> 

	关键函数: 获取一个系统中唯一的key值 [ftok](docs/ftok.md)

	特殊： [共享/文件锁](file_lock.md)，以及[内存映射](内存映射.md)

	|[消息队列](docs/消息队列.md)|[信号量](docs/信号量.md)|[共享内存](docs/共享内存.md)|[内存映射](内存映射.md)|
	| ------------------- | ----------------- | ------------------- | ------------------- |
	|[msgget](docs/msgget.md)|[semget](docs/semget.md)|[shmget](docs/shmget.md)|[mmap](docs/mmap.md)|
	|[msgctl](docs/msgctl.md)|[semctl](docs/semctl.md)|[shmctl](docs/shmctl.md)|[munmap](docs/munmap.md)|
	|[msgsnd](docs/msgsnd-msgrcv.md)|[semop](docs/semop.md)|[shmat](docs/shmat-shmdt.md)|            |
	|[msgsnd](docs/msgsnd-msgrcv.md)|         |[shmdt](docs/shmat-shmdt.md)|        |

	* 3.2 IPC对象 <kbd>POSIX</kbd> POSIX的IPC对象多用在线程同步方面,将放在多线程中概述。
* [Linux多线程编程](docs/线程.md)

|线程基础函数|线程互斥和同步|
|---|----|
|[pthread_create](docs/pthread_create.md)|[互斥锁](docs/互斥锁.md)|
|[pthread_exit](docs/pthread_exit.md)|[读写锁](docs/读写锁.md)|
|[pthread_join](docs/pthead_join.md)|[条件变量](docs/条件变量.md)|
|[pthread_detach](docs/pthead_detach.md)|[线程信号量](docs/semaphore.md)|
|[pthread_attr_init](docs/pthead_attr_init.md)|| 
|[pthread_cancle](docs/pthead_cancle.md)||



* [Linux Socket网络编程](docs/网络编程.md):

	|[套接字结构](docs/套接字结构.md)	|[套接字函数](docs/套接字函数.md)|
	| -------- | ----------|
	|[字节序转换](docs/字节序转换函数.md)|[字节序转换](docs/字节序转换函数.md)|
	|[主机](docs/主机.md)|[服务](docs/服务.md)|

	* [带外数据](docs/带外数据.md)
	  * [sockatmark](README.md)

	  