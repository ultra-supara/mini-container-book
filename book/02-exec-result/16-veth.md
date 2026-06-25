# vethペア

新しいターミナルを開きます．そのターミナルでもroot権限を取得します．

```bash
$ sudo su -
```

以下のコマンドを実行し、vethデバイスペア**`veth1A`**と**`veth1B`**を作成します。

```bash
$ ip link add veth1A type veth peer veth1B
```

その後，ネットワークインターフェイス一覧を確認してみます．

```bash
$ ip link show
```

詳細は異なりますが，おおむね以下のように表示されると思います．

```bash
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN mode DEFAULT group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: ens4: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1460 qdisc mq state UP mode DEFAULT group default qlen 1000
    link/ether 42:01:c0:a8:01:0d brd ff:ff:ff:ff:ff:ff
3: docker0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc noqueue state DOWN mode DEFAULT group default
    link/ether 02:42:8c:b6:77:99 brd ff:ff:ff:ff:ff:ff
4: br-ebd855a484b2: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc noqueue state DOWN mode DEFAULT group default
    link/ether 02:42:b1:3c:53:b2 brd ff:ff:ff:ff:ff:ff
5: veth1B@veth1A: <BROADCAST,MULTICAST,M-DOWN> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/ether ee:f0:b7:02:35:a7 brd ff:ff:ff:ff:ff:ff
6: veth1A@veth1B: <BROADCAST,MULTICAST,M-DOWN> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/ether d6:ba:2b:8b:46:c6 brd ff:ff:ff:ff:ff:ff
```

`veth1A`と`veth1B`の2つのインターフェイスが増えたことがわかります．この2つのインターフェイスで相互にパケットをやりとりできます．

### 名前空間の作成とveth1Bの割り当て

 次に`veth1B`を`ns1`名前空間に移動します．前章から続けていて，すでに`ns1`を作成済みの場合は，`ip netns add ns1`は実行しなくて構いません．

```bash
$ ip netns add ns1
$ ip link set dev veth1B netns ns1
```

ふたたびインターフェイス一覧を確認します．

```bash
$ ip link show
```

詳細は異なりますが，おおむね以下のように表示されると思います．`veth1B`が`ns1`名前空間に移動したためアクセスできなくなり，一覧から表示されなくなったことがわかります．

```bash
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN mode DEFAULT group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: ens4: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1460 qdisc mq state UP mode DEFAULT group default qlen 1000
    link/ether 42:01:c0:a8:01:0d brd ff:ff:ff:ff:ff:ff
3: docker0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc noqueue state DOWN mode DEFAULT group default
    link/ether 02:42:8c:b6:77:99 brd ff:ff:ff:ff:ff:ff
4: br-ebd855a484b2: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc noqueue state DOWN mode DEFAULT group default
    link/ether 02:42:b1:3c:53:b2 brd ff:ff:ff:ff:ff:ff
6: veth1A@if5: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/ether d6:ba:2b:8b:46:c6 brd ff:ff:ff:ff:ff:ff link-netns example-ns1
```

### veth1Aの設定

ここではIPアドレスの割り当てとリンクアップを行います． 先ほど利用したターミナルで，続けて以下のコマンドを入力します．このコマンドは`veth1A`を有効にし，通信ができる状態にします．

```bash
$ ip link set dev veth1A up
```

続けて，以下のコマンドを入力します．このコマンドは`veth1A`にIPv4アドレス`10.0.0.1/24`を割り当てます．

```bash
$ ip address add 10.0.0.1/24 dev veth1A
```

IPアドレスが正常に割り当てられていることを確認します．

```bash
$ ip address show dev veth1A
```

以下のように出力され，IPアドレス`10.0.0.1/24`が正しく割り当てられていることがわかるでしょう．`state LOWERLAYERDOWN`というのは，vethのもう片方のインターフェイスが有効になっていないため，まだ通信ができない状態であることを表します．

```bash
6: veth1A@if5: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc noqueue state LOWERLAYERDOWN group default qlen 1000
    link/ether d6:ba:2b:8b:46:c6 brd ff:ff:ff:ff:ff:ff link-netns example-ns1
    inet 10.0.0.1/24 scope global veth1A
       valid_lft forever preferred_lft forever
```

### veth1Bの設定

ここでは，`ns1`名前空間のインターフェイス`veth1B`へのIPアドレスの割り当てとリンクアップを行います． 先ほど`veth1A`の設定で利用したものとは異なるターミナルで作業を行います．まず，名前空間`ns1`の中でシェルを起動します．

```bash
$ ip netns exec ns1 bash
```

名前空間`ns1`の中でインターフェイス一覧を確認します．

```bash
$ ip link show
```

詳細は異なりますが，おおむね以下のように表示されると思います．`veth1B`が`ns1`名前空間でアクセス可能なインターフェイスとして表示されていることがわかります．

```bash
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
5: veth1B@if6: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/ether ee:f0:b7:02:35:a7 brd ff:ff:ff:ff:ff:ff link-netnsid 0
```

続けて，以下のコマンドを入力します．このコマンドは`veth1B`を有効にし，通信ができる状態にします．

```bash
$ ip link set dev veth1B up
```

以下のコマンドを入力します．このコマンドは`veth1B`にIPv4アドレス`10.0.0.2/24`を割り当てます．

```bash
$ ip address add 10.0.0.2/24 dev veth1B
```

IPアドレスが正常に割り当てられていることを確認します．

```bash
$ ip address show dev veth1B
```

以下のように出力され，IPアドレス`10.0.0.2/24`が正しく割り当てられていることがわかるでしょう．`state UP`となり，通信可能な状態になっています．

```bash
5: veth1B@if6: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether ee:f0:b7:02:35:a7 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 10.0.0.2/24 scope global veth1B
       valid_lft forever preferred_lft forever
    inet6 fe80::ecf0:b7ff:fe02:35a7/64 scope link
       valid_lft forever preferred_lft forever
```

---
