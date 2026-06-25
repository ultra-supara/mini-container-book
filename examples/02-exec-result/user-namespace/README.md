# User namespace

[User名前空間](../../../book/02-exec-result/23-user-namespace.md) の完全版サンプルです。

Linux専用です。一般ユーザーによるUser名前空間作成が有効な環境ではroot権限なしで実行できます。無効化されている環境では`clone: Operation not permitted`のようなエラーになります。

```bash
cmake -S . -B build
cmake --build build
./build/user_namespace_example
```

マッピングを書き込まないため、子プロセス側ではUIDがoverflow IDとして見えることがあります。

