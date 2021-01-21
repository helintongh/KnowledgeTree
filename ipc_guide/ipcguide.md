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