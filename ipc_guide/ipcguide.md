### 一文了解Linux中进程间通信编程


### 1.文章简介

一个系统往往运行着很多个进程,如果进程都是孤立的,那到简单了。但多个进程往往是需要协助完成某些任务的。怎么让不同进程间建立起沟通的桥梁呢?Linux 通过System V实现的IPC进程间机制来达到这一目的。本文就是介绍System V的所有进程间通信的方法。这不是讲函数原型的!!

### 2.初探fork

fork是Linux中创建新进程的函数。

#### 2.1fork是危险而又迷人的峡谷

`fork()` 可以让工程师获得创造新生命的权力。权力有时被认为是通往毁灭的车票。因此，当你在你的系统上摆弄`fork()`的时候，你应该小心。`fork()`就是一把锋利的双刃剑,刀刃向敌人可所向披靡,而刀刃亦可轻易割伤自己。

`fork()`是Unix启动新进程的方式。基本上，它的工作原理是这样的:父进程(已经存在的那个)`fork()`一个子进程(新的那个)。子进程获得父进程数据的副本。瞧!您有两个过程，但只有一个函数就完成了!

，在`fork()`进程时，必须处理各种各样的陷阱，当你没有处理然后你把该子进程注册到进程列表中时，你的操作系统管理进程们将会生气，他们必须在机器上按下重置按钮。


首先,得了解一些Unix下进程的行为。当一个进程死亡时，有时它并没有真正完全消失。有的进程死后就不再运行，但有的死后的进程其实是需要父进程来处理它释放它的pcb和资源,此时这类子进程被叫做僵尸进程。因此当父进程 `fork()` 函数指向子进程后，它必须使用 `wait()` (或 `waitpid()` )函数让子进程退出。正是这种等待的行为让这个进程的所有残余(子进程)消失了这个进程才能叫正常结束。才不会有僵尸进程。

上面的规则有一种情况例外:父进程可以忽略SIGCHLD信号(在一些较老的系统上是SIGCLD)，这样它就不必执行 `wait()` 。在支持该操作的系统下可以这么做:

注: `wait()`可以指代为进程回收。

```c
main()
{
    signal(SIGCHLD, SIG_IGN);  /* 没有必要必须使用wait()! */
    .
    .
    fork();fork();fork();  /* fork就完事了 */
}
```

现在，当一个子进程死亡并且没有执行进程回收函数 `wait()` 时(子进程是僵尸进程)，它通常会在ps列表中显示为**defunct**。它将一直保持这种状态，直到父进程`wait()`调用它，或者像下面提到的那样处理它。

另一个规则是:当父进程在等待子进程之前死亡(假设它没有忽略SIGCHLD)，子进程会被重新分配给init进程(PID 1)。如果子进程仍然活得很好并且在控制之下，这不是一个问题。然而，如果子进程还活着，就有会有麻烦了。原父进程不能再`wait()`了，因为它已经死了,此时子进程变为孤儿进程,孤儿进程是交由init进程管理的。那么init是如何`wait()`处理僵尸进程的呢?

在某些系统上，init会周期性地销毁它拥有的所有已死进程。在另外一些系统上，它直接拒绝成为任何失效进程的父进程，而是立即销毁它们。如果你来设计一个init进程，那么可以很容易地编写一个循环，用init拥有的已死进程列表然后不断的`wait()`已死进程直到已死进程列表为空。

所以使用fork得注意两点:

1. 确保父进程忽略SIGHCLD.
2. 为它 `fork()` 的所有子进程执行`wait()` 。

当然不必总是这样做(比如您正在启动一个守护进程或其他什么)，但总之如果要使用`fork()`，那么在编写代码时一定要小心。

总结:除非父进程忽略了SIGCHLD，否则子进程有可能将在父进程`wait()`之前失效。而且，如果父进程没有等待(再次假设父进程没有忽略SIGCHLD)就夭折了，则其子进程(活着的或已死的)将成为init进程的子进程，而init进程会处理他们但是会有很大的开销。

#### 2.2fork使用案例分析

```c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid; // fork成功,保存子进程的id
    int rv;

    switch(pid = fork()) {
    case -1:
        perror("fork");  /* fork失败了 */
        exit(1);         /* 父进程直接退出 */

    case 0: // 返回0则说明是子进程
        printf(" CHILD: This is the child process!\n");
        printf(" CHILD: My PID is %d\n", getpid()); // 获得当前进程pid
        printf(" CHILD: My parent's PID is %d\n", getppid()); // 获取父进程pid
        printf(" CHILD: Enter my exit status (make it small): ");
        scanf(" %d", &rv); // rv作为子进程被wait()回收时的返回值
        printf(" CHILD: I'm outta here!\n");
        exit(rv);

    default: // 非0则为父进程
        printf("PARENT: This is the parent process!\n");
        printf("PARENT: My PID is %d\n", getpid());
        printf("PARENT: My child's PID is %d\n", pid);
        printf("PARENT: I'm now waiting for my child to exit()...\n");
        wait(&rv); // 等待子进程回收
        printf("PARENT: My child's exit status is: %d\n", WEXITSTATUS(rv));
        printf("PARENT: I'm outta here!\n");
    }

    return 0;
}
```

pid_t是进程类型。在Unix下，这是一个short的(Linux下是int)。因此调用fork()并将返回值保存在pid变量中。fork()很简单，因为它只能返回三个东西:

- **0** - 如果它返回0，那么当前是子进程。可以通过调用`getppid()`来获取父进程的PID。当然，可以通过调用`getpid()`来获得自己的PID

- **-1** - 如果返回-1，则表示出了问题，并且没有创建子进程。可使用`perror()`查看发生了什么。可能已经进程表已满无法创建新进程

- **其他** - fork()返回的任何其他值都意味着当前pid是父进程，而返回的值是子类的PID。这是获得子进程PID的唯一方法，因为没有`getcpid()`调用(显然是由于父进程和子进程之间的一对多关系)。

当子进程最终调用exit()时，传递的返回值将在其父进程`wait()` 时到达。正如从 `wait()` 调用中所看到的，当我们打印返回值时，会出现一些奇怪的情况。这个 `WEXITSTATUS()` 是什么东西呢?这是一个宏，它从`wait()` 返回的值中提取子进程的实际返回值。 具体参照[wait](../docs/wait.md)

这里还有一个疑惑:“wait()如何知道要等待哪个进程?”既然父母可以有多个孩子，那么`wait()`真正等待的是哪个呢?”答案很简单，它会等待最先退出的那一个。如果要等待特定的子进程，可以使用子进程的PID作为参数调用`waitpid()`来指定要等待哪个子进程。

从上面的例子中需要注意的另一件有趣的事情是，父类和子类都使用rv变量。这是否意味着它是在进程之间共享的?不!如果是的话，就不需要了解这些IPC的东西了。每个进程都有自己的所有变量副本。

**进程间的内存不能共享!**

关于上述程序的最后一点注意事项:我使用了switch语句来处理fork()，这并不是很典型。通常你会在那里看到一个if语句;有时它像这样简洁:

```c
if (!fork()) 
{
    printf("I'm the child!\n");
    exit(0);
} 
else 
{
    printf("I'm the parent!\n");
    wait(NULL);
}
```

如果你不关心子对象的返回值是什么，如何使用wait()呢? 你只需要用NULL作为参数调用它。

#### 2.3fork总结

fork能力强大能够极大的利用cpu(尤其是多核)。但于此同时要注意回收它的资源。

下面有几个问题：

子进程创建成功之后,代码执行位置? 父进程到了哪里,子进程就从那里开始。
父子谁先执行? 编程届都是不孝子(笑),C++中子能继承父的方法,父无法调用子。 而父子进程,父子都抢CPU(白富美)谁抢到谁执行。
如何区分父子进程? 通过返回值
父子的数据永远一样吗? fork完成之后一样。读时共享,写时复制。(有血缘关系)

进程回收相关:
1. 孤儿进程
    * 父创建子进程,父进程死亡,子进程还活着。
    * 孤儿进程会被init进程领养,init进程变为其父进程
    * 为了释放子进程占用的资源。为什么init必须成为其父进程呢? 进程结束之后,能够释放用户区空间。释放不了pcb,必须用父进程释放。

2. 僵尸进程
    * 子进程死亡,父进程存活。父进程不去释放子进程的pcb,子进程变为了僵尸进程。
    * 是一个已经死亡的进程
    * 解决方案,直接杀死父进程即可。

### 3.信号

有一个有用的方法可以让一个进程操作另一个进程:信号。

一个进程可以“触发(raise)”一个信号，并将其传递给另一个进程。然后目标进程的信号处理程序(只是一个函数)被调用，目标进程就开始可进行一系列操作。

实际上，在信号传递消息中，能安全地做的事是很少的。所以信号机制得慎用,但它也是一个非常有价值的服务。

例如，一个进程可能想要终止另一个进程，这可以通过向该进程发送信号SIGSTOP来实现。也可让进程必须接收信号SIGCONT才能继续运行。

这样就有两个问题:当它接收到一个特定的信号时，进程是如何知道这个信号是什么?Linux预定义了很多信号，进程有一个默认的信号处理程序来处理它。

举个例子:比如有一个信号是SIGINT。当某个进程在你的终端运行时你给其发送^c信号(键盘按键是ctrl+c)。进程的默认信号处理程序收到SIGINT信号后会导致进程退出。那么很明显了，可以重写针对对SIGINT信号的处理来做任何你想做的事情(或者什么都不做!)。比如你可以让你的进程收到该信号后`printf()`一些语句。

可以让你的进程以你想要的任何方式响应任何信号。除了一些特殊的信号,**SIGKILL**信号为例，9号信号。比如当你要杀死进程时会键入`kill -9 process_number` 去杀死一个运行的进程,其实这条指令是在发送SIGKILL信号给进程。没有任何进程可以从“kill -9”中逃脱。SIGKILL是一种不能添加自己的信号处理程序的信号。前面提到的SIGSTOP也属于这一类。某种程度上保证了进程不会变为癌细胞,始终有杀死它的办法。

有两个信号是不保留的:SIGUSR1和SIGUSER2。你可以自由地使用它们来做任何你想做的事，也可以选择任何方式来处理它们。

#### 3.1来感受捕捉信号的乐趣吧

