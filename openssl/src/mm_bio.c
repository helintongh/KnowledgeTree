/*
	编译指令:
		gcc -o mm_bio mm_bio.c -lssl -lcrypto
	注:bio mem实际上是一个自己的内存池
*/
#include <stdio.h>
#include <openssl/bio.h>

/* 往内存块里存数据 */
int main() 
{

	BIO *b = BIO_new(BIO_s_mem()); // 创建了一块内存空间(封装了,实现了实例化)

	int len = BIO_write(b, "openssl", 7);	// 往b内存块里写openssl
	len = BIO_printf(b, "%s", "Hlt"); // 往b里写入字符串Hlt
	
	len = BIO_ctrl_pending(b); // 获取占用的空间大小

	char out[128] = {0};
	len = BIO_read(b, out, len);	// 读出b内存里的值放入到out字符串中
	printf("out: %s\n", out);

	BIO_free(b);

	return 0;
}


