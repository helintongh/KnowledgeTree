/*
	编译指令:
		gcc -o socket_bio socket_bio.c -lssl -lcrypto
*/
#include <stdio.h>
#include <openssl/bio.h>

#define OUT_LENGTH		128

int main() {
	int sock = BIO_get_accept_socket("8899", 0); // 建立一个listen的socket,监听端口号为8899
	BIO *b = BIO_new_socket(sock, BIO_NOCLOSE);  // 收到的数据放入b中

	char *addr = NULL;
	int ret = BIO_accept(sock, &addr); // 返回监听的socket的ip存到addr里
	BIO_set_fd(b, ret, BIO_NOCLOSE);

	while (1) {
		char out[OUT_LENGTH] = {0};
		BIO_read(b, out, OUT_LENGTH); // 接收数据,放入到out字符串中
		if(out[0] == 'q') {
			break;
		}
		printf("%s\n", out);
	}

	BIO_free(b); // 释放掉缓存b

	return 0;

}