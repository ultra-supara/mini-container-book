# ネットワーク処理を実行する

ここまでに作ったネットワーク処理を，`start_container`へ組み込みます．重要なのは順番です．親プロセスは，子プロセスを作ったあと，子プロセスをまだ待たせたまま，必要な設定を行います．

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

User名前空間のマッピング，親側ネットワーク設定，NAT設定を終えてから，最後に`notify_child_ready`を呼びます．この時点まで，子プロセスはパイプの`read`で待っています．

## 最小構成で実行する

最初の確認では，rootfsとしてホストの`/`を指定し，`/bin/true`を実行します．

```bash
$ sudo ./build/mini-container / /bin/true
$ echo $?
0
```

この実行では，ファイルシステムの隔離としてはほとんど意味がありません．ホストの`/`をそのままrootfsとして使っているからです．それでも，PID名前空間，Mount名前空間，UTS名前空間，親子同期，`chroot`，簡単な`init`，終了コードの受け渡しが通ることを確認できます．

次に，ホスト名を変えてみます．

```bash
$ sudo ./build/mini-container --hostname mini / /bin/hostname
mini
```

`CLONE_NEWUTS`を使っているため，このホスト名変更はコンテナ内だけに影響します．実行後にホスト側で`hostname`を確認しても，ホスト名は変わっていないはずです．

## Network名前空間を試す

Network名前空間を有効にするには`--network`を付けます．

```bash
$ sudo ./build/mini-container --network / /bin/true
```

この実行では，ホスト側に`mc-host0`，コンテナ側に`eth0`を作成し，コンテナ終了後に`mc-host0`を削除します．中の様子を見たい場合は，シェルを起動します．

```bash
$ sudo ./build/mini-container --network / /bin/sh
```

コンテナ内で確認します．

```bash
# ip address
# ip route
```

`lo`と`eth0`が見え，`eth0`に`10.200.0.2/24`が付いていれば，子プロセス側のネットワーク設定が通っています．デフォルトルートが`10.200.0.1`を向いていれば，ホスト側へパケットを渡す準備もできています．

ホスト側へ疎通確認するなら，コンテナ内から次を実行します．

```bash
# ping -c 1 10.200.0.1
```

ここまで通れば，Network名前空間，veth，IPアドレス設定，ルーティングがつながっています．
