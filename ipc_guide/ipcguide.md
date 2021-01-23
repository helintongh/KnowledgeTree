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