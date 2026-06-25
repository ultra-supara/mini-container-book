# PID namespace

[PID名前空間](../../../book/02-exec-result/14-pid-namespace.md) の完全版サンプルです。

Linux専用です。多くの環境では新しいPID名前空間の作成にroot権限が必要です。

```bash
cmake -S . -B build
cmake --build build
sudo ./build/pid_namespace_example
```

期待される出力例:

```text
[parent]PID=12345
[child]PID=1
```

