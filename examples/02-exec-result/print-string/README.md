# print_string

[実装していく項目](../../../book/02-exec-result/08-implementation-items.md) の完全版サンプルです。

`stdio.h`、`string.h`、`stdlib.h`を使わず、`write`システムコールだけで文字列と改行を標準出力に書き込みます。

```bash
cmake -S . -B build
cmake --build build
./build/print_string
```