Linux“kill”命令是向进程发送信号的一种方式。有一个`kill()`的系统调用也做同样的事情。它的参数是信号number(如signal.h中定义的)和进程ID。此外，还有一个函数`raise()`，可用于在同一个进程中引发信号。

如何捕捉特定信号比如SIGTERM?需要调用`sigaction()`并告诉它你想要捕获哪个信号以及你想要调用哪个函数来处理它的所有细节。

原型如下:

```c
int sigaction(int sig, const struct sigaction *act,
              struct sigaction *oact);
```

第一个参数sig是要捕捉哪个信号。这应该是是来自signal.h中定义的符号名

act是一个指向struct sigaction的指针，它有一堆字段，你可以填进去控制信号处理程序的行为。(指向包含在结构体中的信号处理函数本身的指针。)

oact可以为空，但如果不是，它将返回原来的信号处理程序信息。如果希望在以后恢复以前的信号处理程序，这是非常有用的。

同时还需要关注将关注结构体sigaction中的这三个字段:

|Signal | 描述 |
| ---- | --- |
|sa_handler|信号处理函数(或忽略信号的SIG_IGN)|
|sa_mask|在处理这个信号时要阻塞的一组信号|
|sa_flags|标记来修改处理程序的行为，或设为0|

更具体可以参照 [sigaction](../docs/sigaction.md) 文档。

那么sa_mask字段作用是什么呢?在处理信号时，可能希望阻止传递其他信号，可以通过将它们添加到sa_mask来实现这一点。sa_mask是一个“集合”，这意味着可以执行普通的集合操作来操作它们:sigemptyset()、sigfillset()、sigaddset()、sigdelset()和sigismember()。

这几个函数更详细的说明在 [信号阻塞](../docs/信号阻塞.md)

这里有一个处理SIGINT的，它可以通过点击^C来传递

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

void sigint_handler(int sig)
{
    write(0, "Ahhh! SIGINT!\n", 14);
}

int main(void)
{
    void sigint_handler(int sig); /* 处理函数实例->这一句其实可省略 */
    char s[200];
    struct sigaction sa;

    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0; // or SA_RESTART
    sigemptyset(&sa.sa_mask); // 初始化sa信号处理函数执行期间需要被屏蔽的信号,把被屏蔽信号置为空(没有信号被屏蔽)

    if (sigaction(SIGINT, &sa, NULL) == -1) { // 捕捉^c即SIGINT信号,执行sa绑定的函数
        perror("sigaction");
        exit(1);
    }

    printf("Enter a string:\n");

    if (fgets(s, sizeof s, stdin) == NULL)
        perror("fgets");
    else 
        printf("You entered: %s\n", s);

    return 0;
}
```

这个程序有两个函数:main()和sigint_handler()，前者设置信号处理程序(使用sigaction()调用)，后者本身是信号处理程序。

如果正在输入字符串，此时按下^C键，对gets()的调用将失败，并将全局变量errno设置为EINTR。此外，sigint_handler()被调用并执行它的例程，因此实际上可以看到:

```
Enter a string:
helintong^CAhhh! SIGINT!
fgets: Interrupted system call
```

然后程序退出了。

有几件事要做。首先，你将注意到调用了信号处理程序，因为它打印了"Ahhh! SIGINT!"但随后fgets()返回一个错误，即EINTR，或“中Interrupted system call”。看，一些系统调用可能会被信号中断，当这种情况发生时，它们会返回一个错误。你可能会看到这样的代码:

```c
restart:
    if (some_system_call() == -1) 
    {
        if (errno == EINTR) goto restart;
        perror("some_system_call");
        exit(1);
    }
```

与使用goto的效果相同，还可以设置sa_flags来包含SA_RESTART。例如，如果我们将SIGINT处理程序代码改为如下所示:

```c
Enter a string:
Hello^CAhhh! SIGINT!
Er, hello!^CAhhh! SIGINT!
This time fer sure!
You entered: This time fer sure!
```
有些系统调用是可中断的，而它们也有可能可重新启动。

#### 3.2信号处理可能存在的坑

在信号处理程序中调用函数时必须小心。这些函数必须是“异步安全的”，这样就可以调用它们而不出现未定义的行为(隐式行为)。

可能会感到好奇，为什么上面的信号处理程序调用write()而不是printf()来输出消息。答案是，POSIX说write()是异步安全的(因此从处理程序内部调用是安全的)，而printf()不是。

库函数和系统调用是异步安全的，才可以从你的信号处理程序调用:
以下都是安全的
```c
_Exit(), _exit(), abort(), accept(), access(), aio_error(), aio_return(), aio_suspend(), alarm(), bind(), cfgetispeed(), cfgetospeed(), cfsetispeed(), cfsetospeed(), chdir(), chmod(), chown(), clock_gettime(), close(), connect(), creat(), dup(), dup2(), execle(), execve(), fchmod(), fchown(), fcntl(), fdatasync(), fork(), fpathconf(), fstat(), fsync(), ftruncate(), getegid(), geteuid(), getgid(), getgroups(), getpeername(), getpgrp(), getpid(), getppid(), getsockname(), getsockopt(), getuid(), kill(), link(), listen(), lseek(), lstat(), mkdir(), mkfifo(), open(), pathconf(), pause(), pipe(), poll(), posix_trace_event(), pselect(), raise(), read(), readlink(), recv(), recvfrom(), recvmsg(), rename(), rmdir(), select(), sem_post(), send(), sendmsg(), sendto(), setgid(), setpgid(), setsid(), setsockopt(), setuid(), shutdown(), sigaction(), sigaddset(), sigdelset(), sigemptyset(), sigfillset(), sigismember(), sleep(), signal(), sigpause(), sigpending(), sigprocmask(), sigqueue(), sigset(), sigsuspend(), sockatmark(), socket(), socketpair(), stat(), symlink(), sysconf(), tcdrain(), tcflow(), tcflush(), tcgetattr(), tcgetpgrp(), tcsendbreak(), tcsetattr(), tcsetpgrp(), time(), timer_getoverrun(), timer_gettime(), timer_settime(), times(), umask(), uname(), unlink(), utime(), wait(), waitpid(), and write().
```
还有两点注意:

1. 可以在信号处理程序中调用自己的函数(只要它们不调用任何非异步安全函数)。

2. 不能安全地修改任何共享(例如全局)数据，但有一个值得注意的例外:声明为static类型和类型为`volatile sig_atomic_t`的变量。

下面是一个通过设置全局标志来处理SIGUSR1的示例，然后在主循环中检查该标志，以查看是否调用了处理程序。

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

volatile sig_atomic_t got_usr1; // 类型为volatile sig_atomic_t的变量

void sigusr1_handler(int sig)
{
    got_usr1 = 1; // 把该变量设置为1
}

int main(void)
{
    struct sigaction sa;

    got_usr1 = 0;

    sa.sa_handler = sigusr1_handler; // 绑定sigusr1_handler函数
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) { 
        perror("sigaction");
        exit(1);
    }

    while (!got_usr1) {
        printf("PID %d: working hard...\n", getpid()); // 打印进程id
        sleep(1);
    }

    printf("Done in by SIGUSR1!\n");

    return 0;
}
```

在一个窗口中启动它，然后在另一个窗口中使用`kill -USR1 process_id`杀死它。该程序会在终端打印出它的进程ID，这样你就可以把用kill传递信号给它了:

然后在另一个窗口中，发送信号SIGUSR1:

`kill -USR1 40893`

执行结果如下:

```
PID 40893: working hard...
PID 40893: working hard...
Done in by SIGUSR1!
```

注:

1. 即使刚刚调用了`sleep()`，响应也应该是立即的——`sleep()`会被信号中断。

2. ANSI-C定义了一个名为signal()的函数，可以用来捕获信号。它不像sigaction()那样可靠，功能也不全，所以通常不鼓励使用signal()

所有信号放在了这份文档里: [所有信号类型](信号类型.md)

### 4.pipe

pipe是非常简单的进程间通信方式。举个例子pipe()和fork()组成了"ls | more"中的"|"背后的功能，它们在各种Unix上都实现了。因为它太简单了,所以不会花太多的时间去概述。

#### 4.1一个简单的pipe用例

要明白管道,首先要了解文件描述符。你知道stdio.h里的`FILE *`吧。还有与之配套的函数`fopen(),fclose(),fwrite()` 等等函数。这些实际上是使用文件描述符实现的高级函数，文件描述符使用系统调用，如open()、creat()、close()和write()。文件描述符是简单的整数，类似于stdio.h中的`FILE *` 。

例如，stdin是文件描述符"0"，stdout是"1"，stderr是"2"。同样地，使用fopen()打开的任何文件都会得到它们自己的文件描述符，尽管这个细节对你是隐藏的。

pipe示意图如下:

![pipe图](resource/pipe.png)

基本上，调用pipe()函数会返回一对文件描述符。其中一个描述符连接到管道的写端，另一个连接到读端。任何内容都可以写入管道，并从管道的另一端按照输入的顺序读取。在许多系统中，当向管道写入约10K而没有读取任何内容时，管道就会被填满。

如图可知,fd[1]为写端文件描述符,fd[0]为读端文件描述符,下面看代码。

```c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

int main(void)
{
    int pfds[2];
    char buf[30];

    if (pipe(pfds) == -1) {
        perror("pipe");
        exit(1);
    }

    printf("writing to file descriptor #%d\n", pfds[1]);
    write(pfds[1], "test", 5);
    printf("reading from file descriptor #%d\n", pfds[0]);
    read(pfds[0], buf, 5);
    printf("read \"%s\"\n", buf);

    return 0;
}
```

pipe()接受一个包含两个整数的数组作为参数。假设没有错误，它将连接两个文件描述符并将它们返回到数组中。数组的第一个元素是管道的读取端，第二个元素是写入端。

上述代码的输出为
```
writing to file descriptor #4
reading from file descriptor #3
read "test"
```
从文件描述符为4和3,可知0，1，2是系统的stdin,stdout,stderr。

#### 4.2fork和pipe的结合

放入一个fork()，看看会发生什么。假设你是一名传递秘密的特工，你需要负责让子进程向父进程发送单词“test”。

1. 让父进程创建一个管道。

2. 然后fork()子进程。

