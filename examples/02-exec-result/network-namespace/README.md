# Network namespace

[network名前空間](../../../book/02-exec-result/15-network-namespace.md) の完全版サンプルです。

Linux専用です。多くの環境では新しいNetwork名前空間の作成にroot権限が必要です。このサンプルは新しいNetwork名前空間を作成し、その中で見えるネットワークインターフェイス一覧を表示します。

```bash
cmake -S . -B build
cmake --build build
sudo ./build/network_namespace_example
```

