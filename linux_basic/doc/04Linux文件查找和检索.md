### 文件查找和检索

#### find查找

1. 通过name查找

```Shell
find 查找的目录 -name "查找的文件名"

find . -name "program"
```

2. 通过文件类型查找

```Shell
find 目录 -type 文件类型

find ./ -type f
```

注linux有7种类型,find也支持7种。

	- 普通文件 f
	- 目录 d
	- 符号链接 l
	- 管道 p
	- 套接字 s
	- 字符设备 c
	- 块设备 b

3. 通过大小查找

```Shell
find . -size +10k -size -100k # 查找大于10k小于100k的文件
```

#### 高级用法(shell脚本查找)
