# ip6tablesを用いたファイアウォールの設定

セキュリティ対策として、ip6tablesを用いてファイアウォールの設定を行います。以下は、インターネットからの入力トラフィックを制限し、内部ネットワークへのフォワーディングを許可する例です。`ens4`や`ens5`は環境に合わせて置き換えてください。また、SSHなどでリモート接続している環境では、入力トラフィックをDROPする前に必要な許可ルールを追加してください。

```bash
$ ip6tables -A INPUT -i ens4 -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
$ ip6tables -A INPUT -i ens4 -p icmpv6 -j ACCEPT
$ ip6tables -A INPUT -i ens4 -j DROP

$ ip6tables -A FORWARD -i ens4 -o ens5 -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
$ ip6tables -A FORWARD -i ens5 -o ens4 -j ACCEPT
$ ip6tables -A FORWARD -j DROP
```

必要に応じて、これらのルールを **`/etc/ip6tables.rules`** などのファイルに保存し、システム起動時に適用するよう設定します。

必要に応じて、内部ネットワークのデバイスに対してIPv6 DNSサーバーを提供します。これにより、IPv6アドレスを使用した名前解決が可能になります。**`dnsmasq`** などのDNSサーバーソフトウェアを使用して設定できます。

IPv6でNATを使わない場合の設定は以上です。これにより、IPv6ネットワークを利用してインターネットにアクセスできるようになります。また、IPv6を活用することで、NATのような複雑なネットワーク構成を避けることができ、デバイス間の通信がより効率的になります。

---
