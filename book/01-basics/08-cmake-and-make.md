# CMakeとMakeの使い方

### 概要

CMakeとMakeは，予め設定した方法にしたがって，ソースコードから自動的にコンパイルやリンクなどを行い，実行ファイルなどを作成するためのツールです．今回使用するgccコンパイラは，CMakeやMakeを通じて利用します．

### Makefileの書式

makeを使用するには、makeに以下の3つの情報を与える必要があります。

- 入力ファイル (Input File)
- 出力ファイル (Output File)
- 処理方法 (Command)

この情報を記載したファイルをMakefileといいます。ファイル名はMakefile以外を使用することもできますが、makeコマンドはカレントディレクトリにMakefileがあると、明示的に指定しなくともそれを使用します。 Makefileの書式は次のとおりです。

[TAB]と記載してしているように、処理方法の前はスペースでなくタブ1つでなければなりません。

```bash
出力ファイル: 入力ファイル
[TAB]　処理方法
```

また、出力ファイルと入力ファイルは、より抽象的に、それぞれ、ターゲットと依存ファイルと呼ばれることもあります。

### CMakeLists.txt

`CMakeLists.txt`というファイルは，どのような方法でソースコードから実行ファイルを作成するかということをCMakeに設定するためのファイルです．ファイル名はほとんどの場合`CMakeLists.txt`にします．このファイルは上から順に読み込まれます． 以下はその一例です．

```bash
cmake_minimum_required(VERSION 3.16)
project(test_project)
set(CMAKE_C_STANDARD 11)

add_executable(hello main.c)
target_link_libraries(hello m)
```

`cmake_minimum_required`や`set`などがコマンド名で，括弧の中が引数です．それぞれのコマンドの意味は以下のようになります．

| コマンド名 | 意味 |
| --- | --- |
| `cmake_minimum_required` | cmakeの最低バージョンの指定 |
| `project` | プロジェクト名の設定 |
| `set` | 変数に値を設定する |
| `add_executable` | 実行ファイルの作成を指定する |
| `target_link_libraries` | ライブラリをリンクする |

`project`コマンドの引数はプロジェクト名(任意)です． `add_executable`コマンドの引数は第1引数がターゲット名，つまり実行ファイルの名前で，そのあとがソースファイルのパスです．ソースファイルは例えば以下のように複数指定することができます．パスは`CMakeLists.txt`のあるディレクトリからの相対パスまたは絶対パスです．

```bash
add_executable(calculator src/main.c src/parser.c)
```

`target_link_libraries`コマンドの引数は第1引数がターゲット名で，その後がライブラリの名前です．たとえば`pthread`ライブラリを追加でリンクしたい時は以下のように記述します．

```bash
target_link_libraries(hello m pthread)
```

### CMakeとMakeの実行方法

1. CMakeを実行して、Makefileを生成します。ビルドディレクトリを作成し、そのディレクトリ内でCMakeを実行します。

```bash
$ mkdir build
$ cd build
$ cmake [オプション] [CMakeLists.txtのあるディレクトリ]
```

ここで、オプションには、**`-G`**（ジェネレータ指定）や**`-DCMAKE_BUILD_TYPE`**（ビルドタイプ指定）などがあります。

1. Makeを実行して、プログラムをビルドします。CMakeが生成したMakefileがあるディレクトリで、以下のコマンドを実行します。

```bash
$ make
```

これにより、MakeはCMakeLists.txtに従って、ソースコードから実行ファイルを生成します。

### ビルドオプションの設定

CMakeでは、ビルドオプションを設定して、ビルドプロセスをカスタマイズすることができます。例えば、デバッグ情報を含んだ実行ファイルを生成したい場合、`CMakeLists.txt`内で以下のように設定します。

```bash
set(CMAKE_BUILD_TYPE Debug)
```

この設定を適用したい場合は、CMakeを再実行してMakefileを生成し直し、その後Makeを実行してビルドします。また、CMake実行時にコマンドラインからビルドオプションを指定することもできます。

```bash
$ cmake -DCMAKE_BUILD_TYPE=Debug [CMakeLists.txtのあるディレクトリ]
```

