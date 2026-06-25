# File I/O

[ファイルについて](../../../book/02-exec-result/06-file-io.md) の完全版サンプルです。

`open`、`read`、`write`、`close`を使って`file.txt`へ書き込み、その内容を読み出します。

```bash
cmake -S . -B build
cmake --build build
./build/file_io
```

実行すると、カレントディレクトリに`file.txt`を作成または上書きします。

