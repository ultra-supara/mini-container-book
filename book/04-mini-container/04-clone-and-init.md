# 子プロセスの作成とinitの実行

コンテナの中で動くプロセスは，`clone`で作成します．`fork`でも子プロセスは作れますが，名前空間を作りながらプロセスを起動したいので，ここでは`clone`を使います．

`clone`に渡すフラグで，どの名前空間を新しく作るかを指定します．

```c
int flags = SIGCHLD | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS;
if (config->use_network) {
    flags |= CLONE_NEWNET;
}
if (config->use_userns) {
    flags |= CLONE_NEWUSER;
}

pid_t child = clone(container_child, (char*)stack + STACK_SIZE, flags, config);
```

ここで指定している名前空間は次の通りです．

| フラグ | 分離されるもの |
| --- | --- |
| `CLONE_NEWPID` | プロセスIDの見え方 |
| `CLONE_NEWNS` | マウントテーブル |
| `CLONE_NEWUTS` | ホスト名とドメイン名 |
| `CLONE_NEWNET` | ネットワークデバイス，IPアドレス，ルーティング |
| `CLONE_NEWUSER` | UID/GIDとcapabilityの見え方 |

`CLONE_NEWNET`と`CLONE_NEWUSER`はオプションで有効にします．最初の実験ではネットワークやUser名前空間を使わず，PID，Mount，UTS名前空間だけで動かしてみると理解しやすいです．そのあとで`--userns`や`--network`を足していきます．

## スタックを用意する

glibcの`clone`ラッパーでは，子プロセス用のスタックを呼び出し側で用意します．完成版では`mmap`で1MiBの領域を確保しています．

```c
void* stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN | MAP_STACK, -1, 0);
if (stack == MAP_FAILED) {
    perror("mmap");
    return 1;
}
```

`clone`へ渡すのはスタックの末尾側です．多くのアーキテクチャではスタックは高いアドレスから低いアドレスへ伸びるため，`(char*)stack + STACK_SIZE`を渡します．

```c
pid_t child = clone(container_child, (char*)stack + STACK_SIZE, flags, config);
```

`fork`の場合は親子とも同じ場所から処理が続きますが，glibcの`clone`ラッパーでは，子プロセスは指定した関数から実行を開始します．ここでは`container_child`が子プロセスの入口です．

## 子プロセスの入口

子プロセスの入口は，次の順番で処理します．

1. 親プロセスの準備が終わるまで待つ
1. UTS名前空間内のホスト名を設定する
1. Network名前空間を使う場合は，コンテナ側のネットワークを設定する
1. Mount名前空間内で`chroot`と`chdir`を行う
1. 必要なら`/proc`をマウントする
1. `init`としてコマンドを実行する

実装は次のようになります．

```c
static int container_child(void* arg) {
    struct container_config* config = arg;
    close(config->sync_pipe[1]);

    if (wait_parent_ready(config->sync_pipe[0]) != 0) {
        return 1;
    }
    close(config->sync_pipe[0]);

    if (sethostname(config->hostname, strlen(config->hostname)) != 0) {
        perror("sethostname");
        return 1;
    }

    if (config->use_network && setup_child_network() != 0) {
        return 1;
    }

    if (setup_rootfs(config->rootfs) != 0) {
        return 1;
    }

    if (config->mount_proc && mount_procfs() != 0) {
        return 1;
    }

    return run_init(config->command);
}
```

`sethostname`は`CLONE_NEWUTS`と組み合わせて使います．UTS名前空間が分離されていれば，ここで設定したホスト名はコンテナ内だけに影響し，ホスト側のホスト名は変わりません．

`setup_child_network`を`chroot`より前に呼んでいる点にも注目してください．この関数では`ip`コマンドを実行します．`chroot`したあとだと，rootfsの中に`ip`コマンドや必要な共有ライブラリが入っていなければ実行できません．`chroot`前であれば，ファイルシステムとしてはホスト側の`ip`コマンドを使いながら，Network名前空間だけは子プロセスのものを設定できます．
