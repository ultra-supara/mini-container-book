# ここではnetwork名前空間について記述します.

### 概要

LinuxではイーサネットなどのアダプタやIPルーティングなどのネットワーク機能を利用できます．これらの機能を分離するのがNetwork名前空間です．

### デバイス

ネットワークインターフェイス，たとえばイーサネットアダプタなどはただ1つのNetwork名前空間に属することができ，その他のNetwork名前空間からは直接見えなくなります．

### veth

異なる2つのNetwork名前空間の間で通信するには，vethという仮想的なイーサネットデバイスを利用できます．1つのvethペアは2つのインターフェイスを持ち，片方ずつ別のNetwork名前空間に属させることで，パケットのやりとりができます．

### パケットフィルタリングとルーティング

Linuxには`netfilter`(`iptables`などを使って設定できます)というパケットフィルタリングのしくみや，カーネルのIPルーティング機能があります．これらはNetwork名前空間ごとに設定できます．

### ネットワーク名前空間の利用

**`clone`**システムコールに**`CLONE_NEWNET`**フラグを設定すると、新しいネットワーク名前空間でプロセスが開始されます。以下のコードは、新しいネットワーク名前空間を作成する例です。

```c
#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int child_func(void *arg) {
  // 子プロセスの処理
  return 0;
}

int main() {
  int flags = SIGCHLD | CLONE_NEWNET;
  size_t stack_size = 1024 * 1024;
  void *stack = malloc(stack_size);
  if (stack == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  
  int pid = clone(child_func, (char*)stack + stack_size, flags, NULL);
  if (pid == -1) {
    perror("clone");
    free(stack);
    exit(EXIT_FAILURE);
  }
  waitpid(pid, NULL, 0);
  free(stack);

  return 0;
}
```

このコードは、**`CLONE_NEWNET`**フラグを指定して**`clone`**を呼び出すことで、新しいネットワーク名前空間を作成します。**`child_func`**関数は子プロセスで実行される関数です。

### 利用方法

`clone`システムコールに`CLONE_NEWNET`フラグを設定すると，新しいNetwork名前空間でプロセスが開始されます．

```c
clone(child_func, (char*)stack + stack_size, SIGCHLD | CLONE_NEWNET, arg);
```

## 練習

カーネルのネットワーク機能の操作には本来`AF_NETLINK`ソケットというものを用いますが，これを扱うのは手間がかかるので，`ip`や`iptables`コマンドを利用します．これらのコマンドは内部的に`AF_NETLINK`ソケットを利用しています．

### Network名前空間の作成

まず，root権限を取得します．

```bash
$ sudo su -
```

ネットワークインターフェイス一覧は以下のコマンドで確認できます．初めにどのようなネットワークインターフェイスがあるか確認しておきます．

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
```

ここからNetwork名前空間を利用します．以下のコマンドで`ns1`という名前のNetwork名前空間を作成します．

```bash
$ ip netns add ns1
```

以下のコマンドで，新しく作ったNetwork名前空間でシェルを実行します．このシェルで実行したプログラムはすべて`ns1`という名前のNetwork名前空間で実行されます．

```bash
$ ip netns exec ns1 bash
```

名前空間`ns1`の中でネットワークインターフェイス一覧を再度確認してみます．

```bash
$ ip link show
```

以下のように表示され、**`lo`**というインターフェイス以外が見えなくなったことがわかります。この**`lo`**というインターフェイスはループバックと呼ばれ、そこに送られたパケットはそのインターフェイスに返ってきます。

```bash
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
```

完全なサンプルコードは [examples/02-exec-result/network-namespace](../../examples/02-exec-result/network-namespace/README.md) にあります。

---