3. fork()函数表明，子进程将收到父进程所有文件描述符的副本，其中包括管道的文件描述符副本。因此，子进程将能够将数据发送到管道的写端，而父进程将从读端获取数据。如下:

```c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

int main(void)
{
    int pfds[2];
    char buf[30];

    pipe(pfds);

    if (!fork()) 
    {
        printf(" CHILD: writing to the pipe\n");
        write(pfds[1], "test", 5); // pipe的fd[1]是写端
        printf(" CHILD: exiting\n");
        exit(0);
    } 
    else 
    {
        printf("PARENT: reading from pipe\n");
        read(pfds[0], buf, 5); // pipe的fd[0]是读端
        printf("PARENT: read \"%s\"\n", buf);
        wait(NULL);
    }

    return 0;
}
```
fork()一个新进程，并让它写入管道，而父进程从它读取。结果输出如下:

```
PARENT: reading from pipe
 CHILD: writing to the pipe
 CHILD: exiting
PARENT: read "test"
```

在本例中，父进程试图在子进程写入管道之前从管道中读取数据。当这种情况发生时，父进程将被阻塞或休眠，直到数据到达需要读取的位置。可以理解为父进程试图读取数据，然后进入睡眠状态，孩子写入并退出，而父醒来读取数据。

pipe的用途不是太多，因为你看到它有不少限制。

#### 4.3pipe实战

挑战:在C语言中实现`ls | wc -l`

实现这个命令还需要两个函数`exec()`和`dup()`。`exec()`函数族用传递给`exec()`的进程替换当前正在运行的进程。这是将用来运行ls和wc -l的函数。`dup()`获取一个打开的文件描述符，并对其进行克隆(复制)。这就是我们将ls的标准输出连接到wc的标准输入的方式。看，ls的stdout流进管道，wc的stdin从管道流进。管道正好在中间!

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    int pfds[2];

    pipe(pfds);

    if (!fork()) 
    {
        close(1);       /* 关闭stdout文件描述符 */
        dup(pfds[1]);   /* 使stdout与pfds[1]文件描述符相同 */
        close(pfds[0]); /* 子进程不需要pipe读端 */
        execlp("ls", "ls", NULL);
    } 
    else 
    {
        close(0);       /* 关闭stdin文件描述符 */
        dup(pfds[0]);   /* 使stdin与pfds[0]文件描述符相同 */
        close(pfds[1]); /* 父进程不需要pipe写端 */
        execlp("wc", "wc", "-l", NULL);
    }

    return 0;
}
```

我将对`close()`/`dup()`组合做另一个注释，因为它很奇怪。

- `close(1)`：释放文件描述符1(标准输出)。

- `dup(pfds[1])`在第一个可用的文件描述符“1”中复制管道的写端，因为我们刚刚关闭了它。这样，ls写到标准输出stdin(文件描述符1)的任何内容都将转到`pfds[1]` (管道的写端)。代码的wc部分以相同的方式工作，只是相反。

#### 4.4pipe总结

管道的用法是有局限性的:

1. 它只适用于有血缘关系的进程

2. 数据只能读一次,不能重复读取

3. 半双工方式,同一时刻只能一方数据传输是单向的


### 5.FIFO

FIFO是有名管道。也就是说，它就像一根pipe，只不过它有名字!在本例中，管道名是一个文件的名称，多个进程可以`open()`并对其进行读写操作。

#### 5.1创造一个新的FIFO

由于FIFO实际上是磁盘上的一个文件，必须做一些花哨的事情来创建它。这并不难。只需使用适当的参数调用mknod()。下面是一个创建FIFO的mknod()调用:

`mknod("myfifo", S_IFIFO | 0644 , 0);`

在上面的例子中，FIFO文件将被称为“myfifo”。

第二个参数是创建模式，它用于告诉mknod()创建一个FIFO(s_iffifo)并设置该文件的访问权限(octal 644，或rw-r——r——)，也可以通过将sys/stat.h中的宏集合在一起来设置。这个权限与使用chmod命令设置的权限类似。

最后，传递一个设备号。在创建FIFO时，这将被忽略，因此可以在其中放入任何想要的内容。

(顺便说一句:也可以使用Linux mknod命令从命令行创建FIFO。)

#### 5.2生产者和消费者

一旦创建了FIFO，进程就可以启动并使用标准的`open()`系统调用打开它进行读写。

在这里展示两个程序，它们将通过FIFO发送数据。一个是通过FIFO发送数据的speak.c，另一个被称为tick.c，因为它从FIFO中吸取数据。

speak.c
```c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_NAME "chinese_mainland"

int main(void)
{
    char s[300];
    int num, fd;

    mknod(FIFO_NAME, S_IFIFO | 0666, 0); // 创建FIFO

    printf("waiting for readers...\n");
    fd = open(FIFO_NAME, O_WRONLY); // 打开FIFO
    printf("got a reader--type some stuff\n");

    while (gets(s), !feof(stdin)) {
        if ((num = write(fd, s, strlen(s))) == -1) // 往FIFO里写入用户输入的字符串
            perror("write");
        else
            printf("speak: wrote %d bytes\n", num);
    }

    return 0;
}
```
speak所做的是创建FIFO，然后尝试打开它。现在，`open()`调用将阻塞，直到其他进程打开管道的另一端进行读取。(有一种方法可以解决这个问题——参见下面的`O_NDELAY`选项)

下面是tick.c:

```c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_NAME "chinese_mainland"

int main(void)
{
    char s[300];
    int num, fd;

    mknod(FIFO_NAME, S_IFIFO | 0666, 0);

    printf("waiting for writers...\n");
    fd = open(FIFO_NAME, O_RDONLY);
    printf("got a writer\n");

    do {
        if ((num = read(fd, s, 300)) == -1) // 读取FIFO中的数据
            perror("read");
        else {
            s[num] = '\0';
            printf("tick: read %d bytes: \"%s\"\n", num, s);
        }
    } while (num > 0);

    return 0;
}
```
如果没有人再speak端写入数据到FIFO, tick将在open()上阻塞。一旦有人打开FIFO进行书写，tric就会复活。

试一试!运行speak，它将阻塞，直到你开始在另一个窗口执行tick。(类似的,如果你开始tick，它将阻塞，直到你开始在另一个窗口执行speak.c)在“speak”程序窗口里不停地打字，tick就会把它全部吸走。

当speak仍在运行时跳出tick会发生什么呢?“破管（Broken Pipe）!”这是什么意思?发生的情况是，当FIFO的所有读取器关闭而写入器仍然打开时，写入器将在下一次尝试`write()`时接收信号**SIGPIPE**。此信号的默认信号处理程序打印“Broken Pipe”并退出。当然，可以通过signal()调用捕获SIGPIPE来更优雅地处理这个问题。

最后，如果有多个消费者会发生什么?有时候，一个消费者会得到一切。有时在消费者之间交替得到数据。

#### 5.3不可阻挡标识O_NDELAY

前面我提到过，生产者和消费者都只能阻塞地调用`open()`。那么非阻塞的调用`open()`的办法是在`open()`函数mode参数中设置O_NDELAY标志:

```c
fd = open(FIFO_NAME, O_RDONLY | O_NDELAY);
```

如果没有进程打开该文件进行读取，这将导致open()返回-1。

同样地，可以使用O_NDELAY标志打开消费者进程，但这有不同的效果:如果管道中没有数据，那么所有试图从管道中read()的操作都会返回0个读字节。(也就是说，read()将不再阻塞，直到管道中有一些数据。)注意，您不能再判断read()是否返回0，因为管道中没有数据，或者因为写入器已经退出。这是获得非阻塞的代价，但我的建议是尽可能坚持使用阻塞模式。

#### 5.4FIFO总结

把管道的名称写在磁盘上肯定会更容易共享数据，不是吗?不相关的进程可以通过管道通信!

尽管如此，管道的功能可能并不是应用程序所需要的。如果系统支持消息队列，则消息队列可能更适合会提供更快的通信速度。

### 6.文件锁

上面讲了不同进程间通信是通过操作同一块数据来实现的,那么就存在一个问题。大家都操作数据造成异步效果。所以需要锁机制。而这个锁是可以被不同进程锁使用的。

文件锁定为协调文件访问提供了一种非常简单但非常有用的机制。在开始阐述细节之前，让我先介绍一些文件锁定的秘密:

有两种类型的锁定机制:强制性文件锁(mandatory)和尝试性文件锁(advisory)

强锁实际上会阻止对文件的`read()`和`write()`操作。这里不推荐用该锁。

使用尝试性文件锁，进程在文件被锁定时仍然可以对其进行读写操作。那这不少没用吗?不完全如此，因为有一种方法可以让进程在进行读或写操作之前检查锁是否存在。这是一种合作锁定系统。对于几乎所有需要文件锁定的情况，这就足够了。

从现在起，本文中提到任意锁，都指的是尝试锁。

现在，让我进一步分析一下锁的概念。有两种类型的锁(尝试锁):读锁和写锁(也分别称为共享锁和排他锁)。读锁的工作方式是它们不会干扰其他读锁。例如，多个进程可以锁定一个文件，以便在同一时间读取。但是，当一个进程对一个文件有一个写锁时，其他进程都不能激活解锁操作(无论是读锁或写锁)，直到它被放弃。一种简单的方法是，可以同时有多个读取器，但一次只能有一个写入器。

我鼓励使用一种更高级的flock()风格的函数。

#### 6.1设置一个锁

`fcntl()`函数可以完成几乎所有的所有工作，但这里将只使用它来锁定文件。设置锁包括填充一个struct flock(在fcntl.h中声明)来描述需要的锁类型，以匹配的模式打开文件，并使用适当的参数调用fcntl():

```c
struct flock fl;
int fd;
    
fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK 这里是设置写锁  */
fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END 设置偏移量 */
fl.l_start  = 0;        /* Offset from l_whence         */
fl.l_len    = 0;        /* length, 0 = to EOF           */
fl.l_pid    = getpid(); /* our PID                      */

fd = open("filename", O_WRONLY);

