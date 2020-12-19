#include <string.h>
#include <openssl/evp.h>

int main() 
{
	// 100 * 8 / 6 --> 133 --> 100个字符base64编码后大概133字符
	EVP_ENCODE_CTX *ectx, *dctx; // base64编码上下文环境
	unsigned char in[100], base64[150], decode[100];

	ectx = EVP_ENCODE_CTX_new();

	EVP_EncodeInit(ectx);
	int i = 0;
	for (i = 0;i < 100;i ++) { // 对100个字符进行初始化
		in[i] = i;
	} 

	int outl, inl = 100; // 输入的长度是inl,outl函数会传出
	EVP_EncodeUpdate(ectx, base64, &outl, in, inl); // base64编码
	int total = outl;

	EVP_EncodeFinal(ectx, base64+total, &outl); // 进行base64编码,输出结果

	printf("%s\n", base64); 

	// 解码
    dctx = EVP_ENCODE_CTX_new();

	EVP_DecodeInit(dctx);

	int delen;
	EVP_DecodeUpdate(dctx, decode, &delen, base64, total);

	EVP_DecodeFinal(dctx, decode, &delen);

	for (i = 0;i < 100;i ++) { // 对100个字符进行初始化
		printf("%d\n", decode[i]);
	} 

    EVP_ENCODE_CTX_free(dctx);
	return 0;
}
