# PID名前空間とUser名前空間で遊ぶ

コンテナらしさを強く感じる部分の1つは，コンテナ内から見えるIDがホスト側と違うことです．PID名前空間ではプロセスIDの見え方が変わり，User名前空間ではUID/GIDの見え方が変わります．

この章のサンプルでは，PID名前空間は常に有効にし，User名前空間は`--userns`を指定したときだけ有効にします．

```c
int flags = SIGCHLD | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS;
if (config->use_userns) {
    flags |= CLONE_NEWUSER;
}
```

## PID名前空間を見る

PID名前空間の中で最初に起動したプロセスは，その名前空間ではPID 1として見えます．ホスト側から見たPIDとは別の番号空間になるため，同じプロセスでも見えるPIDが異なります．

`--mount-proc`を使うと，コンテナ内で`/proc`を見られます．rootfsとしてホストの`/`を使う実験なら，次のように`ps`を実行できます．

```bash
$ sudo ./build/mini-container --mount-proc / /bin/ps -ef
```

また，`/proc/self/status`を見ると，プロセスの状態を確認できます．

```bash
$ sudo ./build/mini-container --mount-proc / /bin/cat /proc/self/status
```

ここで見たいのは，「ホスト側のPID」と「コンテナ内から見えるPID」が同じではないという点です．PID名前空間は，プロセスIDの番号空間を分離します．

## User名前空間を見る

User名前空間を有効にするには`--userns`を付けます．

```bash
$ sudo ./build/mini-container --userns / /usr/bin/id
uid=0(root) gid=0(root) groups=0(root)
```

コンテナ内ではUID 0に見えます．しかし，ホスト全体のroot権限をそのまま得たわけではありません．親プロセスが`uid_map`と`gid_map`に書いた対応関係により，コンテナ内のUID 0がホスト側の実UIDへ対応付けられています．

この性質が，rootlessコンテナの基礎になります．「コンテナ内ではrootに見えるが，ホスト側では一般ユーザーとして扱われる」という状態を作れるからです．ただし，ファイル所有者やネットワーク設定の扱いは単純ではありません．rootlessで実用的なコンテナを作るには，subuid/subgid，ユーザー空間ネットワーク，補助プログラムなども必要になります．

## 何が分離され，何が分離されないか

PID名前空間はPIDの見え方を変えますが，ファイルシステムを変えるわけではありません．User名前空間はUID/GIDの見え方を変えますが，ネットワークデバイスを自動で用意するわけではありません．

この点を混同しないことが重要です．コンテナは1つの機能ではなく，複数の機能の組み合わせです．PID名前空間，User名前空間，Mount名前空間，Network名前空間，`chroot`，veth，NATをそれぞれ別の部品として見ておくと，どこで何が起きているか切り分けやすくなります．
