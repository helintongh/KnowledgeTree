/*
	编译指令:
		gcc -o ssl_hash ssl_hash.c -lssl -lcrypto
*/
#include <stdio.h>
#include <openssl/lhash.h>


#define NAME_LENGTH		32
#define BUFFER_LENGTH	128

typedef struct _Person {
	char name[NAME_LENGTH];	// 名字
	int high;				// 身高
	char otherInfo[BUFFER_LENGTH];// 详细其他信息
} Person;

// 当我们去查找,怎么知道结点是与hash表中的一致的
static int person_cmp(const void *a, const void *b) {
	char *namea = ((Person*)a)->name;
	char *nameb = ((Person*)b)->name;
	return strcmp(namea, nameb);// 如果相等的话,返回0
}

// key值计算,未实现
unsigned long person_key (const void *a) {
	Person *p = (Person*)a;
	long key;
	return key;
}

// 打印数据
static void print_value(void *a) {
	Person *p = (Person*)a;
	printf("name : %s\n", p->name);
	printf("high : %d\n", p->high);
	printf("other info : %s\n", p->otherInfo);

}



int main() {
	// hash的基本用法
	_LHASH *h = lh_new(NULL, person_cmp); // _LHASH这个结构体就代表实例化了一个hash表,第二个参数为比较函数
	if (h == NULL) {
		printf("err.\n");
		return -1;
	}

	Person p1 = {"Hlt", 175, "xxx"};
	Person p2 = {"Messi", 170, "xxx"};
	Person p3 = {"Owen", 170, "xxx"};
	Person p4 = {"Ronaldo", 170, "xxx"};

	lh_insert(h, &p1);	// p1插入到hash表h中
	lh_insert(h, &p2);
	lh_insert(h, &p3);
	lh_insert(h, &p4);

	lh_doall(h, print_value);	// 遍历,第二个参数为执行的回调函数,打印全部的值

	printf("do all --> \n");
	// 通过key查找value
	void *data = lh_retrieve(h, (const char *)"Owen");
	if (data == NULL) {
		return -1;
	}

	print_value(data);

	lh_free(h);

	return 0;
	
}