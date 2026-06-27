# ここではUser 名前空間について説明します

### 概要

Linuxでは、複数のユーザーやグループを扱うことができます。User名前空間は、ユーザーID（UID）やグループID（GID）の見え方を分離する機能です。

### マッピング

元のUser名前空間のユーザやグループと新しいUser名前空間のユーザやグループを対応づけることができます．これを利用すると，元のUser名前空間の一般ユーザが新しいUser名前空間ではrootに見える，という状態を作ることができます．対応付けられていないユーザやグループは，通常`65534`というoverflow IDとして見えます．多くの環境ではこのIDに`nobody`という名前が対応していますが，名前の表示は`/etc/passwd`などの設定にも依存します．

### uid_map

ユーザIDのマッピングは，`/proc`にあるファイルを利用して行います．プロセスIDが`1234`のとき，このファイルは`/proc/1234/uid_map`です．このファイルはプロセスごとに存在し，元の名前空間にあるプロセスから書き込みます．このファイルは改行で区切られた数行からなり，各行はスペース区切りの3つの整数からなります．これらの意味するところは，前から順に新しい名前空間でのユーザID，元の名前空間でのユーザID，マッピングするユーザIDの数となっています． 例として以下のマッピングを考えます.

```text
0 1000 3
```

このマッピングは，以下のように元の名前空間でのユーザIDと新しい名前空間でのユーザIDを対応付けます．

| 元の名前空間 | 新しい名前空間 |
| --- | --- |
| 1000 | 0 |
| 1001 | 1 |
| 1002 | 2 |

### パーミッション

ファイルの所有者や所有グループの表示も，上で述べたマッピングにより対応付けられます． 例として，元の名前空間での`para`というユーザが新しい名前空間で`root`ユーザにマッピングされているとします．元の名前空間で所有者が`para`だったファイルは，新しい名前空間では所有者が`root`に見えます．逆に，新しい名前空間で`root`を所有者として新しいファイルを作成すると，元の名前空間ではそのファイルの所有者は`para`に見えます．元の名前空間での`syslog`というユーザが新しい名前空間にマッピングされていないとすると，所有者が`syslog`のファイルは，新しい名前空間ではoverflow IDとして見えます．

### User名前空間と他の名前空間との関係

User名前空間は、他の名前空間（例：ネットワーク名前空間、マウント名前空間など）と組み合わせて使われます。User名前空間の中でrootに見えるプロセスは、そのUser名前空間が所有する他の名前空間に対して権限を持てます。一方で、ホスト側の名前空間やホスト側のファイルに対して、そのままホストrootと同じ権限を得るわけではありません。

### DockerとUser名前空間

Dockerなどのコンテナランタイムでは、設定によりUser名前空間を利用できます。User名前空間を有効にすると、コンテナ内の**`root`**ユーザーをホスト上の別の一般ユーザーID範囲へマッピングできます。この機能により、コンテナ内でrootとして動作するプロセスが侵害されても、ホスト上ではrootではないユーザーとして扱われるようにできます。

### 権限はどう見えるか

User名前空間を利用してユーザやグループが別のものに見えても，ホスト上のファイルアクセスはUID/GIDのマッピングに基づいて判定されます．元のUser名前空間での一般ユーザが新しいUser名前空間でrootに見えても，マッピングされていないホスト上の他ユーザのファイルに自由にアクセスできるわけではありません．一方で，新しいUser名前空間の中では多くのcapabilityを持つため，そのUser名前空間が所有する名前空間内の操作は可能になります．

### 利用方法

`clone`に`CLONE_NEWUSER`フラグを設定すると，新しいUser名前空間でプロセスが開始されます．一般ユーザで実行できるかどうかはカーネルやディストリビューションの設定に依存します．

```c
clone(child_func, (char*)stack + stack_size, SIGCHLD | CLONE_NEWUSER, arg);
```

### 練習

好きな場所に`ns_user_example`というディレクトリを作成し，その中に以下のようなディレクトリ構造を作成しましょう．

```bash
- [dir] ns_user_example
     - [dir] src
            - [file] main.c
     - [dir] build
     - [file] CMakeLists.txt
```

以下のCコードは`main.c`に記述します．

### getuidシステムコール