fcntl(fd, F_SETLKW, &fl);  /* F_GETLK, F_SETLK, F_SETLKW */
```

下面是参数的解读:

|参数|描述|
| ----- | ----- |
|l_type|想要设置的锁的类型。如果你想设置读锁、写锁或清除锁，分别是F_RDLCK、F_WRLCK或F_UNLCK。|
|l_whence|这个字段决定了l_start字段从哪里开始(就像偏移量的偏移量)。它可以是SEEK_SET、SEEK_CUR或SEEK_END，表示文件的开头、当前文件位置或文件的结尾。|
|l_start|这是以字节为单位的锁的起始偏移量，相对于l_wherth。|
|l_len|这是以字节为单位的锁区域的长度(从l_start开始，l_start相对于l_where开始)。|
|l_pid|处理该锁的进程号。使用getpid()来获取它。|

我们的示例中，我们告诉它创建一个类型为F_WRLCK(写锁)的锁，相对于SEEK_SET(文件的开头)开始，偏移量为0，长度为0(0值表示“锁定到文件结尾”)，PID设置为getpid()。

下一步是`open()`文件，因为`flock()`需要被锁定的文件的文件描述符。请注意，当打开文件时，需要在锁中指定的相同模式打开它，如下表所示。如果对于给定的锁类型以错误的模式打开文件，`fcntl()`将返回-1,errno将被设置为EBADF。

|l_type|mode|
| ----- | ------ |
|F_RDLCK|O_RDONLY或O_RDWR|
|F_WRLCK|O_WRONLY或O_RDWR|

最后总结，其实对`fcntl()`的调用实际上是设置、清除或获取锁。`fcntl()`的第二个参数(cmd)告诉它如何处理`struct flock`中传递给它的数据。下面的列表总结了每个`fcntl()`的cmd的作用:

- F_SETLKW 这个参数告诉`fcntl()`尝试获取 `struct flock`结构体中请求的锁。如果无法获得锁(因为已经被其他人锁定了)，`fcntl()`将等待(阻塞)直到锁被清除，然后自己设置锁。这是一个非常有用的命令。

- F_SETLK 这个函数与F_SETLKW几乎相同。唯一的区别是，如果它不能获得锁，它将不会等待。它会立即返回-1。这个函数可以通过将`struct flock`中的`l_type`字段设置为F_UNLCK来清除锁。

- F_GETLK 如果你只想检查是否有一个锁，但不想设置一个，你可以使用这个命令。它将查找所有文件锁，直到找到一个与你在`struct flock`中指定的锁冲突的文件锁。然后，它将冲突锁的信息复制到结构中并返回。如果找不到冲突锁，`fcntl()`将返回传递给它的结构体，然后它将把`l_type`字段设置为F_UNLCK。

在上面的例子中，用F_SETLKW作为参数调用fcntl()，这样它就会阻塞，直到它可以设置锁，然后设置锁并继续。

#### 6.3解锁

上面的所有锁定内容之后，是时候做一些简单的事情了:解锁!事实上，相比之下，这简直是小菜一碟。我将重新使用第一个例子，并在最后添加代码来解锁它:

```c
struct flock fl;
int fd;

fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK  这里是设置写锁  */
fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END 设置位偏移量 */
fl.l_start  = 0;        /* Offset from l_whence         */
fl.l_len    = 0;        /* length, 0 = to EOF           */
fl.l_pid    = getpid(); /* our PID                      */

fd = open("filename", O_WRONLY);  /* 获取文件描述符 */
fcntl(fd, F_SETLKW, &fl);  /* 设置锁,并设置cmd为F_SETLKW */
.
.
.
fl.l_type   = F_UNLCK;  /* 标志为改为解锁标准 */
fcntl(fd, F_SETLK, &fl); /* 该文件描述符设置为未锁定状态 */
```

现在，我将旧的锁定代码保留在那里，以便进行比较，但是您可以看出，我只是将l_type字段更改为F_UNLCK(其他字段完全保持不变!)，并使用F_SETLK命令调用fcntl()。简单!

#### 6.3锁使用样例

lockdemo.c程序等待用户点击return，然后锁定自己的源，等待另一个return，然后解锁它。通过在两个(或多个)窗口中运行这个程序，可以看到程序在等待锁时是如何交互的。

基本上，用法是这样的:如果你在不带命令行参数的情况下运行lockdemo，它将尝试在其源文件(lockdemo.c)上获取一个写锁(F_WRLCK)。如果使用任何命令行参数启动它，它将尝试在它上获得一个读锁(F_RDLCK)。

```c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
                    /* l_type   l_whence  l_start  l_len  l_pid   */
    struct flock fl = {F_WRLCK, SEEK_SET,   0,      0,     0 };
    int fd;

    fl.l_pid = getpid(); // pid为当前进程pid

    if (argc > 1) 
        fl.l_type = F_RDLCK; // 参数多余一个为读锁,可被多个打开

    if ((fd = open("lockdemo.c", O_RDWR)) == -1) 
    { 
        perror("open");
        exit(1);
    }

    printf("Press <RETURN> to try to get lock: ");
    getchar();
    printf("Trying to get lock...");

    if (fcntl(fd, F_SETLKW, &fl) == -1) 
    {
        perror("fcntl");
        exit(1);
    }

    printf("got lock\n");
    printf("Press <RETURN> to release lock: ");
    getchar();

    fl.l_type = F_UNLCK;  /* 设置标志位为解锁 */

    if (fcntl(fd, F_SETLK, &fl) == -1) 
    {
        perror("fcntl");
        exit(1);
    }

    printf("Unlocked.\n");

    close(fd);

    return 0;
}
```

编译该程序然后多开几个窗口里运行一下。请注意，当一个lockdemo拥有一个读锁时，程序的其他实例可以毫无问题地获得它们自己的读锁。只有当获得了写锁时，其他进程才不能获得任何类型的锁。

另一件需要注意的事情是，如果在文件的同一区域上有任何读锁，则无法获得写锁。等待获得写锁的进程将一直等待，直到所有读锁都被清除。这样做的一个结果是，尽管可以不断地堆积读锁(因为读锁不会阻止其他进程获得读锁)，但这会造成任何等待写锁的进程都将坐在那里挨饿。如果有一个进程正在等待一个写锁，没有任何规则可以阻止用户添加更多的读锁。一定要小心这种情况。

#### 6.4总结

锁是有用的,但是有时候比如生产者和消费者场景的时候,文件锁有可能就不太够用了。此时需要信号量机制了。后面也会介绍到。

### 7.消息队列

消息队列的工作方式有点像FIFO，但支持一些额外的功能。通常情况下，消息是按照放入队列的顺序从队列中取出的。但是，有一些方法可以在某些消息到达队列前端之前将它们从队列中取出。这就像插队一样。

在使用方面，进程可以创建新的消息队列，也可以连接到现有的消息队列。在后一种方式中，两个进程可以通过相同的消息队列交换信息。

关于System V IPC还有一件事:当你创建一个消息队列时，它不会消失，直到你销毁它。所有曾经使用过它的进程都可以退出，但队列仍然存在。一个好的实践是使用`ipcs`命令来检查您未使用的消息队列是否只是漂浮在那里。可以使用`ipcrm`命令销毁它们，这比让系统管理员访问告诉您已经捕获了系统上所有可用的消息队列要好得多。


#### 7.1怎么找到消息队列呢?

首先，想要连接到一个队列，或者如果它不存在就创建它。实现此目的的调用是msgget()系统调用:

```c
int msgget(key_t key, int msgflg);
```
msgget()成功时返回消息队列ID，失败时返回-1(当然，它还设置errno)。

|参数|描述|
| ------- | ------- |
|key|是系统范围内的唯一标识符，描述你想要连接(或创建)的队列。其他所有想要连接到这个队列的进程都必须使用相同的key。|
|msgflg|msgflg告诉msgget()如何处理有问题的队列。要创建一个队列，这个字段必须设置为IPC_CREAT,它还指定队列权限,队列的权限是按位计算的|

#### 7.2如何得到一个key?

实际上`key_t`就是`long`类型,你可以用任何号码。但是，如果硬编码该数字，而其他一些不相关的程序硬编码相同的数字，但想要另一个队列呢?所以我们需要一个函数生成特定的唯一的一个数字,这个函数就是`ftok()`,该函数有两个参数返回一个key。

```c
key_t ftok(const char *path, int id);
```

path必须是这个进程可以读取的文件。另一个参数id通常被设置为任意的字符，比如'A'。ftok()函数使用关于命名文件的信息(如inode号等)和id为msgget()生成一个可能唯一的key。想要使用相同队列的程序必须使用相同的key，因此它们必须将相同的形参传递给ftok()。

```c
#include <sys/msg.h>

key = ftok("/home/he/somefile", 'b');
msqid = msgget(key, 0666 | IPC_CREAT);
```

在上面的示例中，我将队列的权限设置为666(或者rw-rw-rw-，如果这样对您更有意义的话)。现在我们有了msqid，它将用于发送和接收来自队列的消息。

#### 7.3把消息送入队列

使用msgget()连接到消息队列之后，就可以发送和接收消息了。

发送需要注意:
每条消息由两部分组成，这两部分定义在`struct msgbuf`中，如sys/msg.h中定义的:

```c
struct msgbuf {
    long mtype;
    char mtext[1];
};
```
队列中检索消息时将使用字段mtype，可以将其设置为任何正数。mtext是将被添加到队列中的数据。

只能添加一个字符到队列里?当然不是,你可以添加任何消息放到队列里去，只要第一个元素是long。

```c
struct pirate_msgbuf {
    long mtype;  /* 必须有long字段,最好放在首位 */
    struct pirate_info {
        char name[30];
        char ship_type;
        int notoriety;
        int cruelty;
        int booty_value;
    } info;
};
```

如何将这个信息传递给消息队列呢?答案很简单，我的朋友们:只要使用msgsnd():

```c
int msgsnd(int msqid, const void *msgp,
           size_t msgsz, int msgflg);