### ライブラリの追加

プロジェクトに外部ライブラリを追加する場合、CMakeでライブラリのパスやリンク方法を設定する必要があります。例として、**`libm`**ライブラリを追加する場合、CMakeLists.txtに以下のような設定を追加します。

```cmake
find_library(MATH_LIBRARY NAMES m)
target_link_libraries(hello ${MATH_LIBRARY})
```

**`find_library`**コマンドで、**`libm`**ライブラリを検索し、その結果を**`MATH_LIBRARY`**変数に格納します。そして、**`target_link_libraries`**コマンドで、**`hello`**実行ファイルに対して**`MATH_LIBRARY`**をリンクします。

### 練習

上で学んだ手法を実際に試してみます． まず，プロジェクトのためのディレクトリを作成し，移動します．

```bash
$ mkdir project_linux
$ cd project_linux
```

次に，ソースコードを保存するためのディレクトリを作成します．

```bash
$ mkdir src
```

nanoを利用して`main.c`というファイルを開きます．

```bash
$ nano src/main.c
```

以下のようにCソースを記述します．

```c
#include <stdio.h>
#include <math.h>

int main() {
        printf("Hello, World!\n");
        printf("sin(1) = %lf\n",sin(1));
        return 0;
}
```

画面表示は以下のようになります．

```text
  GNU nano 5.4                       src/main.c *                               
#include <stdio.h>
#include <math.h>

int main() {
        printf("Hello, World!\n");
        printf("sin(1) = %lf\n",sin(1));
        return 0;
}














^G Help      ^O Write Out ^W Where Is  ^K Cut       ^T Execute   ^C Location
^X Exit      ^R Read File ^\ Replace   ^U Paste     ^J Justify   ^_ Go To Line
```

`Ctrl`+`O`，続いて`Enter`を押下し，保存します．

```text
  GNU nano 5.4                       src/main.c *                               
#include <stdio.h>
#include <math.h>

int main() {
        printf("Hello, World!\n");
        printf("sin(1) = %lf\n",sin(1));
        return 0;
}













File Name to Write: src/main.c                                                  
^G Help             M-D DOS Format      M-A Append          M-B Backup File
^C Cancel           M-M Mac Format      M-P Prepend         ^T Browse
```

`Ctrl`+`X`を押下し，終了します． 続いて，nanoを用いて`CMakeLists.txt`を作成します．

```bash
$ nano CMakeLists.txt
```

以下のように入力します．

```cmake
cmake_minimum_required(VERSION 3.16)
project(example)
set(CMAKE_C_STANDARD 11)

add_executable(hello src/main.c)
target_link_libraries(hello m)
```

画面表示は以下のようになります．

```text
  GNU nano 5.4                     CMakeLists.txt *                             
cmake_minimum_required(VERSION 3.16)
project(example)
set(CMAKE_C_STANDARD 11)

add_executable(hello src/main.c)
target_link_libraries(hello m)















^G Help      ^O Write Out ^W Where Is  ^K Cut       ^T Execute   ^C Location
^X Exit      ^R Read File ^\ Replace   ^U Paste     ^J Justify   ^_ Go To Line
```

先ほどと同様にして，保存・終了してください． 次に，ビルド用のディレクトリを作成し，移動します．

```bash
$ mkdir build
$ cd build
```

CMakeを実行します．`CMakeLists.txt`は`build`が入っているディレクトリにあるので，`CMakeLists.txt`のあるディレクトリは`..`になります．

```bash
$ cmake ..
```

実行すると`build`ディレクトリの中に`Makefile`が生成されます．次にMakeを実行します．

```bash
$ make
```

すると，`hello`という名前の実行可能ファイルが生成されます．

```bash
$ ls
CMakeCache.txt  CMakeFiles  cmake_install.cmake  hello  Makefile
```

これを実行してみます．実行可能ファイルのパスを入力することで，そのファイルを実行することができます．

```bash
$ ./hello
Hello, World!
sin(1) = 0.841471
```

完全なサンプルコードは [examples/01-basics/cmake-hello](../../examples/01-basics/cmake-hello/README.md) にあります。

---
