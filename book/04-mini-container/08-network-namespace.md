# Network名前空間で遊ぶ

`CLONE_NEWNET`を指定すると，子プロセスは新しいNetwork名前空間の中で起動します．起動直後のNetwork名前空間には，基本的にループバックインターフェイス`lo`しかありません．ホスト側や外部ネットワークと通信するには，ネットワークの道を作る必要があります．

ここではvethペアを使います．vethは2つで1組の仮想ネットワークインターフェイスです．片方に入ったパケットは，もう片方から出てきます．この性質を使って，片方をホスト側に置き，もう片方をコンテナ側のNetwork名前空間へ移動します．

```text
host namespace                         container network namespace

mc-host0 10.200.0.1/24  <-------->  eth0 10.200.0.2/24
```

この構成では，ホスト側の`mc-host0`が`10.200.0.1/24`を持ち，コンテナ側の`eth0`が`10.200.0.2/24`を持ちます．コンテナ側のデフォルトゲートウェイは`10.200.0.1`にします．

## なぜ親子で分担するのか

Network名前空間の設定は，親プロセスと子プロセスで分担します．

親プロセスはホスト側のNetwork名前空間にいます．そのため，ホスト側のveth作成や，片方を子プロセスのNetwork名前空間へ移動する処理を担当します．

子プロセスはコンテナ側のNetwork名前空間にいます．そのため，移動されたインターフェイスの名前変更，IPアドレス設定，リンクアップ，デフォルトルート設定を担当します．

この分担は自然です．`ip link set mc-child0 netns <pid>`でインターフェイスを子側へ移動したあとは，親側のNetwork名前空間からそのインターフェイスは見えなくなります．移動後の設定は，子プロセス側で行う必要があります．

## Network名前空間を有効にする

実装では，`--network`が指定されたときに`CLONE_NEWNET`を追加します．

```c
int flags = SIGCHLD | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS;
if (config->use_network) {
    flags |= CLONE_NEWNET;
}
```

`--nat-if`を指定した場合も，ネットワークが必要なので`--network`を暗黙に有効にします．

```c
if (strcmp(argv[index], "--nat-if") == 0) {
    config->use_network = true;
    config->nat_if = argv[index + 1];
}
```

この設計により，外部へ出るNATを使いたいときに，利用者が`--network`を付け忘れても動くようにしています．
