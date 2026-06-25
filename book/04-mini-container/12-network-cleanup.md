# 親プロセスのネットワークを片付ける

コンテナが終了したら，親プロセスはホスト側に残したvethを削除します．vethペアは片方を削除すると，もう片方も削除されます．子プロセスが終了すればNetwork名前空間も消えるため，通常はホスト側の`mc-host0`を削除すれば十分です．

```c
static void cleanup_parent_network(void) {
    char* const delete_veth[] = {"ip", "link", "del", "mc-host0", NULL};
    run_program(delete_veth, true);
}
```

この削除は失敗しても致命的なエラーとして扱いません．すでに消えている可能性があるからです．そのため`allow_failure`を`true`にしています．

## 失敗時にも片付ける

ネットワーク設定は途中で失敗することがあります．たとえば，vethの作成には成功したが，IPアドレスの設定に失敗した，という状態です．この場合も，できるだけ作ったものを片付けます．

`start_container`では，失敗時と正常終了時の両方でcleanupへ進みます．

```c
fail:
    kill(child, SIGKILL);
    close(config->sync_pipe[1]);
    waitpid(child, NULL, 0);

cleanup:
    cleanup_nat(config->nat_if);
    if (config->use_network) {
        cleanup_parent_network();
    }
```

このような失敗時の処理は，学習用のコードでも省かない方がよいです．名前空間やネットワーク設定を扱うプログラムは，途中で失敗したときにホストへ状態を残しやすいからです．

## 手動で片付ける

実験中にプログラムを強制終了すると，`mc-host0`が残ることがあります．その場合は手動で削除できます．

```bash
$ sudo ip link del mc-host0
```

NATルールが残っていないかも確認します．

```bash
$ sudo iptables -t nat -S
$ sudo iptables -S FORWARD
```

ネットワーク実験では，前回の設定が残っているせいで次の実験結果が分かりにくくなることがあります．作成と削除を常にセットで考えます．
