# CMake hello

[CMakeとMakeの使い方](../../../book/01-basics/08-cmake-and-make.md) の完全版サンプルです。

```bash
cmake -S . -B build
cmake --build build
./build/hello
```

期待される出力例:

```text
Hello, World!
sin(1) = 0.841471
```

