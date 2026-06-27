# 作図追加（Mermaid主軸）設計

## Goal

読者が「動いているもの」のイメージを持てるように、本文へ大量の作図を追加する。

第1段階（Phase 1）では、仕組みが動く中核章である `book/02-exec-result/` と
`book/04-mini-container/` に **30図以上** を入れる。第2段階で `book/01-basics/` と
`book/03-linux-security/` へ横展開する。

本書のMarkdownはGitHub等での直接閲覧とTypst→PDF生成の **両方** で読まれる。図はこの
両経路で成立させる。

## 図のフォーマット

図は ```mermaid フェンスドコードブロックとして本文Markdownに **インライン** で書く。
Markdownが唯一の原稿（single source）であり、別途の図ファイルは原稿に持たない。

- GitHub等のMarkdownビューアは ```mermaid をネイティブに描画する（追加コストゼロ）。
- PDF経路は `scripts/build_typst.py` を拡張し、mermaidをSVGに事前レンダリングして
  Typstの `image()` で埋め込む。Typstは **SVGをネイティブに埋め込める** ため、
  PNG変換は不要。

mermaidを採用する理由：テキストソースなので差分管理しやすく、AIで大量に生成でき、
シーケンス/状態遷移/フロー/構造といった「動き」を表す図に向く。

### 図の種類（題材に合わせて使い分ける）

| 種類 | mermaid記法 | 主な用途 |
| --- | --- | --- |
| シーケンス図 | `sequenceDiagram` | clone→wait→設定→close(pipe)→chroot→exec などの時間軸の流れ |
| 状態遷移 / Before-After | `stateDiagram-v2` / 2枚の `flowchart` | veth移動前後、chroot前後、uid_map適用前後 |
| トポロジ / パケットフロー | `flowchart` | host—veth—ns の接続、ping往復、NATでの送信元IP書き換え |
| 構造図（ツリー/入れ子） | `flowchart` / `graph` | PID名前空間のプロセスツリー、fdテーブル、名前空間の入れ子 |

### キャプション規約

各図の直前に **太字1行のキャプション** を置く。例：

```markdown
**図: cloneで子プロセスを新しい名前空間に作る**

​```mermaid
sequenceDiagram
  ...
​```
```

- キャプションは通常段落なので、GitHubでもPDFでも同一テキストで表示される。
- 図番号の自動採番は今回は導入しない（YAGNI）。必要になれば後段で追加する。

## Architecture

### `scripts/build_typst.py` の拡張

既存の変換挙動（見出し・段落・リスト・コードブロック・表・リンク・水平線）は **不変**。
mermaid以外の出力は完全後方互換とする。

`convert_markdown` のコードフェンス処理に分岐を追加する。フェンス言語が `mermaid` の
ときだけ専用処理に入る：

1. 閉じフェンス ```` ``` ```` までを読み取り、mermaidソース文字列を組み立てる。
   閉じられないまま終端に達した場合は `BuildTypstError` を送出する
   （既存の未閉鎖コードブロックと同じ扱い）。
2. ソースを正規化（各行の右側空白を除去し `\n` で連結）し、その SHA-256 の先頭12桁を
   アセットIDとする。出力ファイルは `dist/assets/<id>.svg`。
3. レンダラ（mmdc）が利用可能なら、`dist/assets/<id>.svg` が無いときだけ生成する
   （ハッシュによる内容キャッシュ。同一図は再生成しない）。生成済みなら、Typstへ
   中央寄せの画像を出力する：

   ```typst
   #align(center, image("assets/<id>.svg", width: 90%))
   ```

   `.typ` は `dist/` に、SVGは `dist/assets/` に出るため、相対パス `assets/<id>.svg`
   で解決できる。`width` は既定90%（過大表示を避ける調整可能な既定値）。
4. レンダラが無く、キャッシュも無い場合は **グレースフルフォールバック** する。
   mermaidソースを失わないよう、淡色のコードブロックとして出力する：

   ```typst
   #block(fill: luma(245), inset: 8pt, radius: 2pt, width: 100%)[
     #raw("<mermaidソース>", lang: "mermaid")
   ]
   ```

   これによりPDFビルドは **mmdc無しでも決して失敗しない**。図はキャプション
   （直前の太字段落）と原図ソースが残る。

#### レンダラの抽象化（テスト容易性）

mermaidレンダリングは注入可能なコールバックに分離する。

- `find_mermaid_renderer()` … `shutil.which("mmdc")`（環境変数 `MMDC` で上書き可）を
  探し、見つかれば「ソース文字列と出力パスを受け取りSVGを生成する」関数を返す。
  無ければ `None`。
- `render_mermaid_block(source, assets_dir, renderer)` … 上記の分岐3/4を担う純粋な
  関数。`renderer` を引数で受けることで、テストはスタブ（SVGファイルを作るだけの偽物 /
  `None`）を渡して画像出力・フォールバック双方を検証できる。
- 既定レンダラ実装は mmdc をサブプロセス起動する：
  `mmdc -i <tmp.mmd> -o dist/assets/<id>.svg -b transparent`。
  一時 `.mmd` はソースを書き出して渡す。

`build_document` / `convert_markdown` はレンダラを引数で受け取り、未指定なら
`find_mermaid_renderer()` を既定にする。

### Makefile

- `make typst` … 依存ゼロのまま。mmdcが無くてもフォールバックでTypstを生成できる。
- `make pdf` … 既存のtypst必須チェックは維持。mmdcがあれば図を自動SVG化して埋め込み、
  無ければフォールバックで継続。
- `make diagrams`（新規・任意）… mmdc（または `npx -p @mermaid-js/mermaid-cli mmdc`）で
  全 ```mermaid を `dist/assets/*.svg` に事前生成する。実体は `build_typst.py` を
  レンダラ有効で1回走らせるラッパ。

