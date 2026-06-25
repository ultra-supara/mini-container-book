# pipe

[pipeシステムコール](../../../book/02-exec-result/12-pipe.md) の完全版サンプルです。

`fork`で作った子プロセスから親プロセスへ、パイプで文字列を送ります。

```bash
cmake -S . -B build
cmake --build build
./build/pipe_example
```

