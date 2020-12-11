### 压缩包管理

1. Linux常用压缩包格式

gz后缀: gzip
bz2: bzip

2. 压缩相关命令

tar, rar, zip/unzip。

#### tar指令

tar对应参数

1. c - 创建

2. x - 释放

3. v - 打印提示

4. f - 指定名字

5. z - 使用gzip压缩文件

6. j - 使用bzip2的方式压缩文件

- 压缩

`tar 参数 原材料(可多个)`

```Shell
tar zcvf test.tar.gz file_dir1 file_dir2 # 该命令将file_dir1和file_dir2压缩进test.tar.gz
```

- 解压

```Shell
tar zxvf test.tar.gz

tar zxvf test.tar.gz -C 解压到的路径
```

#### rar格式

压缩与解压

```Shell
# 压缩
rar a 压缩包的名字 压缩内容(如果是目录则需要-r参数)

# 解压
rar x 压缩包名 解压文件存放路径
```

#### zip/unzip

压缩与解压

```Shell
zip 参数 压缩包名 原材料(有目录加-r)

unzip 压缩包 -d 指定目录
```