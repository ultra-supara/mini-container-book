# Examples

このディレクトリには、本文に登場するサンプルコードの完全版を置いています。

各サンプルは基本的に以下の形です。

```text
README.md
CMakeLists.txt
src/main.c
```

ビルド方法は各ディレクトリで共通です。

```bash
cmake -S . -B build
cmake --build build
```

名前空間、ネットワーク、setuidを扱う例はLinux専用です。root権限が必要なものやホスト環境に影響するものは、各サンプルのREADMEを確認してから実行してください。

