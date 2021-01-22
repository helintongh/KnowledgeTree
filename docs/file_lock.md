## 文件锁

文件锁分为mandatory和advisory,我主要介绍advisory锁这个锁更有实际意义。

此锁总共有两个部分组成结构体`struct flock`和加/解锁函数`fcntl()`

## flock结构

```c
struct flock
{
	short int l_type;  	// 指定锁的类型;F_RDLCK、F_WRLCK或F_UNLCK之一。
	short int l_whence; // 这对应于fseek或lseek的where参数，并指定偏移量相对于什么。它的值可以是SEEK_SET、SEEK_CUR或SEEK_END之一。
	off_t l_start;		// 这指定了锁所应用的区域的起始偏移量，以字节为单位给出了相对于l_wherth成员所指定的点的偏移量。
	off_t l_len;		// 指定要锁定的区域的长度。0的值会被特殊处理;这意味着该区域扩展到文件的末尾。
	pid_t l_pid;		// 该字段为进程ID

};
```

## 利用fcntl加锁或解锁

加锁和解锁都是设置flock结构体的l_type字段,改变字段的值就是加解锁操作了。

加锁如下:

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

解锁如下:

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