```

- msqid：消息队列标识符(由msgget生成)

- msgp：指向用户自定义的缓冲区（msgp），想放入消息队列的数据

- msgsz:接收信息的大小。范围在0～系统对消息队列的限制值

- msgflg:指定在达到系统为消息队列限定的界限时应采取的操作。

	* - IPC_NOWAIT 如果需要等待，则不发送消息并且调用进程立即返回，errno为EAGAIN
	* - 如果设置为0，则调用进程挂起执行，直到达到系统所规定的最大值为止，并发送消息

获取要发送的数据大小的最佳方法是一开始就正确设置它。正如所看到的，结构的第一个字段应该是一个`long`类型。为了安全和便于携带，应该只有一个额外的字段。如果你需要多个，可以像上面的pirate_msgbuf那样把它包装在一个结构体中。

获取要发送的数据的大小是多大呢?只需取第二个字段的大小:

```c
struct cheese_msgbuf {
    long mtype;
    char name[20];
};

/* 计算要发送的数据大小: */

struct cheese_msgbuf mbuf;
int size;

size = sizeof mbuf.name;

/* 获取cheese_msgbuf,name字段大小的另一种办法 */

size = sizeof ((struct cheese_msgbuf*)0)->name;
```

或者，如果你有很多不同的字段，把它们放到一个结构中，然后使用sizeof操作符。这样做会更加方便，因为子结构可以有一个名称来引用。下面的代码片段显示了一个女明星的信息被添加到消息队列:

```c
#include <sys/msg.h>
#include <stddef.h>

key_t key;
int msqid;
struct pirate_msgbuf pmb = {2, { "Utsunomiya Shion", 'female', 1, 3, 1994 } };

key = ftok("/home/he/somefile", 'b');
msqid = msgget(key, 0666 | IPC_CREAT);

/* 让她排队 */
/* pirate_info是子结构 */
msgsnd(msqid, &pmb, sizeof(struct pirate_info), 0);
```

除了记住对所有这些函数的返回值进行错误检查之外，这就是它的全部内容。注意，在这里任意地将mtype字段设置为2。这在下一节中很重要。

#### 7.4从队列里接收消息

现在Utsunomiya Shion困在我们的消息队列里了，我们怎么把她救出来?可以想象，与`msgsnd()`相对应的是:`msgrcv()`。就这么简单。

在msgrcv()调用中有一些新东西需要注意:

```c
int msgrcv(int msqid, void *msgp, size_t msgsz,
           long msgtyp, int msgflg);
```

在调用中指定的2是请求的msgtyp。回想一下，在本文的msgsnd()部分中将mtype任意设置为2,这就是从队列中检索到的那个。

实际上，`msgrcv()`的行为可以通过选择正的、负的或零的msgtyp来大幅修改:

- msgtyp=0：收到的第一条消息，任意类型。

- msgtyp>0：收到的第一条msgtyp类型的消息。

- msgtyp<0：收到的第一条最低类型（小于或等于msgtyp的绝对值）的消息。

通常的情况是，只是想要队列中的下一条消息，而不管它是什么mtype。因此，需要将msgtyp参数设置为0。

#### 7.5摧毁一个队列

有时你不得不销毁消息队列。就像我之前说的，它们会一直存在直到你明确地移除它们;这样做不会浪费系统资源是很重要的。你已经用这个消息队列一整天了，它已经老化了。你想要抹掉它。有两种方式:

1. 使用Linux命令ipcs获取已定义的消息队列列表，然后使用命令ipcrm删除该队列。

2. 写一个程序来帮你做。

通常，后一种选择是最合适的，因为可能希望程序在某个时候或其他时候清理队列。为此需要引入另一个函数:`msgctl()`。
```c
int msgctl(int msqid, int cmd,
           struct msqid_ds *buf);
```

msqid是从msgget()获得的队列标识符。重要的参数是cmd，它告诉msgctl()如何行为。它可以是各种各样的东西，但在此只讨论IPC_RMID，它用于删除消息队列。为了IPC_RMID的目的，可以将buf参数设置为NULL。

假设我们有上面创建的队列来容纳女明星。可以通过下面的调用来销毁这个队列:

```c
#include <sys/msg.h>
.
.
msgctl(msqid, IPC_RMID, NULL);
```
这样消息队列也不复存在了。

#### 7.6示例程序

为了完整起见，我将写一对使用消息队列进行通信的程序。首先，kirk.c将消息添加到消息队列中，然后spock.c检索它们。

kirk.c如下

```c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

struct my_msgbuf {
    long mtype;
    char mtext[200];
};

int main(void)
{
    struct my_msgbuf buf;
    int msqid;
    key_t key;

    if ((key = ftok("kirk.c", 'B')) == -1)  // 直接用kirk.c源文件通信,这不好这里只是示范用。最好自己随意选择一个文件
    {
        perror("ftok");
        exit(1);
    }

    if ((msqid = msgget(key, 0644 | IPC_CREAT)) == -1) 
    {
        perror("msgget");
        exit(1);
    }
    
    printf("Enter lines of text, ^D to quit:\n");

    buf.mtype = 1; /*在这种情况下我们并不关心是什么类型的消息,设置为0也可 */

    while(fgets(buf.mtext, sizeof buf.mtext, stdin) != NULL) 
    {
        int len = strlen(buf.mtext);

        /* 如果存在换行符，则在末尾删除换行符 */
        if (buf.mtext[len-1] == '\n') buf.mtext[len-1] = '\0';

        if (msgsnd(msqid, &buf, len+1, 0) == -1) /* +1 存放 '\0' */
            perror("msgsnd");
    }

    if (msgctl(msqid, IPC_RMID, NULL) == -1) 
    {
        perror("msgctl");
        exit(1);
    }

    return 0;
}
```
kirk的工作方式是允许你输入几行文本。每一行都被绑定到一条消息中，并添加到消息队列中。然后由spock读取消息队列。

spock.c
```c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

struct my_msgbuf {
    long mtype;
    char mtext[200];
};

