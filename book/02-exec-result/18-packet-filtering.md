# パケットフィルタリング設定

### フォワーディングの有効化

**`/proc/sys/net/ipv4/ip_forward`**を1に設定して、IPフォワーディングを有効化します。通常の`sudo echo 1 > ...`ではリダイレクト部分が一般ユーザーの権限で実行されて失敗するため、rootシェルで実行するか`sysctl`を使います。

**図: ip_forwardを1にすると別インターフェイス間の転送が有効になる**

```mermaid
flowchart LR
    IN["入力インターフェイス"] --> FWD{"ip_forward = 1 ?"}
    FWD -->|"1: 転送する"| OUT["出力インターフェイス"]
    FWD -->|"0: 破棄"| DROP["転送しない"]
```

```bash
$ sudo sh -c 'echo 1 > /proc/sys/net/ipv4/ip_forward'
```

`sysctl` コマンドを使用してIPフォワーディングを有効化することもできます。

```bash
$ sysctl -w net.ipv4.ip_forward=1
```
