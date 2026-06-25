# initを作る

PID名前空間の中で最初に起動したプロセスは，その名前空間のPID 1になります．LinuxのPID 1は普通のプロセスと少し違います．シグナルの扱いが特別だったり，親を失ったプロセスの引き取り先になったりします．

コンテナの中で最初に起動するプロセスは，このPID 1としての役割を少しだけ背負います．本物のinitほど多くの機能は必要ありませんが，少なくとも次のことを考える必要があります．

- 指定されたコマンドを起動する
- そのコマンドの終了を待つ
- 終了コードを親プロセスへ返せる形にする
- 可能な範囲で子プロセスを回収する

もっとも小さいコンテナなら，子プロセス自身がそのまま`execvp`で目的のコマンドに置き換わっても動きます．しかし，そのコマンドがさらに子プロセスを作る場合，終了した子プロセスを回収する役割が必要です．そこで，この章では簡単な`init`を作ります．

## コマンドを子プロセスとして起動する

`run_init`は，指定されたコマンドをさらに子プロセスとして起動し，終了を待ちます．

```c
static int run_init(char** command) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        execvp(command[0], command);
        perror(command[0]);
        _exit(127);
    }

    int status = 0;
    int command_status = 1;
    for (;;) {
        pid_t waited = wait(&status);
        if (waited == -1) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == ECHILD) {
                break;
            }
            perror("wait");
            return 1;
        }

        if (waited == pid) {
            command_status = status;
            break;
        }
    }

    while (waitpid(-1, &status, WNOHANG) > 0) {
    }

    if (WIFEXITED(command_status)) {
        return WEXITSTATUS(command_status);
    }
    if (WIFSIGNALED(command_status)) {
        return 128 + WTERMSIG(command_status);
    }
    return 1;
}
```

子プロセス側では`execvp`を呼びます．`execvp`に成功すると，そのプロセスは指定されたコマンドに置き換わるため，その下の`perror`には到達しません．到達するのは，コマンドが見つからない，実行権限がない，必要な動的リンカがrootfs内に存在しない，といった失敗時です．

`execvp`に失敗したときは`127`で終了します．シェルでも「コマンドが見つからない」場合によく`127`が使われます．ここではその慣習に合わせています．

## 終了コードを返す

親側の`init`は`wait`で子プロセスの終了を待ちます．`wait`はシグナルで中断されることがあるため，`errno == EINTR`の場合はもう一度待ちます．

目的のコマンドが終了したら，`WIFEXITED`と`WEXITSTATUS`で終了コードを取り出します．シグナルで終了した場合は，よくある慣習に合わせて`128 + signal`を返します．

これにより，外側から見た`mini-container`の終了コードは，コンテナ内で実行したコマンドの終了コードに近くなります．

```bash
$ sudo ./build/mini-container / /bin/true
$ echo $?
0

$ sudo ./build/mini-container / /bin/false
$ echo $?
1
```

この性質は地味ですが重要です．コンテナをスクリプトやCIから使う場合，コンテナ内のコマンドが成功したのか失敗したのかを，外側のプロセスが終了コードで判断できる必要があるからです．
