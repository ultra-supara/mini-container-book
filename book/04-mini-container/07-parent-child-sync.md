# 親プロセスの準備を待ってから実行する

子プロセスは新しい名前空間の中で起動します．しかし，起動した瞬間にすぐ`chroot`やコマンド実行へ進んでしまうと，親プロセスがまだ準備を終えていない可能性があります．

たとえばUser名前空間を使う場合，親プロセスは`/proc/<pid>/uid_map`と`/proc/<pid>/gid_map`を書き込む必要があります．Network名前空間を使う場合，親プロセスはvethペアを作って，片方を子プロセスのNetwork名前空間へ移動する必要があります．

そのため，この章の実装ではパイプを使って，親子のタイミングをそろえます．

## パイプを作る

`start_container`の中で同期用のパイプを作ります．

```c
if (pipe(config->sync_pipe) != 0) {
    perror("pipe");
    munmap(stack, STACK_SIZE);
    return 1;
}
```

このパイプはデータを運ぶためではなく，タイミングをそろえるために使います．親が1バイトを書き込むまで，子は`read`で止まります．

## 子プロセスは待つ

子プロセス側では，最初にパイプを読んで待機します．

```c
static int wait_parent_ready(int fd) {
    char c;
    for (;;) {
        ssize_t n = read(fd, &c, 1);
        if (n == 1) {
            return 0;
        }
        if (n == -1 && errno == EINTR) {
            continue;
        }
        if (n == 0) {
            fprintf(stderr, "parent closed sync pipe before container was ready\n");
        } else {
            perror("read sync");
        }
        return -1;
    }
}
```

`read`が1バイト読めたら，親の準備が終わったと判断します．`EINTR`はシグナルによる中断なので，再試行します．親が何も書かずにパイプを閉じた場合は，準備に失敗したと考えます．

## 親プロセスは通知する

親プロセス側では，UID/GIDマッピングやネットワーク設定が終わったあとに1バイトを書き込みます．

```c
static int notify_child_ready(int fd) {
    for (;;) {
        ssize_t n = write(fd, "1", 1);
        if (n == 1) {
            return 0;
        }
        if (n == -1 && errno == EINTR) {
            continue;
        }
        perror("write sync");
        return -1;
    }
}
```

この1バイトの内容に意味はありません．重要なのは，子プロセスの`read`を解除することです．

## start_containerでの順番

親側の流れは次のようになります．

```c
if (config->use_userns &&
    setup_user_map(child, getuid(), getgid()) != 0) {
    goto fail;
}

if (config->use_network && setup_parent_network(child) != 0) {
    goto fail;
}

if (config->nat_if != NULL && setup_nat(config->nat_if) != 0) {
    goto fail;
}

if (notify_child_ready(config->sync_pipe[1]) != 0) {
    goto fail;
}
```

順番に注目してください．User名前空間のマッピング，親側ネットワーク設定，NAT設定を終えてから，最後に`notify_child_ready`を呼びます．この時点まで，子プロセスはパイプの`read`で待っています．

この構造により，「子を作る」と「子を本格的に動かす」の間に，親が必要な準備を差し込めます．
