# NATでインターネットに接続する

`veth1A`を設定したターミナルで，以下のコマンドを入力します．このコマンドはNATという機能を利用するための設定です．この例では、**`ens4`**というインターフェイス名を使用していますが、実際の環境では異なる名前が使われることがあります。`ip route`などでインターネット側へ出ていくインターフェイス名を確認し，環境に合わせて置き換えてください。

```bash
$ iptables --table nat --append POSTROUTING --source 10.0.0.0/24 --out-interface ens4 --jump MASQUERADE
```

```bash
$ iptables --table filter --append FORWARD --source 10.0.0.0/24 --jump ACCEPT
```

```bash
$ iptables --table filter --append FORWARD --destination 10.0.0.0/24 --match conntrack --ctstate ESTABLISHED,RELATED --jump ACCEPT
```

次に，`veth1B`を設定したターミナルで，以下のコマンドを入力します．このコマンドはインターネットへのパケットをveth経由で送信するための設定です．

```bash
$ ip route add default via 10.0.0.1
```

`veth1B`を設定したターミナルで，名前空間からも`veth`を通してインターネットに接続できたことを確かめてみます．

```bash
$ ping 8.8.8.8 -c 4
```

以下のように表示されれば，正常に通信が行われています．

```bash
PING 8.8.8.8 (8.8.8.8) 56(84) bytes of data.
64 bytes from 8.8.8.8: icmp_seq=1 ttl=121 time=1.54 ms
64 bytes from 8.8.8.8: icmp_seq=2 ttl=121 time=1.50 ms
64 bytes from 8.8.8.8: icmp_seq=3 ttl=121 time=1.43 ms
64 bytes from 8.8.8.8: icmp_seq=4 ttl=121 time=1.34 ms

--- 8.8.8.8 ping statistics ---
4 packets transmitted, 4 received, 0% packet loss, time 3005ms
rtt min/avg/max/mdev = 1.340/1.453/1.544/0.076 ms
```
