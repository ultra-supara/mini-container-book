# e_sudo

[sudoを作ろう！](../../../book/03-linux-security/07-build-sudo.md) の完全版サンプルです。

setuid rootを使う危険な実験です。学習用の隔離されたLinux環境でのみ実行してください。実際の`sudo`の代替として使ってはいけません。

```bash
cmake -S . -B build
cmake --build build
sudo chown root:root build/e_sudo
sudo chmod 4755 build/e_sudo
sudo touch /etc/e_sudoers
sudo chown root:root /etc/e_sudoers
sudo chmod 0644 /etc/e_sudoers
printf '%s\n' "$USER" | sudo tee /etc/e_sudoers
./build/e_sudo id
```

後片付け:

```bash
sudo rm -f /etc/e_sudoers
sudo chmod 0755 build/e_sudo
```