int main(void)
{
    struct my_msgbuf buf;
    int msqid;
    key_t key;

    if ((key = ftok("kirk.c", 'B')) == -1) {  /* 和kirk.c的key保持一致 */
        perror("ftok");
        exit(1);
    }

    if ((msqid = msgget(key, 0644)) == -1) { /* 连接上队列 */
        perror("msgget");
        exit(1);
    }
    
    printf("spock: ready to receive messages, captain.\n");

    for(;;) { /* Spock一直等待并接收队列中的数据! */
        if (msgrcv(msqid, &buf, sizeof buf.mtext, 0, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }
        printf("spock: \"%s\"\n", buf.mtext);
    }

    return 0;
}
```

注意，在对msgget()的调用中，spock不包含IPC_CREAT选项。我们让kirk来创建消息队列，如果不这么做，spock就会返回一个错误。

请注意，当你在两个单独的窗口中运行时，你杀死了其中一个窗口。也可以试着运行两个kirk(此时一个spock)或两个spock(此时一个kirk)，以了解当你有两个读者或两个作者时会发生什么。另一个有趣的演示是运行kirk，输入一堆信息，然后运行spock，看它在一次行动中检索所有信息。摆弄一下这些玩具程序将帮助您了解到底发生了什么。

#### 7.7消息队列总结

消息队列的内容远远超出了本文的范围。一定要查看手册页，看看还可以做些什么，特别是在msgctl()方面。此外，还可以将更多选项传递给其他函数，分别控制msgsnd()和msgrcv()在队列满或为空时的处理方式。

#### 8.信号量

还记得文件锁定吗?信号量可以被认为是真正通用的锁机制。可以使用它们来控制对文件、共享内存以及任何您想要的内容的访问。信号量的基本功能是，你可以设置、检查信号量，或者等待信号量清除后再设置信号量(“test-n-set”)。不管后面的东西有多复杂，记住这三个操作。

本文档将概述信号量的功能，并以一个使用信号量来控制对文件访问的程序结束。(不可否认，这个任务可以很容易地用文件锁定来处理，但是它是一个很好的例子，因为它比共享内存更容易理解。)

#### 8.1获得一些信号量

在System V IPC中，不能抓取单个信号量;而应该抓取一组信号量。当然，可以获取一个只有一个信号量的信号量集，但关键是可以通过创建一个信号量集来获得大量的信号量。

如何创建信号量集?它是通过调用`semget()`来完成的，它返回信号量id(以下称为semid):

```c
#include <sys/sem.h>

int semget(key_t key, int nsems, int semflg);
```

key是什么?它是一个唯一的标识符，不同的进程使用它来标识这个信号量集(这个key将使用ftok()生成，在消息队列部分中有描述)。

下一个参数nsems是这个信号量集的个数。确切的数字取决于系统，但它可能在500到2000之间。如果你需要更多的信号量，只需获得另一个信号量集。

最后，还有semflg参数。这告诉semget()在新信号量集上应该有哪些权限，是在创建一个新信号量集还是只是想连接到一个现有信号量集，以及其他操作。为了创建一个新集合，可以使用IPC_CREAT标志。

下面是一个示例调用，它使用ftok()生成key，并创建一个10信号量集，具有666 (rw-rw-rw-)权限:

```c
#include <sys/ipc.h>
#include <sys/sem.h>

key_t key;
int semid;

key = ftok("/home/he/somefile", 'E');
semid = semget(key, 10, 0666 | IPC_CREAT);
```

你已经创建了一个新的信号量集!运行该程序后，你可以使用ipcs命令检查它。(当你用完它后需要使用ipcrm删除它，不要忘记删除它!)


当你第一次创建一些信号量时，它们都没有初始化;它需要另一个调用来将它们标记为空闲(即semop()或semctl()——请参阅下面几节。)这是什么意思?这意味着信号量的创建不是原子的(换句话说，它不是一个一步过程)。如果两个进程试图同时创建、初始化和使用信号量，可能会产生竞争条件。

解决这个问题的一种方法是，在主进程开始运行之前，使用一个init进程来创建和初始化信号量。主进程只是访问它，但不会创建或销毁它。

(W. Richard Stevens称)这个问题可称为信号量的“致命缺陷”。他通过创建带有IPC_EXCL标志的信号量集解决了这个问题。如果进程1首先创建它，进程2将在调用时(进程1还未初始化信号量集)返回一个错误(errno设置为EEXIST)。此时，进程2将不得不等待，直到进程1初始化信号量。

进程2是怎么知道信号量集已初始化呢?进程2可以使用IPC_STAT标志重复调用semctl()，并查看返回的`struct semid_ds`结构的`sem_otime`成员。如果它是非零，则意味着进程1已经使用`semop()`对信号量执行了操作，该操作是为了初始化信号量集。

我会写一个示例代码semdemo.c。

在写之前先来看一看`semctl()`函数吧。

#### 8.2使用semctl()控制你的信号量

创建了信号量集之后，必须将它们初始化为正值，以表明资源是可用的。函数semctl()允许对单个信号量或完整的信号量集进行原子值更改。

```c
int semctl(int semid, int semnum,
           int cmd, ... /*arg*/);
```

semid是从前面对semget()的调用中获得的信号量集id。semnum是操纵信号量的ID的值。cmd是对有问题的信号量做的操作。最后一个“参数”，“arg”，如果需要，需要是一个union semun，它将由你在你的代码中定义为以下之一:

```c
union semun {
    int val;               /* 仅用于SETVAL */
    struct semid_ds *buf;  /* 用于IPC_STAT和IPC_SET */
    ushort *array;         /* 用于GETALL和SETALL */
};
```
`union semun`中的各个字段的使用取决于`setctl()`的cmd参数的值:

共有的ipc的四个操作:


- IPC_RMID删除消息队列。从系统中删除给消息队列以及仍在该队列上的所有数据，这种删除立即生效。 仍在使用这一消息队列的其他进程在它们下一次试图对此队列进行操作时，将出错，并返回EIDRM。 此命令只能由如下两种进程执行： 其有效用户ID等于msg_perm.cuid或msg_perm.guid的进程。 另一种是具有超级用户特权的进程。

- IPC_SET设置消息队列的属性。按照buf指向的结构中的值，来设置此队列的msqid_id结构。 该命令的执行特权与上一个相同。

- IPC_STAT读取消息队列的属性。取得此队列的msqid_ds结构，并存放在buf中。

- IPC_INFO读取消息队列基本情况。

- GETVAL //返回的是semnum的值
- GETALL //设置semnum为0，那么获取信号集合的地址传递给第四个参数。返回值为0，-1
- GETNCNT //设置semnum为0，返回值为等待信号量值的递增进程数，否则返回-1
- GETZCNT //设置semnum为0，返回值是等待信号量值的递减进程数，否则返回-1
- SETVAL //将第四个参数指定的值设置给编号为semnum的信号量。返回值为0，1
- SETALL //设置semnum为0，将第四个参数传递个所有信号量。返回值0，1

面是union semun中使用的结构体semid_ds的内容:

```c
/*位于/usr/include/linux*/
struct semid_ds {
	struct ipc_perm	sem_perm;		/* 权限 .. see ipc.h */
	__kernel_time_t	sem_otime;		/* 最近semop时间 */
	__kernel_time_t	sem_ctime;		/*最近 修改时间*/
	struct sem	*sem_base;		/* 队列第一个信号量 */
	struct sem_queue *sem_pending;		/* 阻塞信号量 */
	struct sem_queue **sem_pending_last;	/* 最后一个阻塞信号量*/
	struct sem_undo	*undo;	/* undo队列 */
	unsigned short	sem_nsems; /* no. of semaphores in array */
};
```

稍后将在下面的示例代码中编写`initsem()`时使用sem_otime成员。

#### 8.3semop()原子操作

所有设置、获取或检查并设置信号量的操作都使用semop()系统调用。这个系统调用是通用的，它的功能是由传递给它的struct sembuf结构来决定的:

```c
struct sembuf{
        unsigned short sem_num;  /* 信号量标号 */
        short          sem_op;   /* 信号量操作（加减） */
        short          sem_flg;  /* 操作标识 */
};
```

当然，sem_num是您想要操作的信号量集中的数量。然后，sem_op是你想对这个信号量做的事情。根据sem_op是正的、负的还是零，这有不同的含义，如下表所示:

- sem_op是要进行的操作（PV操作）：
	* 如果为正整数，表示增加信号量的值（若为3，则加上3）
	* 如果为负整数，表示减小信号量的值
	* 如果为0，表示对信号量当前值进行是否为0的测试


所以，基本上，你要做的是加载一个带有你想要的任何值的struct sembuf，然后调用semop()，像这样:

```c
int semop(int semid, struct sembuf *sops,
          unsigned int nsops);
```

semid参数是从对semget()的调用中获得的数字。接下来是sop，它是一个指向struct sembuf的指针，可以用信号量命令填充该struct sembuf。如果你愿意，还可以创建一个struct sembufs数组，以便同时执行一大堆信号量操作。semop()知道你在做这个的方式是nsop参数，它告诉你发送了多少struct sembufs给它。如果只有一个，把1作为参数。

sembuf结构中还有一个字段是sem_flg字段，它允许程序指定进一步修改semop()调用的效果的标志。

其中一个标志是IPC_NOWAIT，顾名思义，它会导致对semop()的调用在遇到通常会阻塞的情况时返回错误EAGAIN。这对于可能想要“轮询”以查看是否可以分配资源的情况很有用。

另一个非常有用的标志是SEM_UNDO标志。这导致semop()以某种方式记录对信号量所做的更改。当程序退出时，内核将自动撤销所有用SEM_UNDO标志标记的更改。当然，程序应该尽最大努力释放它使用信号量标记的任何资源，但有时当程序获得SIGKILL或发生其他可怕的崩溃时就没法自动撤销该更改了。

#### 8.4摧毁一个信号量

有两种方法可以消除信号量:一种是使用Unix命令ipcrm。另一种方法是通过调用semctl()，并将cmd设置为IPC_RMID。

基本上，需要调用semctl()并将semid设置为想要裁减的信号量ID。cmd应该设置为IPC_RMID，它告诉semctl()删除这个信号量集。semnum参数在IPC_RMID上下文中没有任何意义，可以设置为0。
下面是一个删除信号量集的示例:

```c
int semid; 
.
.
semid = semget(...);
.
.
semctl(semid, 0, IPC_RMID);
```
#### 8.5示例程序

有两个。第一,semdemo.c。如果有必要，创建信号量，并在一个演示中对它执行一些假装的文件锁定，非常类似于文件锁定文档中的操作。第二个程序semrm.c用于销毁信号量(同样，可以使用ipcrm来完成此任务)。

其思想是在几个窗口中运行 semdemo.c生成的可执行程序，查看所有进程是如何交互的。完成后，使用semrm.c生成的可执行程序删除信号量。还可以尝试在运行semdemo.c时删除信号量，以查看生成的错误类型。

下面是semdemo.c
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define MAX_RETRIES 10

union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};

/*
** initsem() -- 改编于 W. Richard Stevens' UNIX 网络编程
*/
int initsem(key_t key, int nsems)  /* 参数key是从ftok()获取的 */
{
    int i;
    union semun arg;
    struct semid_ds buf;
    struct sembuf sb;
    int semid;

    semid = semget(key, nsems, IPC_CREAT | IPC_EXCL | 0666);

    if (semid >= 0) 
    { /* 大于0说明创建成功了并返回相应消息队列标识符 */
        sb.sem_op = 1; sb.sem_flg = 0;
        arg.val = 1;

        printf("press return\n"); getchar();

        for(sb.sem_num = 0; sb.sem_num < nsems; sb.sem_num++) 
        { 
            /* 使用semop()来“释放”信号量 */
            /* 这将设置sem_otime字段，如下所示. */
            if (semop(semid, &sb, 1) == -1) 
            {
                int e = errno;
                semctl(semid, 0, IPC_RMID); /* 清除队列 */
                errno = e;
                return -1; /* 发生了错误并检查errno */
            }
        }

    } 
    else if (errno == EEXIST) 
    { /* 已经存在了 */
        int ready = 0;

        semid = semget(key, nsems, 0); /* 获取存在队列的id */
        if (semid < 0) return semid; /* 发生了错误并检查errno */

        /* 等待其他进程初始化信号量: */
        arg.buf = &buf;
        for(i = 0; i < MAX_RETRIES && !ready; i++) 
        {
            semctl(semid, nsems-1, IPC_STAT, arg);
            if (arg.buf->sem_otime != 0) 
            {
                ready = 1;
            } 
            else 
            {
                sleep(1);
            }
        }
        if (!ready) 
        {
            errno = ETIME;
            return -1;
        }
    } 
    else 
    {
        return semid; /* 发生了错误并检查errno */
    }

    return semid;
}

int main(void)
{
    key_t key;
    int semid;
    struct sembuf sb;
    
    sb.sem_num = 0;
    sb.sem_op = -1;  /* 设置标志为用来分配资源 */
    sb.sem_flg = SEM_UNDO;

    if ((key = ftok("semdemo.c", 'J')) == -1) {
        perror("ftok");
        exit(1);
    }

    /* 获取由semdemo.c为载体的信号量集: */
    if ((semid = initsem(key, 1)) == -1) {
        perror("initsem");
        exit(1);
    }

    printf("Press return to lock: ");
    getchar();
    printf("Trying to lock...\n");

    if (semop(semid, &sb, 1) == -1)  // 占用sem信号量资源,锁住其他的运行不了了因为信号量为1
    { 
        perror("semop");
        exit(1);
    }

    printf("Locked.\n");
    printf("Press return to unlock: ");
    getchar();

    sb.sem_op = 1; /* 释放资源 */
    if (semop(semid, &sb, 1) == -1) {
        perror("semop");
        exit(1);
    }

    printf("Unlocked\n");

    return 0;
}
```

semrm.c如下:

```c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int main(void)
{
    key_t key;
    int semid;

    if ((key = ftok("semdemo.c", 'J')) == -1) {
        perror("ftok");
        exit(1);
    }

    /* 获取由semdemo.c为载体的信号量集: */
    if ((semid = semget(key, 1, 0)) == -1) {
        perror("semget");
        exit(1);
    }

    /* 删除信号量集: */
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl");
        exit(1);
    }

    return 0;
}
```

#### 8.6信号量总结

不要低估了信号量的用处。我向你保证，它们在并发情况下非常非常有用。它们通常也比普通的文件锁更快。同样，你也可以在其他非文件的东西上使用它们，比如共享内存段!事实上，坦白地说，有时没有他们的生活是很难的。

### 9.共享内存

关于共享内存很酷的一点是，它就如它的名字一般:进程之间共享的一段内存。想想它的潜力吧!可以为一个多人游戏分配一个块的玩家信息，并让每个进程(玩家)有访问它的能力!

像往常一样，有很多的陷阱需要提防，但从长远来看，这一切都很容易。只是连接到共享内存，并获得一个指向该内存的指针。可以对这个指针进行读写操作，你所做的所有改变都会被连接到这个段上的其他人看到。再简单不过了。

#### 9.1创建共享内存并连接

类似于System V IPC的其他形式，共享内存段是通过shmget()调用创建和连接的:

```c
int shmget(key_t key, size_t size,
           int shmflg);
```

成功完成后，shmget()返回共享内存段的标识符。key参数的创建与前文中消息队列中所示的一样，使用ftok()。下一个参数size是共享内存的大小(以字节为单位)。最后，如果要创建段，shmflg应该设置为IPC_CREAT。(每次都指定IPC_CREAT也无妨——如果段已经存在，它将简单地连接您。)

下面的例子调用创建一个1K的段，具有644个权限(rw-r——r——):

```c
key_t key;
int shmid;

key = ftok("/home/he/somefile3", 'R');
shmid = shmget(key, 1024, 0644 | IPC_CREAT);
```
但是如何从shmid句柄获得指向该数据的指针呢?答案在下面一节的shmat()调用中。

#### 9.2一个指向共享内存的指针

在可以使用一个共享内存段之前，必须使用`shmat()`调用将你自己附加到它身上:

```c
key_t key;
int shmid;
char *data;

key = ftok("/home/he/somefile3", 'R');
shmid = shmget(key, 1024, 0644 | IPC_CREAT);
data = shmat(shmid, (void *)0, 0);
```

有一个指向共享内存段的指针!注意，shmat()返回一个void\*型的指针，在本例中，我们将它视为一个char指针。可以按照自己喜欢的方式对待这段共享内存，这取决于其中包含的数据类型。指向结构体指针和其他任何东西一样都是可以接受的。

有趣的是shmat()在失败时返回-1。但是如何在空指针中得到-1呢?只需要在比较过程中进行强制转换以检查错误:

```c
data = shmat(shmid, (void *)0, 0);
if (data == (char *)(-1))
    perror("shmat");
```

现在所要做的就是将它所指向的数据更改为普通的指针样式。下一节中有一些示例。

#### 9.3共享内存的读与写

假设你有上面例子中的数据指针。它是一个字符指针，所以我们将从它读取和写入字符。此外，为了简单起见，假设1K的共享内存包含一个以`'\0'` (空)结束的字符串。

再简单不过了。因为它只是一个字符串，可以这样打印它:

```c
printf("shared contents: %s\n", data);
```
也可以很容易地在里面存储一些东西:
```c
printf("Enter a string: ");
gets(data);
```

当然，就像我前面说的，除了字符之外，还可以有其他数据。我只是举个例子。我将假设你对C中的指针足够熟悉，可以处理插入其中的任何类型的数据。

#### 9.4分离和删除共享内存

当你使用完共享内存段后，你的程序应该使用shmdt()调用将自己从它分离出来:

```c
int shmdt(void *shmaddr);
```

唯一的参数shmaddr是您从shmat()获得的地址。错误时返回-1，成功时返回0。

当你从片段中分离出来时，它不会被破坏。当每个人都脱离它时，它也不会消失。你必须使用调用shmctl()来销毁它，类似于对其他System V IPC函数的控制调用:

```c
shmctl(shmid, IPC_RMID, NULL);
```

上面的调用删除共享内存段，假设没有其他连接到它。当然，shmctl()函数的功能远不止于此。

与往常一样，可以使用ipcrm命令从命令行销毁共享内存段。另外，请确保不要留下任何使用过的共享内存段，以免浪费系统资源。可以使用ipcs命令查看拥有的所有System V IPC对象。

#### 9.5并发问题

什么是并发性问题?当有多个进程修改共享内存会出现一些问题。举个例子:当对一个共享对象有多个写入器时,多个写入器对该共享内存的数据进行更新时，可能会出现某些错误。这种并发访问几乎总是一个问题。

解决这个问题的方法是在进程写入共享内存段时使用信号量来锁定它。(有时锁将包含对共享内存的读和写，这取决于你正在做什么。)

关于并发性的真正讨论超出了本文的范围(C和C++并发安全的问题太大太难了这里不赘述)

#### 9.6示例代码

现在，我已经向你介绍了不使用信号量并发访问共享内存段的所有危险，下面我将展示一个演示。由于这不是一个任务关键型应用程序，而且不太可能与其他进程同时访问共享数据，因此为了简单起见，我将不使用信号量。

这个程序做两件事中的一件:如果不带命令行参数运行它，它将打印共享内存的内容。如果给它一个命令行参数，它会将该参数存储在共享内存中。

下面是shmdemo.c
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_SIZE 1024  /* 共享内存大小1k */

int main(int argc, char *argv[])
{
    key_t key;
    int shmid;
    char *data;
    int mode;

    if (argc > 2) {
        fprintf(stderr, "usage: shmdemo [data_to_write]\n");
        exit(1);
    }

    /* 产生一个key: */
    if ((key = ftok("shmdemo.c", 'R')) == -1) {
        perror("ftok");
        exit(1);
    }

    /* 连接(并可能创建)段: */
    if ((shmid = shmget(key, SHM_SIZE, 0644 | IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }

    /* 附加到段以获得指向该段的指针: */
    data = shmat(shmid, (void *)0, 0);
    if (data == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }

    /* 根据命令行读取或修改该段: */
    if (argc == 2) 
    {
        printf("writing to segment: \"%s\"\n", argv[1]);
        strncpy(data, argv[1], SHM_SIZE);
    } 
    else
        printf("segment contains: \"%s\"\n", data);

    /* 从段中分离: */
    if (shmdt(data) == -1) {
        perror("shmdt");
        exit(1);
    }

    return 0;
}
```

更常见的情况是，当其他程序更改和读取共享段时，进程将附加到段上并运行一段时间。观察一个进程更新这个段，并看到这些变化出现在其他进程身上，是一件很简洁的事情。同样，为了简单起见，示例代码并没有这样做，但是可以看到数据是如何在独立进程之间共享的。

此外，这里没有用于删除段的代码,最好得删除。

### 10.内存映射

有时，您需要读写文件，以便在进程之间共享信息。可以这样考虑:两个进程同时打开同一个文件，并对其进行读写操作，从而共享信息。问题是，有时候做所有这些`fseek()`之类的事情是很痛苦的。如果你能把文件的一部分映射到内存中，并得到一个指向它的指针，不是更容易吗?然后，可以简单地使用指针来获取(和设置)文件中的数据。

这就是内存映射文件。而且它也很容易使用。几个简单的调用，加上一些简单的规则,就能做极大的事情。

#### 10.1制造共享文件

在将文件映射到内存之前，你需要使用open()系统调用获取文件描述符:

```c
int fd;

fd = open("mapdemofile", O_RDWR);
```
在本例中，我们打开了文件以获得读/写访问权限。您可以以您想要的任何模式打开它，但是它必须与下面的mmap()调用在prot参数中指定的模式相匹配。
要在内存中映射一个文件，可以使用mmap()系统调用，它的定义如下:

```c
void *mmap(void *addr, size_t len, int prot,
           int flags, int fildes, off_t offset);
```

它的参数参照 [mmap](../docs/mmap.md)

至于返回值，大多数人可能已经猜到的，mmap()在出错时返回-1，并设置errno。否则，它返回一个指向映射数据开始的指针。

我将做一个简短的演示，将文件的第二个“页面”映射到内存中。首先，我将调用open()获取文件描述符，然后使用getpagesize()获取虚拟内存页的大小，并将这个值用于len和offset。通过这种方式，将从第二个页开始映射，并映射一个页的长度。(在我的Linux机器上，页面大小是4K。)

```c
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

int fd, pagesize;
char *data;

fd = open("foo", O_RDONLY);
pagesize = getpagesize();
data = mmap((caddr_t)0, pagesize, PROT_READ, MAP_SHARED, fd,pagesize);
```

一旦这个代码扩展运行，就可以使用data[0]访问文件映射部分的第一个字节。注意这里有很多类型转换。例如，mmap()返回caddr_t，但我们将其视为char*。事实上，caddr_t通常被定义为一个char*，所以一切都没问题。

还要注意，设置了prot为PROT_READ，因此我们有只读访问。任何写入数据的尝试(例如data[0] = 'B')都将导致段错误。如果希望对数据有读写两种权限，可以通过open()函数的O_RDWR参数来获取文件的描述符，并将prot设置为PROT_READ|PROT_WRITE。

#### 10.2消除内存映射

当然，有一个munmap()函数可以取消内存映射文件:

```c
int munmap(caddr_t addr, size_t len);
```

这只是取消了addr(从mmap()返回)指向的区域，其长度为len(与传递给mmap()的len相同)。munmap()错误时返回-1并设置errno变量。

一旦你解除了文件的映射，任何通过旧指针访问数据的尝试都会导致段错误。

最后注意:当然，如果程序退出，该文件将自动取消映射。

#### 10.3内存映射的并发问题

如果有多个进程同时操作同一个文件中的数据，可能会遇到麻烦。可能必须锁住文件或使用信号量来控制对文件的访问，否则进程会破坏文件。(同样本文不做更多的讨论)

#### 10.4简单的用例

这里有一个演示程序，它将自己的源代码映射到内存，并打印指定的位偏移量处找到的字节(当然是显示在终端的)。

该程序将可以指定的偏移量限制在文件长度范围内为0。文件长度是通过调用stat()获得的，这在以前可能没有见过。它返回一个充满文件信息的结构，其中一个字段是以字节为单位的大小。很容易使用该函数。

下面是mmapdemo.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    int fd, offset;
    char *data;
    struct stat sbuf;

    if (argc != 2) {
        fprintf(stderr, "usage: mmapdemo offset\n");
        exit(1);
    }

    if ((fd = open("mmapdemo.c", O_RDONLY)) == -1) {
        perror("open");
        exit(1);
    }

    if (stat("mmapdemo.c", &sbuf) == -1) {
        perror("stat");
        exit(1);
    }

    offset = atoi(argv[1]);
    if (offset < 0 || offset > sbuf.st_size-1) {
        fprintf(stderr, "mmapdemo: offset must be in the range 0-%d\n", \
                                                              sbuf.st_size-1);
        exit(1);
    }
    
    data = mmap((caddr_t)0, sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0)) \
				== (caddr_t)(-1)) {
        perror("mmap");
        exit(1);
    }

    printf("byte at offset %d is '%c'\n", offset, data[offset]);

    return 0;
}
```

编译它然后运行结果如下:

```
mmapdemo 30
byte at offset 30 is 'e'
```

它可以使用这个系统调用编写一些非常酷的程序。

#### 10.5内存映射总结

内存映射非常有用，特别是在不支持共享内存的系统上。事实上，这两者在很多方面非常相似。(内存映射也被提交到磁盘，所以这甚至可能是一个优势，对吗?)有了文件锁定或信号量，内存映射文件中的数据可以很容易地在多个进程之间共享。

### 11.Unix Sockets

Unix域套接字!如套接字是什么，它是一个双向通信管道，可以用于在各种领域进行通信。套接字通信最常见的领域之一是Internet，但我们在这里不讨论这个。然而，我们将讨论Unix领域中的套接字;也就是说，可以在同一个Unix系统上的进程之间使用的套接字。


#### 11.1概述

Unix套接字就像双向的fifo。但是，所有的数据通信都将通过socket接口进行，而不是通过file接口。尽管Unix套接字是文件系统中的一个特殊文件(就像FIFOs一样)，但你不会使用open()和read()去操作它——将使用socket()、bind()、recv()等。

例如，当描述您想要使用哪个Unix套接字时(即套接字所在的特殊文件的路径)，您可以使用sockaddr_un结构，它有以下字段:
```c
struct sockaddr_un {
    unsigned short sun_family;  /* AF_UNIX */
    char sun_path[108];
}
```

这是将传递给bind()函数的结构，该函数将套接字描述符(文件描述符)与某个文件(其名称在sun_path字段中)关联起来。

#### 11.2怎么写一个服务器?

将概述服务器程序为完成任务通常必须经历的步骤，但不涉及太多细节。我将尝试实现一个“echo服务器”，它只是echo回它在套接字上得到的一切数据。

1. 调用socket():使用适当的参数调用socket()将创建Unix套接字:

```c
unsigned int s, s2;
struct sockaddr_un local, remote;
int len;

s = socket(AF_UNIX, SOCK_STREAM, 0);
```

第二个参数SOCK_STREAM告诉socket()创建一个流套接字(顺序的,可靠,双向,面向连接的比特流)。Unix域中支持数据报套接字(提供定长的,不可靠,无连接的通信)，但这里只讨论流套接字()。唯一改变的是，现在使用的是sockaddr_un结构体，而不是sockaddr_in结构体。

2. 调用bind():把从socket()的调用中获得的一个套接字描述符->绑定到Unix域中的一个地址。(如前所述，该地址是磁盘上的一个特殊文件。)

```c
local.sun_family = AF_UNIX;  /* local的声明在调用socket()之前,注意第一段代码 */
strcpy(local.sun_path, "/home/he/mysocket");
unlink(local.sun_path);
len = strlen(local.sun_path) + sizeof(local.sun_family);
bind(s, (struct sockaddr *)&local, len);
```

这将套接字描述符“s”与Unix套接字地址“/home/he/mysocket”相关联。注意，在bind()之前调用unlink()来移除已经存在的套接字。如果文件已经在那里，你会得到一个`EINVAL`错误。

3. 调用listen():这指示套接字侦听来自客户端程序的传入连接:

```c
listen(s, 5);
```
第二个参数5是在调用accept()之前可以传入连接的数量。如果有等于5的连接等待被接受，其他客户端将生成错误`ECONNREFUSED`。

4. 调用accept():这将接受来自客户端的连接。这个函数返回另一个套接字描述符s2!旧的描述符仍然在监听新的连接，但是这个新的连接已经连接到客户端:

```c
len = sizeof(struct sockaddr_un);
s2 = accept(s, &remote, &len);
```

当accept()返回时，远程变量s2将被远程(remote)端`struct sockaddr_un`填充，len将被设置为其长度。描述符s2连接到客户端，并准备好接收send()和recv()。

5. 处理连接并返回accept():通常你会想要在这里与客户端通信(我们只是回送它发送给我们的所有东西)，关闭连接，然后accept()一个新的连接。

```c
while (len = recv(s2, &buf, 100, 0), len > 0)
    send(s2, &buf, len, 0);

/* 这里将返回到accept() */
```

6. 关闭连接:可以通过调用Close()或shutdown()来关闭连接。

综上所述，以下是echo服务器的一些源代码，echos.c。它所做的就是等待Unix套接字(在本例中名为“echo_socket”)上的连接。

```c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "echo_socket" // unix套接字地址,其实是一个特殊的文件

int main(void)
{
    int s, s2, t, len;
    struct sockaddr_un local, remote;
    char str[100];

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)  // 创建unix套接字
    {
        perror("socket");
        exit(1);
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, SOCK_PATH);
    unlink(local.sun_path);         // 如果该地址已经存在了那么移除,后序bind会重新创建一个socket套接字
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(s, (struct sockaddr *)&local, len) == -1)  // 把从socket获得到套接字绑定到一个地址
    {
        perror("bind");
        exit(1);
    }

    if (listen(s, 5) == -1) 
    {
        perror("listen");
        exit(1);
    }

    for(;;) 
    {
        int done, n;
        printf("Waiting for a connection...\n");
        t = sizeof(remote);
        if ((s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1)  // 接收客户端连接,s2将被对端的`struct sockaddr_un`填充
        {
            perror("accept");
            exit(1);
        }

        printf("Connected.\n");

        done = 0;
        do 
        {
            n = recv(s2, str, 100, 0);
            if (n <= 0) 
            {
                if (n < 0) perror("recv");
                done = 1;
            }
            if (!done) 
            {
                if (send(s2, str, n, 0) < 0) 
                {
                    perror("send");
                    done = 1;
                }
            }
        } while (!done);

        close(s2);
    }

    return 0;
}
```

上述所有步骤都包含在这个程序中:调用socket()，调用bind()，调用listen()，调用accept()，并执行一些网络数据发送和接收的操作`send()`和`recv()`。

#### 11.3客户端实现

需要有一个程序来与上述服务器通信，对客户端的实现来说，要比服务器容易得多，因为不需要做任和listen()或accept()操作。以下是步骤:

1. 调用socket()来获得一个Unix域套接字来进行通信。

2. 使用远程地址(服务器正在侦听的地址)设置`struct sockaddr_un`，并将其作为参数调用`connect()`

3. 假设没有错误，说明已经连接到服务端了!尽情使用`send()`和`recv()`吧!

以下是完整代码 echoc.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCK_PATH "echo_socket"

int main(void)
{
    int s, t, len;
    struct sockaddr_un remote; // 服务端的地址
    char str[100];

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
    {
        perror("socket");
        exit(1);
    }

    printf("Trying to connect...\n");

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, SOCK_PATH);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(s, (struct sockaddr *)&remote, len) == -1)  // 连接到remote socket,s通过connect调用获取remote socket
    {
        perror("connect");
        exit(1);
    }

    printf("Connected.\n");

    while(printf("> "), fgets(str, 100, stdin), !feof(stdin))  // 先写入然后循环读取服务端发送来的字节
    {
        if (send(s, str, strlen(str), 0) == -1)  // 发送str到remote
        {
            perror("send");
            exit(1);
        }

        if ((t=recv(s, str, 100, 0)) > 0)  // 接收remote发送来的字符串
        {
            str[t] = '\0';
            printf("echo> %s", str);
        } 
        else 
        {
            if (t < 0) perror("recv");
            else printf("Server closed connection\n");
            exit(1);
        }
    }

    close(s);

    return 0;
}
```

在客户端代码中，要注意到只有几个系统调用用于设置:socket()和connect()。因为客户端不会接受任何传入的连接，所以它不需要listen()。当然，客户端仍然使用send()和recv()来传输数据。差不多就是这样。

#### 11.4socketpair()快速全双工管道

如果想使用`pipe()`，但又想使用单个管道从双方发送和接收数据，该怎么办?因为管道是单向的(SYSV中有例外)。不过，有一种解决方案:使用Unix域套接字，因为它们可以处理双向数据。

不过，这太麻烦了!使用`listen()`和`connect()`等设置所有代码，只是为了双向传递数据!但你猜怎么着!你不必这么做!

没错，有一个称为`socketpair()`的系统调用，它很好地返回一对已经连接的套接字!不需要做额外的工作;可以立即使用这些套接字描述符进行进程间通信。

例如，让我们设置两个进程。第一个将一个字符发送给第二个，第二个将字符更改为大写并返回它。下面是一些简单的代码，称为spair.c(为了代码更简单，没有错误检查):

```c
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

int main(void)
{
	int sv[2]; /* 用来保存一对已连接socket的数组 */
	char buf; /* 用于进程间数据交互 */

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1)  // 申请一对已连接套接字保存在sv中
    {
		perror("socketpair");
		exit(1);
	}

	if (!fork()) 
    {  /* 子进程 */
		read(sv[1], &buf, 1);
		printf("child: read '%c'\n", buf);
		buf = toupper(buf);  /* 使其大写 */
		write(sv[1], &buf, 1);
		printf("child: sent '%c'\n", buf);

	} 
    else 
    { /* 父进程 */
		write(sv[0], "b", 1);
		printf("parent: sent 'b'\n");
		read(sv[0], &buf, 1);
		printf("parent: read '%c'\n", buf);
		wait(NULL); /* 等待子进程结束回收资源 */
	}

	return 0;
}
```

当然，这是一种昂贵的方式来改变一个字符的大小写，但事实上，可以通过此方法让父子进程交流，这才是真正重要的。

要注意的一点是，`socketpair()`同时接受Unix域套接字(AF_UNIX)和流式套接字(SOCK_STREAM)。这些值可以是任何合法的值。

最后，你可能会好奇为什么使用`write()`和`read()`而不是`send()`和`recv()`。某种程度来讲,send和recv是write和read的一个扩展,它们多了第四个参数来控制读写操作(这里简单起见可以少写一点)。

### 12.参考资料

Unix Network Programming, volumes 1-2  by W. Richard Stevens.
Advanced Programming in the UNIX Environment by W. Richard Stevens. 
The Linux Programming Interface by Michael Kerrisk
