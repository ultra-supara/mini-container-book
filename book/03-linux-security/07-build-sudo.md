# sudoを作ろう！

### sudoとは？

一般ユーザーから、別のユーザーの権限で一時的にプロセスを実行するためのコマンドです. ユーザーを指定しないとrootになります.

```bash
user@host:~$ sudo apt update #一般ユーザーでもaptを実行できる
user@host:~$ sudo vim /etc/default/grub #一般ユーザーでも通常は編集できないファイルを編集できる
user@host:~$ sudo kill <PID> #一般ユーザーでは操作できないプロセスにシグナルを送れる
```

ここで作るものは学習用の小さな`sudo`です。実際の`sudo`のようなパスワード確認、詳細なポリシー、ログ、安全な環境変数の扱いは実装しません。setuid rootのプログラムは危険なので、実験環境でのみ試してください。

### 仕組みは？

### SUIDビット

ファイルのパーミッションには**`SUID(Set owner User ID)`**ビットという属性があります. それを実行可能ファイルにセットしておくと、そのファイルが実行される時には自動的にそのプロセスの`euid`がそのファイルの所有ユーザーと同じになるというものです.

### 処理の流れ

1. ユーザーに`sudo`が実行される(自動的に`euid`が所有ユーザー(`root`)になる)
1. 設定ファイルを読んで、そのユーザーに`sudo`を使わせて良いのか確認する
1. 必要なUIDへ変更する
1. `execvp`ライブラリ関数を呼んで、指定されたコマンドを実行する

### 作るもの

GNUのsudoを見るとオプションがたくさんありますが、これを全部実装するのは大変なので機能を減らします.

- `/etc/e_sudoers`に書いてあるユーザーだけが使えます
- `e_sudo [-u user] command`として実行できます
- ユーザーが省略された場合は`root`

### ユーザー名からuidを取得する関数

ユーザー名から`uid`を取得するには、`getpwnam`ライブラリ関数を使います. この関数はNSSの設定に従って、`/etc/passwd`などからユーザー情報を返します.

```c
int username_to_uid(const char* username, uid_t* uid) {
    errno = 0;
    struct passwd* pwd = getpwnam(username);
    if (pwd == NULL) {
        if (errno != 0) {
            print_error("getpwnam");
        } else {
            printf("warn: no matching user for username %s\n", username);
        }
        return -1;
    }

    *uid = pwd->pw_uid;
    return 0;
}
```

### sudoersをチェックする関数

`sudoers`を1行ごとに読み込んで`uid`を取得して、実行したユーザー(=プロセス所有者(`ruid`))と同じものが存在するか調べます. `sudoers`が存在しない場合はエラーになります.

```c
int match_sudoers(uid_t uid) {
    FILE* f = fopen(SUDOERS_PATH, "r");
    if (f == NULL) {
        print_error("fopen");
        return -1;
    }

    char buffer[256];
    int ret = -1;

    while (fscanf(f, "%255s", buffer) == 1) {
        uid_t entry_uid;
        if (username_to_uid(buffer, &entry_uid) != 0) {
            continue;
        }
        if (entry_uid == uid) {
            ret = 0;
            break;
        }
    }

    if (ferror(f)) {
        print_error("fscanf");
        ret = -1;
    }

    fclose(f);
    return ret;
}
```

### main関数

まず、現在の`ruid`を取得して、そのユーザーが`sudoers`ファイルにあるかどうか調べます. また変更先のユーザー名が指定されている場合はそのユーザーの`uid`を調べます. そして`ruid`、`euid`、`suid`を変更し、`execvp`で指定のコマンドを実行します.

