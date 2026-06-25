# mini-container

このサンプルは、本文の「ミニコンテナを作ろう」に対応する完全版です。

Linux専用です。PID名前空間、Mount名前空間、UTS名前空間、必要に応じてUser名前空間とNetwork名前空間を作成し、指定したrootfsでコマンドを実行します。

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Usage

```bash
sudo ./build/mini-container [OPTIONS] ROOTFS COMMAND [ARGS...]
```

主なオプションは以下です。

- `--hostname NAME`: コンテナ内のホスト名を指定します。
- `--userns`: User名前空間を作成し、実行ユーザをコンテナ内のUID/GID 0へマッピングします。
- `--network`: vethペアを作成し、コンテナ側に`eth0`を設定します。
- `--nat-if IFACE`: `--network`に加えて、指定した外側インターフェイスへNATするiptablesルールを追加します。
- `--mount-proc`: `chroot`後に`/proc`へprocfsをマウントします。

最小の動作確認には、rootfsとしてホストの`/`を使い、`/bin/true`を実行できます。

```bash
sudo ./build/mini-container / /bin/true
```

ホスト側のファイルシステムをそのまま見せるので安全な隔離ではありませんが、`clone`、PID名前空間、Mount名前空間、UTS名前空間、親子同期、`chroot`、init処理が通ることを確認できます。

ネットワークも含めて試す場合は以下のように実行します。

```bash
sudo ./build/mini-container --network / /bin/true
```

NATまで試す場合は、外へ出るインターフェイスを確認して指定します。

```bash
ip route show default
sudo ./build/mini-container --network --nat-if ens4 / /bin/sh
```

このプログラムは学習用です。重要なホストでは実行せず、実験用のLinux仮想マシンで試してください。
