# 標準入出力について

Cには，標準入出力というものがあります．これは，コンソールからプログラムを実行したときに，文字を入力してプログラムから読み取ったり，またプログラムから出力して文字を表示するために利用するものです．

### 種類

標準入出力には以下の種類があります．

- 標準入力
- 標準出力
- 標準エラー出力

### 利用例

よく知られた`printf`関数は標準出力に文字列を書き込むものです．また，同様によく知られた`scanf`関数は標準入力から文字列を読み込むものです．

### 読み書き

標準入出力を読み書きするには，ファイルの項で説明した`read`システムコールまたは`write`システムコールを用います． 標準入出力はファイルと同じように読み書きできますが，プログラムが開始されたときに自動的に開かれており，`open`システムコールを使用しなくても読み書きすることができます．使用するファイルディスクリプタは予め決まっています．

| 種類 | ファイルディスクリプタの値 |
| --- | --- |
| 標準入力 | `0` |
| 標準出力 | `1` |
| 標準エラー出力 | `2` |

`read`と`write`の使用例を書きます.    標準入出力の読み書きの例:

```c
// 標準入力から読み込む例
char inputBuffer[1024];
ssize_t bytesRead = read(STDIN_FILENO, inputBuffer, sizeof(inputBuffer) - 1);
if (bytesRead != -1) {
    inputBuffer[bytesRead] = '\0';
    printf("Read from standard input: %s\n", inputBuffer);
}

// 標準出力に書き込む例
const char *outputMessage = "Hello, world!\n";
ssize_t bytesWritten = write(STDOUT_FILENO, outputMessage, strlen(outputMessage));
if (bytesWritten != -1) {
    printf("Wrote %zd bytes to standard output.\n", bytesWritten);
}
```

これらをよりわかりやすく扱うため，`unistd.h`というインクルードファイルで，以下のマクロが定義されています．

```c
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
```

完全なサンプルコードは [examples/02-exec-result/stdio](../../examples/02-exec-result/stdio/README.md) にあります。

---
