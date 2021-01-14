## 文件I/O方式分为`两`种：
* 基于文件描述符的I/O操作（Linux系统API）
* 基于数据流的I/O操作（标准C）

## 基于文件描述符
基于文件描述符的I/O常用的就是`5`个系统函数：

|[open][1]|[read][2]|[write][3]|[lseek][4]| close
|----|----|-----|-----|-----

*****
[1]:open.md 
[2]:read.md
[3]:write.md
[4]:lseek.md

## 基于数据流
基于数据流的I/O操作常用的函数有：

|fopen|fclose|fread|fscanf|fwrite|getc
|-----|------|-----|------|-----|-----


