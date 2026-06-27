# openとwriteでprocファイルへ書き込む

User名前空間を使う場合，`clone`で子プロセスを作ったあと，親プロセスは子プロセスに対してUID/GIDのマッピングを書き込みます．書き込む先は`/proc/<pid>/uid_map`と`/proc/<pid>/gid_map`です．

これは普通の設定ファイルのように見えますが，実体はprocfsが提供するカーネルとのインターフェイスです．ここへ文字列を書き込むことで，カーネルにUser名前空間のマッピングを伝えます．

**図: /procへのwriteはカーネルへの設定書き込みになる**

```mermaid
flowchart LR
    P["親プロセス"] -->|"open(O_WRONLY)"| F["/proc/&lt;pid&gt;/uid_map"]
    P -->|"write('0 1000 1')"| F
    F --> K["カーネル: User名前空間のマッピングを設定"]
```

## ファイルへ文字列を書き込む関数

まず，指定したファイルへ文字列を書き込む関数を作ります．ここでは，前に学んだ`open`と`write`をそのまま使います．

```c
static int write_file(const char* path, const char* data, bool required) {
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        if (required) {
            perror(path);
        }
        return -1;
    }

    size_t len = strlen(data);
    size_t offset = 0;
    while (offset < len) {
        ssize_t n = write(fd, data + offset, len - offset);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("write");
            close(fd);
            return -1;
        }
        offset += (size_t)n;
    }

    if (close(fd) != 0) {
        perror("close");
        return -1;
    }
    return 0;
}
```

`write`は，指定したバイト数を必ず一度で全部書くとは限りません．そのため，`offset`を進めながら，全部書き終わるまで繰り返します．このような小さな関数でも，システムコールを直接扱うときの基本が出てきます．

`required`は，失敗時にエラーを表示するかどうかを決める引数です．たとえば`setgroups`は環境によって存在しないこともあるため，必須ではない書き込みとして扱えるようにしています．

## UID/GIDマッピングを書く

UID/GIDマッピングは次のように書きます．

```c
static int setup_user_map(pid_t pid, uid_t host_uid, gid_t host_gid) {
    char path[256];
    char map[256];

    snprintf(path, sizeof(path), "/proc/%d/setgroups", pid);
    write_file(path, "deny\n", false);

    snprintf(path, sizeof(path), "/proc/%d/uid_map", pid);
    snprintf(map, sizeof(map), "0 %d 1\n", (int)host_uid);
    if (write_file(path, map, true) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "/proc/%d/gid_map", pid);
    snprintf(map, sizeof(map), "0 %d 1\n", (int)host_gid);
    if (write_file(path, map, true) != 0) {
        return -1;
    }

    return 0;
}
```

`0 1000 1`のような行は，「新しいUser名前空間のUID 0を，親側のUser名前空間のUID 1000へ，1個分対応付ける」という意味です．つまり，コンテナ内ではrootに見えても，ホスト側では元のユーザーとして扱われます．

`setgroups`へ`deny`を書く処理は，`gid_map`を書き込む前に必要になることがあります．ディストリビューションやカーネル設定によって挙動は変わるため，User名前空間まわりで失敗した場合は，`/proc/sys/kernel/unprivileged_userns_clone`やsubuid/subgidの設定も確認します．

## procfsはAPIとして読める

`/proc`配下のファイルは，普通のファイルに似た操作で扱えます．ただし，背後にいるのはカーネルです．`open`，`read`，`write`を通して，カーネル内部の状態を読んだり，設定を変更したりします．

この章ではUser名前空間のマッピングに使いましたが，同じ考え方は後で出てくる`/proc/sys/net/ipv4/ip_forward`にも使います．そこへ`1`を書き込むことで，IPv4フォワーディングを有効にします．
