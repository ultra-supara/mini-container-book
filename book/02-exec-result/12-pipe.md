# ここでは，pipeシステムコールの使い方について説明します．

### プロトタイプ

ライブラリ関数のプロトタイプは以下のようになっています．

```c
int pipe(int pipefd[2]);
```

### 動作

**`pipe`**システムコールは、新しいパイプを作成します。パイプは内部にバッファを持ち、2つのファイルディスクリプタを使ってデータを読み書きします。このシステムコールで作成したパイプは、**`0`**番目が読み取り専用で**`1`**番目が書き込み専用です。データは書き込み専用ファイルディスクリプタからバッファを経由して、読み取り専用ファイルディスクリプタに送信されます。

**図: pipeは書き込み端から読み取り端へバッファ経由で流れる**

```mermaid
flowchart LR
    W["子プロセス: write(p[1])"] --> BUF["パイプのバッファ"]
    BUF --> R["親プロセス: read(p[0])"]
```

### 引数

パイプのファイルディスクリプタを格納する`int[2]`型のポインタを渡します．

### 返り値

成功した場合`0`で，失敗した場合は`-1`です．

### 使用方法

パイプを作成するには，まず作成したパイプのファイルディスクリプタを格納する配列を準備します．

```c
int fd[2] = {0};
```

その後，`pipe`システムコールを呼びます．

```c
pipe(fd);
```

## 例

以下のプログラムは、**`fork`**を使って親プロセスと子プロセスで通信するためにパイプを使用します。

```c
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUTPUT_BUFFER_SIZE 1024

int main() {
    int p[2];
    if (pipe(p) == -1) {
        perror("pipe failed!");
        return -1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed!");
        return -1;
    }

    if (pid == 0) { // Child process
        close(p[0]); // Close read end of the pipe
        write(p[1], "child\n", 6);
        close(p[1]); // Close write end of the pipe
    } else { // Parent process
        close(p[1]); // Close write end of the pipe
        char buf[OUTPUT_BUFFER_SIZE] = {0};
        ssize_t sz = read(p[0], buf, OUTPUT_BUFFER_SIZE);
        if (sz < 0) {
            perror("read failed!");
            return -1;
        }
        write(STDOUT_FILENO, buf, sz);
        close(p[0]); // Close read end of the pipe

        if (waitpid(pid, NULL, 0) == -1) {
            perror("waitpid failed!");
            return -1;
        }
    }

    return 0;
}
```

この例では、親プロセスが**`pipe`**システムコールを使ってパイプを作成し、その後**`fork`**システムコールで子プロセスを生成します。子プロセスは、パイプの書き込み専用ファイルディスクリプタを使ってデータをパイプに送信します。一方、親プロセスは、パイプの読み取り専用ファイルディスクリプタを使ってデータを受信し、標準出力に表示します。最後に、親プロセスは**`waitpid`**で子プロセスが終了するのを待ちます。

この例では、親プロセスと子プロセス間で簡単なデータ通信を実現しています。**`pipe`**システムコールは、プロセス間通信（IPC）の基本的な仕組みのひとつであり、他の高度なIPC機構（例えばソケットや共有メモリ）と組み合わせることで、より複雑な通信シナリオを実現できます。

完全なサンプルコードは [examples/02-exec-result/pipe](../../examples/02-exec-result/pipe/README.md) にあります。

---
