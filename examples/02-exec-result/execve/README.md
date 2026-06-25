# execve

[execveシステムコール](../../../book/02-exec-result/10-execve.md) の完全版サンプルです。

現在のプロセスを`/usr/bin/id`に置き換えます。`execve`が成功すると、`execve`呼び出しより後のコードには戻りません。

```bash
cmake -S . -B build
cmake --build build
./build/execve_example
```