```c
int main(int argc, char** argv) {
    uid_t ruid, euid, suid;
    if (getresuid(&ruid, &euid, &suid) != 0) {
        print_error("getresuid");
        return -1;
    }

    if (match_sudoers(ruid) != 0) {
        printf("error: no matching entry found in sudoers file %s\n", SUDOERS_PATH);
        return -2;
    }

    uid_t target_uid = 0;
    int command_index = 1;

    if (argc >= 4 && strcmp(argv[1], "-u") == 0) {
        if (username_to_uid(argv[2], &target_uid) != 0) {
            printf("error: specified user not found: -u %s\n", argv[2]);
            return -3;
        }
        command_index = 3;
    } else if (argc < 2) {
        printf("error: no command specified\n");
        return -4;
    }

    if (setresuid(target_uid, target_uid, target_uid) != 0) {
        print_error("setresuid");
        return -5;
    }

    execvp(argv[command_index], &argv[command_index]);

    print_error("execvp");
    return -6;
}
```

### 全体

```c
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SUDOERS_PATH "/etc/e_sudoers"

void print_error(const char* message) {
    printf("error: %s: %s\n", message, strerror(errno));
}

int username_to_uid(const char* username, uid_t* uid) {
    errno = 0;
    struct passwd* pwd = getpwnam(username);
    if (pwd == NULL) {
        if (errno != 0) {
            print_error("getpwnam");
        } else {
            printf("warn: no matching user for username %s\n", username);
        }
        return -1;
    }

    *uid = pwd->pw_uid;
    return 0;
}

int match_sudoers(uid_t uid) {
    FILE* f = fopen(SUDOERS_PATH, "r");
    if (f == NULL) {
        print_error("fopen");
        return -1;
    }

    char buffer[256];
    int ret = -1;

    while (fscanf(f, "%255s", buffer) == 1) {
        uid_t entry_uid;
        if (username_to_uid(buffer, &entry_uid) != 0) {
            continue;
        }
        if (entry_uid == uid) {
            ret = 0;
            break;
        }
    }

    if (ferror(f)) {
        print_error("fscanf");
        ret = -1;
    }

    fclose(f);
    return ret;
}

int main(int argc, char** argv) {
    uid_t ruid, euid, suid;
    if (getresuid(&ruid, &euid, &suid) != 0) {
        print_error("getresuid");
        return -1;
    }

    if (match_sudoers(ruid) != 0) {
        printf("error: no matching entry found in sudoers file %s\n", SUDOERS_PATH);
        return -2;
    }

    uid_t target_uid = 0;
    int command_index = 1;

    if (argc >= 4 && strcmp(argv[1], "-u") == 0) {
        if (username_to_uid(argv[2], &target_uid) != 0) {
            printf("error: specified user not found: -u %s\n", argv[2]);
            return -3;
        }
        command_index = 3;
    } else if (argc < 2) {
        printf("error: no command specified\n");
        return -4;
    }

    if (setresuid(target_uid, target_uid, target_uid) != 0) {
        print_error("setresuid");
        return -5;
    }

    execvp(argv[command_index], &argv[command_index]);

    print_error("execvp");
    return -6;
}
```

### 実行！

```bash
gcc -Wall -std=gnu99 e_sudo.c -o e_sudo #コンパイルする
sudo chown root:root e_sudo #所有者をrootにする
sudo chmod 4755 e_sudo #suidビットをセットする
sudo touch /etc/e_sudoers #sudoersファイルを作る
sudo chown root:root /etc/e_sudoers
sudo chmod 0644 /etc/e_sudoers

./e_sudo id #sudoersにユーザー名が無いのでエラーになるはず
printf '%s\n' "$USER" | sudo tee /etc/e_sudoers #現在のユーザー名をsudoersに書く

./e_sudo id #rootとして実行されることを確認する
./e_sudo mkdir /hoge #rootなので通常は作れないディレクトリを作れる
./e_sudo ./test1 #uidがどうなっているか見てみよう
./e_sudo -u user ./test1 #userを実在するユーザー名に置き換え、任意のユーザーになれることを確認しよう
```

実験後、不要になったファイルは削除してください。

```bash
sudo rm -f /etc/e_sudoers
sudo rmdir /hoge
rm -f e_sudo
```

完全なサンプルコードは [examples/03-linux-security/e-sudo](../../examples/03-linux-security/e-sudo/README.md) にあります。

---
