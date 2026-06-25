# ここでは, PID名前空間について解説します.

### 概要

前述の通り、プロセスはそれぞれプロセスIDを持ちます。PID名前空間は、プロセスIDの見え方を分離する機能です。これにより、異なるPID名前空間ではプロセスIDの割り当てが独立して行われます。例えば、プロセスIDが1のプロセスは、それぞれのPID名前空間ごとに1つ存在できます。

### 利用方法

`clone`に`CLONE_NEWPID`フラグを設定すると，新しいPID名前空間で子プロセスが開始されます．

```c
clone(child_func, (char*)stack + stack_size, SIGCHLD | CLONE_NEWPID, arg);
```

ここで扱うglibcの`clone`ラッパー関数は，新しいプロセスを作成し，そのプロセスで指定した関数を実行します。成功した場合，親プロセス側の`clone`呼び出しは子プロセスのPIDを返します。`fork`とは異なり，このラッパー関数では子プロセス側で`clone`の返り値`0`を受け取って分岐する形にはなりません。

### 練習

PID名前空間で確かにプロセスIDが独立して割り振られていることを確認してみましょう．

### ディレクトリの作成

好きな場所に`ns_pid`というディレクトリを作成し，その中に以下のようなディレクトリ構造を作成しましょう．

```bash
- [dir] ns_pid
     - [dir] src
            - [file] main.c
     - [dir] build
     - [file] CMakeLists.txt
```

以下を`main.c`に記述します．

### getpidシステムコール

`getpid`システムコールはプロセスのプロセスIDを返します．

```c
pid_t getpid(void);
```

現在のプロセスIDを標準出力に出力するには，以下のように記述します．

```c
printf("PID=%d\n", (int)getpid());
```

### PIDを出力する関数の記述

`getpid`を利用して以下のような関数を記述しましょう．

```c
void print_pid(const char* s) {
    printf("[%s]PID=%d\n", s ? s : "", (int)getpid());
}
```

### main関数の記述

以下のようなmain関数を記述します．

```c
int main() {
    print_pid("parent");
    return 0;
}
```

### ヘッダのインクルード

以下の必要なヘッダファイルをインクルードします．

```c
#include <unistd.h>
#include <stdio.h>
```

### ここまでのコードまとめ

ここまでを行うと以下のようになっていると思います．

```c
#include <unistd.h>
#include <stdio.h>

void print_pid(const char* s) {
    printf("[%s]PID=%d\n", s ? s : "", (int)getpid());
}

int main() {
    print_pid("parent");
    return 0;
}
```

### CMakeLists.txtの記述

以下のように`CMakeLists.txt`に記述します．

```cmake
cmake_minimum_required(VERSION 3.16)
project(ns_pid_example)
set(CMAKE_C_STANDARD 11)

add_executable(example src/main.c)
```

### ビルドと実行

`build`ディレクトリに移動します．

```bash
$ cd ./build
```

`cmake`を実行します．`CMakeLists.txt`を含むディレクトリ，今回は`ns_pid`ディレクトリを`..`で指定しています．

```bash
$ cmake ..
```

`make`を実行します．

```bash
$ make
```

`example`というバイナリが生成されるので，実行します．

```bash
$ ./example
```

プロセスIDが出力されます．

```text
[parent]PID=7561
```

### 子プロセスの実行

子プロセスを新しいPID名前空間で実行するには，`clone`に`CLONE_NEWPID`フラグを指定します．`clone`に渡す関数は`int child(void*)`という形にします．

```c
int child(void* arg) {
    print_pid((const char*)arg);
    return 0;
}
```

スタックの準備などを付け加えて書き換えた`main`関数は以下です．

```c
int main() {
    const size_t stack_size = 1024 * 1024;

    print_pid("parent");

    void* stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }

    pid_t c = clone(child, (char*)stack + stack_size, CLONE_NEWPID | SIGCHLD, "child");
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

### まとめ

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

void print_pid(const char* s) {
    printf("[%s]PID=%d\n", s ? s : "", (int)getpid());
}

int child(void* arg) {
    print_pid((const char*)arg);
    return 0;
}

int main() {
    const size_t stack_size = 1024 * 1024;

    print_pid("parent");

    void* stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }

    pid_t c = clone(child, (char*)stack + stack_size, CLONE_NEWPID | SIGCHLD, "child");
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

前と同様に，ビルドして実行します．PID名前空間の作成には権限が必要な環境が多いため，ここでは`sudo`で実行します．

```bash
$ cd ./build
$ make
$ sudo ./example
```

以下のように結果が出力されると思います．

```bash
[parent]PID=7790
[child]PID=1
```

新しいPID名前空間で最初に作成されたプロセスのプロセスIDは`1`であることがわかります．通常，ホスト側のPIDはプロセスが作成されるごとに既存のPIDと重ならない値になりますが，PID名前空間の内側では独立したPIDが割り当てられます．

完全なサンプルコードは [examples/02-exec-result/pid-namespace](../../examples/02-exec-result/pid-namespace/README.md) にあります。

---