## Phase 1 作図インベントリ

下表は実装計画のチェックリスト。1ファイルに複数図が入るものもある（合計30図以上）。

### `book/02-exec-result/`

| ファイル | 図（種類） |
| --- | --- |
| 09-clone.md | fork/cloneで親子に分岐（シーケンス）＋プロセスツリー（構造） |
| 10-execve.md | execveでメモリイメージ置換・PID不変（Before/After） |
| 11-dup.md | fdテーブルがdup前後で同一ファイル記述を指す（構造Before/After） |
| 12-pipe.md | pipeのread/write端と親子間のデータの流れ（構造/フロー） |
| 13-namespaces.md | 名前空間がプロセスを包む入れ子（構造） |
| 14-pid-namespace.md | ns内PID1・ホストからは別PID（構造Before/After） |
| 15-network-namespace.md | network ns分離・loのみ見える（トポロジ） |
| 16-veth.md | veth作成→片端をnsへ移動→IP割当→疎通（トポロジ＋Before/After、複数図） |
| 17-ping.md | ICMP echo req/replyの往復経路（パケットフロー） |
| 18-packet-filtering.md | パケットがiptablesチェインを通る・DROP/ACCEPT（フロー） |
| 19-nat.md | MASQUERADEで送信元IP書き換え・内→外→戻り（パケットフロー） |
| 20-cleanup.md | 作ったリソースが消える（状態Before/After） |

### `book/04-mini-container/`

| ファイル | 図（種類） |
| --- | --- |
| 01-overview.md | 親準備→子待機→close(pipe)→chroot/exec（既存ASCIIをmermaidシーケンス化） |
| 02-init.md | initがコマンドをfork/exec・waitpidで終了コード回収（シーケンス） |
| 03-chroot.md | chroot+chdirでルート切替（Before/After） |
| 04-clone-and-init.md | cloneフラグ→子でinit実行（シーケンス） |
| 05-pid-user-namespace.md | uid_map適用とPID1（構造＋Before/After） |
| 06-proc-file-write.md | open→write で /proc の uid_map/gid_map へ書く（フロー） |
| 07-parent-child-sync.md | sync_pipeで待機/解放（シーケンス、★中核） |
| 08-network-namespace.md | コンテナのnetwork ns分離（トポロジ） |
| 10-parent-network.md | host側veth・IP・route・NAT準備（トポロジ） |
| 11-child-network.md | ns側veth・IP・default route（トポロジ） |
| 12-network-cleanup.md | veth/iptables撤去（状態Before/After） |
| 13-run-network.md | ネットワーク配線の全体手順（シーケンス） |
| 14-iptables.md | NAT/FORWARDチェインをパケットが通る（パケットフロー） |

第2段階（規約確立後）：`book/01-basics/`（users/groups/files/processes/pipes/signals）と
`book/03-linux-security/`（chroot/仮想化とコンテナ）へ横展開する。本仕様の対象外。

## Error Handling

変換器が非ゼロ終了するのは既存条件に加えて：

- ```mermaid ブロックが閉じられないまま終端に達した場合。

mmdcの不在・mmdcのレンダリング失敗は **致命的にしない**。フォールバック出力に切り替え、
ビルドを継続する（必要に応じて標準エラーへ警告を出す）。

## Testing

`tests/test_build_typst.py` に変換器のユニットテストを追加する：

- ```mermaid ブロック＋SVGを生成する偽レンダラ → 出力に `image("assets/<id>.svg"` を含む。
- ```mermaid ブロック＋レンダラ `None`（mmdc不在）→ 出力にmermaidソースを含む
  フォールバックブロックが出る。
- 同一mermaidソース → 同一アセットID（ハッシュ決定性）。
- 未閉鎖の ```mermaid ブロック → `BuildTypstError`。
- 既存テストが引き続き通る（後方互換）。

検証コマンド：

- `python3 -m unittest discover -s tests`
- `make typst`
- 可能なら `npx -p @mermaid-js/mermaid-cli mmdc` を用意して代表図のSVG生成まで確認する。

注：この環境には `typst` が未導入のため、PDFコンパイル自体は既存の
Typst出力設計と同様にここでは未検証（依存チェックまでの確認に留める）。
