# pingによる疎通の確認

この状態で通信可能なことを確認します．`ns1`名前空間の外にある`veth1A`のアドレスである`10.0.0.1`と通信ができることを確認します．`veth1B`の設定を行ったターミナルで続けて以下のコマンドを実行します．

```bash
$ ping -c 3 10.0.0.1
```

以下のように表示されれば正常に通信が行われています．

```bash
PING 10.0.0.1 (10.0.0.1) 56(84) bytes of data.
64 bytes from 10.0.0.1: icmp_seq=1 ttl=64 time=0.035 ms
64 bytes from 10.0.0.1: icmp_seq=2 ttl=64 time=0.075 ms
64 bytes from 10.0.0.1: icmp_seq=3 ttl=64 time=0.066 ms

--- 10.0.0.1 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2047ms
rtt min/avg/max/mdev = 0.035/0.058/0.075/0.017 ms
```

しかし，まだインターネットとは通信ができません．以下のコマンドでインターネットのアドレスである`8.8.8.8`と通信を試みます．

```bash
$ ping -c 4 8.8.8.8
```

以下のようなエラーが表示されます．これは指定したアドレスに到達できないことを表します．

```bash
ping: connect: Network is unreachable
```

---
