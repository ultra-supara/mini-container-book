# Standard I/O file descriptors

[標準入出力について](../../../book/02-exec-result/07-stdio.md) の完全版サンプルです。

標準入力から1行読み取り、標準出力へ表示します。

```bash
cmake -S . -B build
cmake --build build
printf 'hello\n' | ./build/stdio_example
```

