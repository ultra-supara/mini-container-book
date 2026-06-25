# パケットフィルタリング設定

### フォワーディングの有効化

**`/proc/sys/net/ipv4/ip_forward`**を1に設定して、IPフォワーディングを有効化します。通常の`sudo echo 1 > ...`ではリダイレクト部分が一般ユーザーの権限で実行されて失敗するため、rootシェルで実行するか`sysctl`を使います。

```bash
$ sudo sh -c 'echo 1 > /proc/sys/net/ipv4/ip_forward'
```

`sysctl` コマンドを使用してIPフォワーディングを有効化することもできます。

```bash
$ sysctl -w net.ipv4.ip_forward=1
```
