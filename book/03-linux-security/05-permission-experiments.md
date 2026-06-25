# パーミッションの管理を実験してみよう！

### 実験1 : 読み書き実行のパーミッションを設定してみる

以下のような内容のテキストファイルを`test.sh`として作ります.

```bash
#! /bin/bash
echo Hello!
```

初めに、パーミッションをすべて解除します.

```bash
chmod ugo-rxw test.sh
```

ファイルを読み書き実行しようとしてみましょう.

```bash
echo "echo Hello!!" >> test.sh
cat test.sh
./test.sh
```

ここで、所有者に読み込み権限を付与します.

```bash
chmod u+r test.sh
```

ファイルが読めるようになりますが、他の操作はまだできません.

```bash
echo "echo Hello!!" >> test.sh
cat test.sh
./test.sh
```

ここで、所有者に実行権限を付与します.

```bash
chmod u+x test.sh
```

実行もできるようになりますが、書き込みはまだできません.

```bash
echo "echo Hello!!" >> test.sh
cat test.sh
./test.sh
```

次に、所有者に書き込み権限を付与します.

```bash
chmod u+w test.sh
```

すべての操作が行えます.

```bash
echo "echo Hello!!" >> test.sh
cat test.sh
./test.sh
```

ファイルの所有者を変えてみます.

```bash
sudo chown root test.sh
```

操作は行えなくなりました.

```bash
echo "echo Hello!!" >> test.sh
cat test.sh
./test.sh
```

### 実験2 所有グループやその他のユーザーの権限

再び、パーミッションをすべて解除します .

```bash
sudo chmod ugo-rxw test.sh
```

今度はグループに読み込み権限を与えてみます.

```bash
sudo chmod g+r test.sh
```

ファイルの内容が読めるようになります.

```bash
cat test.sh
```

ファイルの所有グループを変えてみます.

```bash
sudo chown root:root test.sh
```

ファイルは読めなくなります.

```bash
cat test.sh
```

最後に、その他のユーザーに読み込み権限を与えてみます.

```bash
sudo chmod o+r test.sh
```

ファイルが再び読めるようになりました.

```bash
cat test.sh
```

---
