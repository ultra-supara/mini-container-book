# clone

[cloneシステムコール](../../../book/02-exec-result/09-clone.md) の完全版サンプルです。

Linux専用です。子プロセスを作り、子プロセス側でメッセージを表示します。

```bash
cmake -S . -B build
cmake --build build
./build/clone_example
```

