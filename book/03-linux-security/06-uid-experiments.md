# uidのふるまいを実験してみよう！

### 概要

先に述べたように、プロセスのユーザーIDには`euid、suid、ruid`があります. それぞれを操作したり子プロセスを作ったりして挙動を見てみます.

### 実験1 : ユーザーIDの表示

```c
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(int argc, char** argv) {
	uid_t ruid,euid,suid;
	int ret = getresuid(&ruid,&euid,&suid);
	if(ret != 0) {
		printf("getresuid: %s\n",strerror(errno));
		return -1;
	}
	printf("euid: %d; suid:%d; ruid:%d\n",euid,suid,ruid);
}
```

**実行**

```bash
$ gcc test1.c -Wall -o test1
./test1
sudo ./test1 

#sudoを使うと どうなる？
```

### 実験2 : euidを変えてみよう

```c
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

void print_error() {
	printf("error: %s\n",strerror(errno));
}

void print_uid() {
	uid_t ruid,euid,suid;
	int ret = getresuid(&ruid,&euid,&suid);
	if(ret != 0) print_error();
	else printf("euid: %d; suid:%d; ruid:%d\n",euid,suid,ruid);
}

int main(int argc,char ** argv) {
	if(argc < 3) {
		printf("test2 euid1 euid2\n");
		return -1;
	}
	int euid[] = {atoi(argv[1]),atoi(argv[2])};
	print_uid();
	printf("seteuid(%d)\n",euid[0]);
	if(seteuid(euid[0]) != 0) print_error();
	print_uid();
	printf("seteuid(%d)\n",euid[1]);
	if(seteuid(euid[1]) != 0) print_error();
	print_uid();
}
```

### 実行

```bash
$ gcc test2.c -Wall -o test2
$ ./test2 0 1000 #一般ユーザーからrootになろうとして失敗することを確認する
$ sudo ./test2 1000 0 #rootを捨ててその後rootになることを試みる
```

### 実験3 : euid、ruid、suidを変える

```c
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

void print_error() {
	printf("error: %s\n",strerror(errno));
}

void print_uid() {
	uid_t ruid,euid,suid;
	int ret = getresuid(&ruid,&euid,&suid);
	if(ret != 0) print_error();
	else printf("euid: %d; suid:%d; ruid:%d\n",euid,suid,ruid);
}

int main(int argc,char ** argv) {
	if(argc < 3) {
		printf("test2 uid1 uid2\n");
		return -1;
	}
	int uid[] = {atoi(argv[1]),atoi(argv[2])};
	print_uid();
	printf("setresuid(%d,%d,%d)\n",uid[0],uid[0],uid[0]);
	if(setresuid(uid[0],uid[0],uid[0]) != 0) print_error();
	print_uid();
	printf("setresuid(%d,%d,%d)\n",uid[1],uid[1],uid[1]);
	if(setresuid(uid[1],uid[1],uid[1]) != 0) print_error();
	print_uid();
}
```

### 実行

```bash
$ gcc test3.c -Wall -o test3
$ ./test3 0 1000 #一般ユーザーからrootになろうとして失敗することを確認する
$ sudo ./test3 1000 0 #rootを捨ててその後rootになることを試みる
```

---
