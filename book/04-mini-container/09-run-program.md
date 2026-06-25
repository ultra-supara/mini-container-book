# コマンド実行処理を書く

ネットワーク設定では，`ip`や`iptables`などの外部コマンドを実行します．最初は`system("ip link ...")`のように書きたくなりますが，この実装では`fork`と`execvp`を使い，引数配列として実行します．

シェルを経由しないことには意味があります．外部から受け取ったインターフェイス名を文字列連結してシェルに渡すと，クォートやメタ文字の扱いを間違える危険があります．引数配列として`execvp`へ渡せば，少なくともシェルによる再解釈は避けられます．

## 外部コマンドを実行する関数

完成版では次のような`run_program`を使っています．

```c
static int run_program(char* const argv[], bool allow_failure) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        if (allow_failure) {
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull != -1) {
                dup2(devnull, STDOUT_FILENO);
                dup2(devnull, STDERR_FILENO);
                close(devnull);
            }
        }
        execvp(argv[0], argv);
        perror(argv[0]);
        _exit(127);
    }

    int status = 0;
    if (wait_for_child(pid, &status) != 0) {
        return -1;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return 0;
    }

    return allow_failure ? 0 : -1;
}
```

`allow_failure`は，失敗しても処理を続けたいコマンドに使います．たとえば古いvethを削除する処理では，そもそもvethが存在しないことがあります．その場合は失敗しても問題ありません．一方，vethの作成やIPアドレスの設定に失敗した場合は，コンテナのネットワークが壊れた状態になるため，エラーとして扱います．

## 引数配列で書く

`run_program`には，次のような引数配列を渡します．

```c
char* const up_loopback[] = {"ip", "link", "set", "lo", "up", NULL};
run_program(up_loopback, false);
```

最後の`NULL`は，`execvp`に渡す引数配列の終端です．これは`argv`と同じ形式です．

シェル文字列であれば次のように1つの文字列になります．

```bash
ip link set lo up
```

しかしCの実装では，コマンド名と各引数を分けて持ちます．

```c
{"ip", "link", "set", "lo", "up", NULL}
```

この形式に慣れておくと，`execve`や`execvp`を使うプログラムを書きやすくなります．