`getuid`システムコールは呼び出したプロセスを実行しているユーザのユーザIDを返します．

```c
uid_t getuid(void);
```

ユーザIDを標準出力に出力するには，以下のように記述します．

```c
printf("UID=%d\n", (int)getuid());
```

### UIDを出力する関数の記述

`getuid`を利用して以下のような関数を記述しましょう．

```c
void print_uid(const char* s) {
    printf("[%s]UID=%d\n", s ? s : "", (int)getuid());
    fflush(stdout);
}
```

### main関数の記述

以下のようなmain関数を記述します．

```c
int main() {
    print_uid("parent");
    return 0;
}
```

### ヘッダのインクルード

以下の必要なヘッダファイルをインクルードします．

```c
#include <unistd.h>
#include <stdio.h>
```

### ここまでのCコードのまとめ

ここまでを行うと以下のようになっていると思います．

```c
#include <unistd.h>
#include <stdio.h>

void print_uid(const char* s) {
    printf("[%s]UID=%d\n", s ? s : "", (int)getuid());
    fflush(stdout);
}

int main() {
    print_uid("parent");
    return 0;
}
```

### CMakeLists.txtの記述

以下のように`CMakeLists.txt`に記述します．

```cmake
cmake_minimum_required(VERSION 3.16)
project(ns_user_example)
set(CMAKE_C_STANDARD 11)

add_executable(example src/main.c)
```

### ビルドと実行

`build`ディレクトリに移動します．

```bash
$ cd ./build
```

`cmake`を実行します．`CMakeLists.txt`を含むディレクトリを`..`で指定しています．

```bash
$ cmake ..
```

`make`を実行します．

```bash
$ make
```

`example`というバイナリが生成されるのでこれを実行します．

```bash
$ ./example
```

ユーザIDが出力されます．

```text
[parent]UID=1011
```

### 子プロセスの実行

子プロセスを新しいUser名前空間で実行するには，`clone`に`CLONE_NEWUSER`フラグを指定します．`clone`に渡す関数は`int child(void*)`という形にします．

```c
int child(void* arg) {
    print_uid((const char*)arg);
    return 0;
}
```

スタックの準備などを付け加え，以下のようにmain関数を変更します．

```c
int main() {
    const size_t stack_size = 1024 * 1024;

    print_uid("parent");

    void* stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }

    pid_t c = clone(child, (char*)stack + stack_size, CLONE_NEWUSER | SIGCHLD, "child");
    if (c == -1) {
        perror("clone failed");
        munmap(stack, stack_size);
        return -1;
    }

    waitpid(c, NULL, 0);
    munmap(stack, stack_size);
    return 0;
}
```

### 追加のヘッダのインクルード

以下のヘッダファイルを追加でインクルードします．

```c
#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
```

### ここまでのコードのまとめ

ここまでをすべて行うと次のようになります．

```c
#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

void print_uid(const char* s) {
    printf("[%s]UID=%d\n", s ? s : "", (int)getuid());
    fflush(stdout);
}

int child(void* arg) {
    print_uid((const char*)arg);
    return 0;
}

int main() {
    const size_t stack_size = 1024 * 1024;

    print_uid("parent");

    void* stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }

    pid_t c = clone(child, (char*)stack + stack_size, CLONE_NEWUSER | SIGCHLD, "child");
    if (c == -1) {
        perror("clone failed");
        munmap(stack, stack_size);
        return -1;
    }

    waitpid(c, NULL, 0);
    munmap(stack, stack_size);
    return 0;
}
```

### ビルドと実行

ビルドと実行の手順は以下の通りです。

```bash
$ cd ./build
$ cmake ..
$ make
$ ./example
```

出力されるユーザIDを確認します。

```bash
[parent]UID=1011
[child]UID=65534
```

今回はユーザIDのマッピングを行わなかったので，新しい名前空間では親のUIDが対応付けられておらず，overflow IDである**`65534`**として見えています。次の段階では，`uid_map`や`gid_map`にマッピングを書き込むことで，新しいUser名前空間の中でUID 0に見せることができます。

完全なサンプルコードは [examples/02-exec-result/user-namespace](../../examples/02-exec-result/user-namespace/README.md) にあります。

---
