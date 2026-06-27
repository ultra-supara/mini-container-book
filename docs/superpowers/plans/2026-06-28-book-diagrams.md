# Book Diagrams (Mermaid) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 本文Markdownに ```mermaid 図を大量に追加し、Typst→PDF経路では mermaid を SVG へ事前レンダリングして埋め込む（mmdc 不在時はフォールバック）。

**Architecture:** 図は ```mermaid フェンスとして `book/**.md` にインライン配置（単一ソース、GitHubはネイティブ描画）。`scripts/build_typst.py` を拡張し、```mermaid を検出→ソースのSHA-256で `dist/assets/<id>.svg` をキャッシュ参照→mmdcがあればレンダリングし `#image` を出力、無ければ ```mermaid コードブロックとしてフォールバックする。

**Tech Stack:** Python 3（標準ライブラリのみ：hashlib/os/shlex/shutil/subprocess/tempfile）、Mermaid CLI（mmdc, 任意）、Typst（SVGネイティブ埋め込み）。

参照spec: `docs/superpowers/specs/2026-06-28-book-diagrams-design.md`

---

## Part 1: ビルド基盤（TDD）

### Task 1: mermaid ブロック検出とフォールバック出力

**Files:**
- Modify: `scripts/build_typst.py`（imports、`convert_markdown` のフェンス処理、新関数 `mermaid_asset_id`/`render_mermaid_block`）
- Test: `tests/test_build_typst.py`

- [ ] **Step 1: 失敗するテストを書く**

`tests/test_build_typst.py` の `BuildTypstTests` クラスに追加する：

```python
    def test_convert_markdown_falls_back_when_no_renderer(self):
        markdown = "```mermaid\nflowchart TB\n  A --> B\n```\n"

        typst = build_typst.convert_markdown(markdown, Path("book/01.md"))

        self.assertIn("```mermaid", typst)
        self.assertIn("A --> B", typst)

    def test_convert_markdown_raises_on_unclosed_mermaid_block(self):
        with self.assertRaises(build_typst.BuildTypstError):
            build_typst.convert_markdown(
                "```mermaid\nflowchart TB\n  A --> B\n", Path("book/01.md")
            )

    def test_mermaid_asset_id_is_deterministic_and_12_chars(self):
        a = build_typst.mermaid_asset_id("flowchart TB\n  A --> B")
        b = build_typst.mermaid_asset_id("flowchart TB\n  A --> B\n")

        self.assertEqual(a, b)
        self.assertEqual(len(a), 12)
```

- [ ] **Step 2: テストが失敗することを確認**

Run: `python3 -m unittest tests.test_build_typst -v`
Expected: FAIL（`AttributeError: module 'scripts.build_typst' has no attribute 'mermaid_asset_id'`、および mermaid テストで code-block 化されず）

- [ ] **Step 3: 最小実装を書く**

`scripts/build_typst.py` の先頭 import を次へ置き換える（現状は `argparse, re, sys, pathlib.Path` のみ）：

```python
from __future__ import annotations

import argparse
import hashlib
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
```

`class BuildTypstError(Exception):` の直後に例外クラスを追加する：

```python
class MermaidRenderError(Exception):
    pass
```

`convert_markdown` の関数シグネチャと、コードフェンス分岐を次へ置き換える。現状：

```python
def convert_markdown(markdown: str, source_path: Path) -> str:
    output: list[str] = []
    lines = markdown.splitlines()
    in_code = False
    index = 0

    while index < len(lines):
        raw_line = lines[index]
        line = raw_line.rstrip()
        if line.startswith("```"):
            output.append(line)
            in_code = not in_code
            index += 1
            continue
```

置き換え後：

```python
def convert_markdown(
    markdown: str,
    source_path: Path,
    assets_dir: Path | None = None,
    renderer=None,
) -> str:
    output: list[str] = []
    lines = markdown.splitlines()
    in_code = False
    index = 0

    while index < len(lines):
        raw_line = lines[index]
        line = raw_line.rstrip()
        if line.startswith("```"):
            if not in_code and line[3:].strip() == "mermaid":
                block_lines: list[str] = []
                index += 1
                while index < len(lines) and not lines[index].startswith("```"):
                    block_lines.append(lines[index])
                    index += 1
                if index >= len(lines):
                    raise BuildTypstError(
                        f"unclosed mermaid block in {source_path}"
                    )
                index += 1  # closing fence を読み飛ばす
                source = "\n".join(item.rstrip() for item in block_lines)
                output.append(render_mermaid_block(source, assets_dir, renderer))
                continue
            output.append(line)
            in_code = not in_code
            index += 1
            continue
```

`convert_markdown` の定義より前（`escape_typst_text` の上あたり）に、新関数を追加する：

```python
def mermaid_asset_id(source: str) -> str:
    normalized = "\n".join(line.rstrip() for line in source.splitlines())
    digest = hashlib.sha256(normalized.encode("utf-8")).hexdigest()
    return digest[:12]


def render_mermaid_block(source: str, assets_dir: Path | None, renderer) -> str:
    asset_id = mermaid_asset_id(source)
    if renderer is not None and assets_dir is not None:
        svg_path = assets_dir / f"{asset_id}.svg"
        try:
            if not svg_path.exists():
                renderer(source, svg_path)
            return f'#align(center, image("assets/{asset_id}.svg", width: 90%))'
        except MermaidRenderError as exc:
            print(f"warning: mermaid render failed: {exc}", file=sys.stderr)
    return "```mermaid\n" + source + "\n```"
```

- [ ] **Step 4: テストが通ることを確認**

Run: `python3 -m unittest tests.test_build_typst -v`
Expected: PASS（既存テストも含め全件 OK）

- [ ] **Step 5: コミット**

```bash
git add scripts/build_typst.py tests/test_build_typst.py
git commit -m "feat(typst): detect mermaid blocks with hash-based fallback

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 2: 画像出力とレンダラ注入

**Files:**
- Modify: `scripts/build_typst.py`（`find_mermaid_renderer`、`build_document`/`write_typst_file` のレンダラ受け渡し）
- Test: `tests/test_build_typst.py`

- [ ] **Step 1: 失敗するテストを書く**

`tests/test_build_typst.py` に追加する：

```python
    def test_convert_markdown_emits_image_when_renderer_available(self):
        with tempfile.TemporaryDirectory() as tmp:
            assets = Path(tmp)

            def fake_renderer(source, out_path):
                out_path.write_text("<svg/>", encoding="utf-8")

            markdown = "Intro.\n\n```mermaid\nflowchart TB\n  A --> B\n```\n"

            typst = build_typst.convert_markdown(
                markdown, Path("book/01.md"), assets, fake_renderer
            )

            self.assertIn('image("assets/', typst)
            self.assertEqual(len(list(assets.glob("*.svg"))), 1)

    def test_write_typst_file_renders_diagrams_with_injected_renderer(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            book = root / "book"
            book.mkdir()
            (book / "README.md").write_text(
                "# mini containerを作って学ぶLinux\n\n"
                "## Files\n\n"
                "- [Chapter](chapter.md)\n",
                encoding="utf-8",
            )
            (book / "chapter.md").write_text(
                "# Chapter\n\n```mermaid\nflowchart TB\n  A --> B\n```\n",
                encoding="utf-8",
            )
            output = root / "dist" / "mini-container-book.typ"

            def fake_renderer(source, out_path):
                out_path.write_text("<svg/>", encoding="utf-8")

            build_typst.write_typst_file(root, output, renderer=fake_renderer)

            text = output.read_text(encoding="utf-8")
            self.assertIn('image("assets/', text)
            self.assertTrue((output.parent / "assets").exists())
```

- [ ] **Step 2: テストが失敗することを確認**

Run: `python3 -m unittest tests.test_build_typst -v`
Expected: FAIL（`write_typst_file` が `renderer` 引数を受け取らない / image 出力されない）

- [ ] **Step 3: 最小実装を書く**

`scripts/build_typst.py` に既定レンダラ検出関数を追加する（`render_mermaid_block` の下）：

```python
def find_mermaid_renderer():
    command = os.environ.get("MMDC")
    if command:
        argv = shlex.split(command)
    else:
        mmdc = shutil.which("mmdc")
        if not mmdc:
            return None
        argv = [mmdc]

    def render(source: str, out_path: Path) -> None:
        out_path.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(
            "w", suffix=".mmd", delete=False, encoding="utf-8"
        ) as handle:
            handle.write(source)
            tmp_path = Path(handle.name)
        try:
            result = subprocess.run(
                argv + ["-i", str(tmp_path), "-o", str(out_path), "-b", "transparent"],
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                raise MermaidRenderError(
                    f"mmdc failed for {out_path.name}: {result.stderr.strip()}"
                )
        finally:
            tmp_path.unlink(missing_ok=True)

    return render
```

`build_document` を次へ置き換える。現状：

```python
def build_document(root: Path) -> str:
    parts = [document_preamble()]
    for source in extract_book_files(root):
        if not source.exists():
            raise BuildTypstError(f"referenced Markdown file does not exist: {source}")
        parts.append(convert_markdown(source.read_text(encoding="utf-8"), source))
    return "\n\n".join(part.strip() for part in parts if part.strip()) + "\n"
```

置き換え後：

```python
def build_document(root: Path, assets_dir: Path | None = None, renderer=None) -> str:
    parts = [document_preamble()]
    for source in extract_book_files(root):
        if not source.exists():
            raise BuildTypstError(f"referenced Markdown file does not exist: {source}")
        parts.append(
            convert_markdown(
                source.read_text(encoding="utf-8"), source, assets_dir, renderer
            )
        )
    return "\n\n".join(part.strip() for part in parts if part.strip()) + "\n"
```

`write_typst_file` を次へ置き換える。現状：

```python
def write_typst_file(root: Path, output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(build_document(root), encoding="utf-8")
```

置き換え後（`_RENDERER_UNSET` センチネルは `write_typst_file` の直前に定義）：

```python
_RENDERER_UNSET = object()


def write_typst_file(root: Path, output: Path, renderer=_RENDERER_UNSET) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    assets_dir = output.parent / "assets"
    if renderer is _RENDERER_UNSET:
        renderer = find_mermaid_renderer()
    if renderer is not None:
        assets_dir.mkdir(parents=True, exist_ok=True)
    output.write_text(
        build_document(root, assets_dir, renderer), encoding="utf-8"
    )
```

- [ ] **Step 4: テストが通ることを確認**

Run: `python3 -m unittest tests.test_build_typst -v`
Expected: PASS（全件）

- [ ] **Step 5: コミット**

```bash
git add scripts/build_typst.py tests/test_build_typst.py
git commit -m "feat(typst): render mermaid to SVG via injectable renderer

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 3: Makefile に diagrams ターゲットを追加

**Files:**
- Modify: `Makefile`

- [ ] **Step 1: `.PHONY` 行と新ターゲットを追加**

`Makefile` 先頭の `.PHONY: typst pdf clean` を次へ置き換える：

```make
.PHONY: typst pdf diagrams clean
```

`clean:` ターゲットの直前に次を追加する：

```make
diagrams:
	@if [ -z "$$MMDC" ] && ! command -v mmdc >/dev/null 2>&1; then \
		echo "error: mmdc (mermaid-cli) is required for 'make diagrams'" >&2; \
		echo "install: npm install -g @mermaid-js/mermaid-cli  (or set MMDC=...)" >&2; \
		exit 1; \
	fi
	python3 scripts/build_typst.py --output $(TYPST_SOURCE)
```

（`typst`/`pdf` ターゲットは変更不要。`build_typst.py` が mmdc を自動検出して機会的にレンダリングする。）

- [ ] **Step 2: typst 生成が通ることを確認**

Run: `make typst && head -c 200 dist/mini-container-book.typ`
Expected: 成功。`dist/mini-container-book.typ` が生成される（この環境は mmdc 未導入のため図はフォールバックの ```mermaid ブロックになる）。

- [ ] **Step 3: diagrams ターゲットの失敗メッセージを確認（mmdc 未導入時）**

Run: `make diagrams; echo "exit=$?"`
Expected: mmdc 未導入なら `error: mmdc (mermaid-cli) is required...` を表示し `exit=1`。mmdc 導入済みなら typst を生成し `exit=0`。

- [ ] **Step 4: コミット**

```bash
git add Makefile
git commit -m "build: add 'make diagrams' target for mermaid rendering

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

## Part 2: 作図オーサリング

各オーサリングタスクは以下を守る：
- 図は該当ファイルの指定アンカー直後に「**太字キャプション1行 → 空行 → ```mermaid ブロック → 空行**」の形で挿入する。
- 挿入後に必ず `make typst`（成功）を実行し、未閉鎖等の構文崩れが無いことを確認してからコミットする。
- 既存本文は削除しない（Task 4 の overview と Task 8 の 08-network-namespace で既存 ```text を置換する2箇所のみ例外）。

### Task 4: 第2章 プロセス系（clone / execve / dup / pipe）

**Files:**
- Modify: `book/02-exec-result/09-clone.md`, `10-execve.md`, `11-dup.md`, `12-pipe.md`

- [ ] **Step 1: 09-clone.md にシーケンス図を追加**

`## 例` 末尾の説明段落（`...munmap`関数で解放しています。` で終わる行）と、その次の `完全なサンプルコードは [examples/02-exec-result/clone]...` 行の **間** に挿入する：

```markdown
**図: cloneで子プロセスを作り、終了を待つ流れ**

​```mermaid
sequenceDiagram
    participant P as 親プロセス
    participant K as カーネル
    participant C as 子プロセス
    P->>K: clone(child, stack, SIGCHLD, arg)
    K-->>C: child関数の実行を開始
    K-->>P: 子プロセスのPIDを返す
    C->>C: puts(message)
    C-->>K: return 0 で終了
    K-->>P: SIGCHLD
    P->>K: waitpid(child)
    K-->>P: 終了ステータス
​```
```

（注：上のコードブロック内 ```mermaid の行頭にある不可視文字（ゼロ幅空白）はこの計画書の表示用。実ファイルには通常の ```mermaid を書く。以降の図も同様。）

- [ ] **Step 2: 09-clone.md に構造図を追加**

`### ファイルディスクリプタ` の段落（`...利用できる状態です．` で終わる行）の直後に挿入する：

```markdown
**図: cloneによる親子プロセスと、開いているファイルの共有**

​```mermaid
flowchart TB
    P["親プロセス"] -->|clone| C["子プロセス（child関数）"]
    P -. fd継承 .-> F["開いているファイル<br/>オフセットを共有・closeは独立"]
    C -. fd継承 .-> F
​```
```

- [ ] **Step 3: 10-execve.md に Before/After 図を追加**

`### メモリ` の段落（`現在のプロセスのメモリは全て破棄され，新しいプロセスのものに置き換わります．`）の直後に挿入する：

```markdown
**図: execveはプロセスを保ったまま中身を置き換える（PIDは不変）**

​```mermaid
flowchart LR
    subgraph Before["execve前（PID=1234）"]
        A["メモリ: 呼び出し元プログラム"]
    end
    subgraph After["execve後（PID=1234のまま）"]
        B["メモリ: /usr/bin/id に置換<br/>fdは引き継ぎ"]
    end
    Before -->|"execve(path, argv, envp)"| After
​```
```

- [ ] **Step 4: 11-dup.md に fdテーブル図を追加**

`### 概要` の段落（`...新しいファイルディスクリプタを使ってそのファイルにアクセスできるようになります．`）の直後に挿入する：

```markdown
**図: dupは別のfd番号から同じ「開いているファイル」を指す**

​```mermaid
flowchart LR
    A["fd 1（標準出力）"] --> OFD["開いているファイル<br/>オフセット等を共有"]
    B["fd new（dupで複製）"] --> OFD
​```
```

- [ ] **Step 5: 12-pipe.md にデータフロー図を追加**

`### 動作` の段落（`...読み取り専用ファイルディスクリプタに送信されます。`）の直後に挿入する：

```markdown
**図: pipeは書き込み端から読み取り端へバッファ経由で流れる**

​```mermaid
flowchart LR
    W["子プロセス<br/>write(p[1])"] --> BUF["パイプのバッファ"]
    BUF --> R["親プロセス<br/>read(p[0])"]
​```
```

- [ ] **Step 6: typst を生成して検証**

Run: `make typst && python3 -m unittest tests.test_build_typst -v`
Expected: typst 生成成功・テスト全件 PASS。

- [ ] **Step 7: コミット**

```bash
git add book/02-exec-result/09-clone.md book/02-exec-result/10-execve.md book/02-exec-result/11-dup.md book/02-exec-result/12-pipe.md
git commit -m "docs: add process diagrams (clone/execve/dup/pipe)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 5: 第2章 名前空間（namespaces / pid / network-namespace）

**Files:**
- Modify: `book/02-exec-result/13-namespaces.md`, `14-pid-namespace.md`, `15-network-namespace.md`

- [ ] **Step 1: 13-namespaces.md に入れ子構造図を追加**

`### 概要` の段落（`...しくみもあります．`）の直後に挿入する：

```markdown
**図: 名前空間は資源の見え方をプロセスごとに分ける**

​```mermaid
flowchart TB
    subgraph NS1["名前空間A"]
        P1["プロセス群A<br/>独自のNW/PID/UID..."]
    end
    subgraph NS2["名前空間B"]
        P2["プロセス群B<br/>独自のNW/PID/UID..."]
    end
    K["共有カーネル"] --- NS1
    K --- NS2
​```
```

- [ ] **Step 2: 14-pid-namespace.md にPID見え方の図を追加**

末尾の段落（`...独立したPIDが割り当てられます．`）と、その次の `完全なサンプルコードは [examples/02-exec-result/pid-namespace]...` 行の **間** に挿入する：

```markdown
**図: 同じプロセスがホストとPID名前空間で別のPIDに見える**

​```mermaid
flowchart LR
    subgraph Host["ホストのPID名前空間"]
        H["子プロセス: PID=7790"]
    end
    subgraph NS["新しいPID名前空間"]
        N["同じプロセス: PID=1"]
    end
    H -. 同一プロセス .- N
​```
```

- [ ] **Step 3: 15-network-namespace.md にトポロジ図を追加**

`### veth` の段落（`...パケットのやりとりができます．`）の直後に挿入する：

```markdown
**図: 2つのNetwork名前空間をvethペアでつなぐ**

​```mermaid
flowchart LR
    subgraph NSA["Network名前空間A"]
        A["vethの片側"]
    end
    subgraph NSB["Network名前空間B"]
        B["vethのもう片側"]
    end
    A <-->|パケット| B
​```
```

- [ ] **Step 4: typst を生成して検証**

Run: `make typst`
Expected: 成功。

- [ ] **Step 5: コミット**

```bash
git add book/02-exec-result/13-namespaces.md book/02-exec-result/14-pid-namespace.md book/02-exec-result/15-network-namespace.md
git commit -m "docs: add namespace diagrams (namespaces/pid/netns)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 6: 第2章 vethネットワーク（veth / ping / packet-filtering / nat / cleanup）

**Files:**
- Modify: `book/02-exec-result/16-veth.md`, `17-ping.md`, `18-packet-filtering.md`, `19-nat.md`, `20-cleanup.md`

- [ ] **Step 1: 16-veth.md にveth作成の図を追加**

`veth1A`と`veth1B`の2つのインターフェイスが増えたことを説明する段落（`...相互にパケットをやりとりできます．`）の直後に挿入する：

```markdown
**図: vethペアを作った直後（両方ともホスト側に存在）**

​```mermaid
flowchart LR
    subgraph Host["ホスト"]
        A["veth1A"] <--> B["veth1B"]
    end
​```
```

- [ ] **Step 2: 16-veth.md にveth1B移動の図を追加**

`### 名前空間の作成とveth1Bの割り当て` 節で、移動後の確認結果を説明する段落（`...一覧から表示されなくなったことがわかります．`）の直後に挿入する：

```markdown
**図: veth1Bをns1へ移動するとホスト側から見えなくなる**

​```mermaid
flowchart LR
    subgraph Host["ホスト"]
        A["veth1A"]
    end
    subgraph NS1["ns1名前空間"]
        B["veth1B"]
    end
    A <-->|パケット| B
​```
```

- [ ] **Step 3: 16-veth.md に最終トポロジ図を追加**

ファイル末尾の最後の出力ブロック（`### veth1Bの設定` の `inet 10.0.0.2/24 ...` を含むコードブロック）の後ろにある終端 `---` 行の **直前** に挿入する：

```markdown
**図: IP割り当て後の構成（10.0.0.1 ⇄ 10.0.0.2）**

​```mermaid
flowchart LR
    subgraph Host["ホスト"]
        A["veth1A<br/>10.0.0.1/24"]
    end
    subgraph NS1["ns1名前空間"]
        B["veth1B<br/>10.0.0.2/24"]
    end
    A <-->|"10.0.0.0/24"| B
​```
```

- [ ] **Step 4: 17-ping.md にICMP往復図を追加**

最初の成功出力ブロック（`rtt min/avg/max/mdev = ...` で終わるコードブロック）の後ろ、`しかし，まだインターネットとは通信ができません．` の行の **直前** に挿入する：

```markdown
**図: ns1からveth1A（10.0.0.1）へのpingの往復**

​```mermaid
sequenceDiagram
    participant B as ns1: veth1B (10.0.0.2)
    participant A as ホスト: veth1A (10.0.0.1)
    B->>A: ICMP echo request
    A-->>B: ICMP echo reply
​```
```

- [ ] **Step 5: 18-packet-filtering.md にフォワーディング図を追加**

`### フォワーディングの有効化` の段落（`...rootシェルで実行するか`sysctl`を使います。`）の直後に挿入する：

```markdown
**図: ip_forwardを1にすると別インターフェイス間の転送が有効になる**

​```mermaid
flowchart LR
    IN["入力インターフェイス"] --> FWD{"ip_forward = 1 ?"}
    FWD -->|"1: 転送する"| OUT["出力インターフェイス"]
    FWD -->|"0: 破棄"| DROP["転送しない"]
​```
```

- [ ] **Step 6: 19-nat.md にMASQUERADE図を追加**

冒頭段落（`...環境に合わせて置き換えてください。`）の直後に挿入する：

```markdown
**図: MASQUERADEで送信元10.0.0.2をens4のアドレスへ書き換える**

​```mermaid
flowchart LR
    NS["ns1: 10.0.0.2"] -->|"src=10.0.0.2"| H["ホスト<br/>POSTROUTING MASQUERADE"]
    H -->|"src=ens4のIPへ書換"| NET["インターネット (8.8.8.8)"]
    NET -->|"戻りはconntrackで対応付け"| H
    H --> NS
​```
```

- [ ] **Step 7: 20-cleanup.md にBefore/After図を追加**

冒頭段落（`使用が終わった後は、...削除してください。`）の直後に挿入する：

```markdown
**図: クリーンアップで実験用リソースを消す**

​```mermaid
flowchart LR
    subgraph Before["後始末前"]
        A["ns1 / veth1A・veth1B<br/>iptables NAT・FORWARD<br/>ip_forward=1"]
    end
    subgraph After["後始末後"]
        B["元の状態へ復帰"]
    end
    Before -->|"netns del / link del / iptables -D / ip_forward=0"| After
​```
```

- [ ] **Step 8: typst を生成して検証**

Run: `make typst`
Expected: 成功。

- [ ] **Step 9: コミット**

```bash
git add book/02-exec-result/16-veth.md book/02-exec-result/17-ping.md book/02-exec-result/18-packet-filtering.md book/02-exec-result/19-nat.md book/02-exec-result/20-cleanup.md
git commit -m "docs: add veth/ping/nat network diagrams

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 7: 第4章 中核（overview / init / chroot / clone-and-init）

**Files:**
- Modify: `book/04-mini-container/01-overview.md`, `02-init.md`, `03-chroot.md`, `04-clone-and-init.md`

- [ ] **Step 1: 01-overview.md の既存ASCII図をmermaidシーケンスへ置換**

次の既存ブロック（```text 〜 ``` の全体）を探す：

````text
```text
parent process
  |
  | clone(CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | ...)
  v
child process in new namespaces
  |
  | wait until parent finishes uid/gid and network setup
  v
set hostname, configure network, chroot, run init
```
````

これを次へ置き換える：

```markdown
**図: 親が準備し、子が待ち、準備完了後に子が実行する**

​```mermaid
sequenceDiagram
    participant P as 親プロセス（ホスト側）
    participant C as 子プロセス（新名前空間）
    P->>C: clone(CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | ...)
    Note over C: sync_pipeのreadで待機
    P->>P: uid/gidマッピング・ネットワーク・NAT設定
    P->>C: notify（pipeへ1バイト）
    Note over C: sethostname → network → chroot/chdir → init
    C->>C: 指定コマンドを実行
​```
```

- [ ] **Step 2: 02-init.md にinitシーケンス図を追加**

冒頭の導入末尾段落（`...簡単な`init`を作ります．`）の直後、`## コマンドを子プロセスとして起動する` 見出しの **直前** に挿入する：

```markdown
**図: initがコマンドをfork/execし、終了コードを回収する**

​```mermaid
sequenceDiagram
    participant I as init（PID 1）
    participant G as 目的コマンド
    I->>G: fork + execvp(command)
    G->>G: 実行
    G-->>I: 終了（SIGCHLD）
    I->>I: wait で終了コード回収
    I->>I: waitpid(-1, WNOHANG) で孤児を回収
    Note over I: WIFEXITED→終了コード / WIFSIGNALED→128+signal
​```
```

- [ ] **Step 3: 03-chroot.md にBefore/After図を追加**

冒頭の段落（`...この2つはセットで考える方が安全です．`）の直後、最初のコードブロックの **直前** に挿入する：

```markdown
**図: chroot+chdirでプロセスの「/」を切り替える**

​```mermaid
flowchart LR
    subgraph Before["chroot前"]
        A["プロセスの / = ホストの /"]
    end
    subgraph After["chroot後 + chdir(/)"]
        B["プロセスの / = rootfs<br/>/bin/sh = rootfs/bin/sh"]
    end
    Before -->|"mount private → chroot(rootfs) → chdir(/)"| After
​```
```

- [ ] **Step 4: 04-clone-and-init.md に名前空間ツリー図を追加**

最初の節で、フラグ表の後ろの段落（`...そのあとで`--userns`や`--network`を足していきます．`）の直後に挿入する：

```markdown
**図: cloneで子だけが新しい名前空間に入る**

​```mermaid
flowchart TB
    subgraph Host["ホストの名前空間"]
        P["親プロセス"]
    end
    subgraph New["新しい名前空間（PID/Mount/UTS、任意でNET/USER）"]
        C["子プロセス container_child"]
    end
    P -->|"clone(flags, ...)"| C
​```
```

- [ ] **Step 5: 04-clone-and-init.md に子プロセス入口シーケンス図を追加**

`## 子プロセスの入口` 節の番号付きリスト（1.〜6.）の直後、`実装は次のようになります．` の行の **直前** に挿入する：

```markdown
**図: container_childの処理順**

​```mermaid
sequenceDiagram
    participant C as container_child
    Note over C: 親の準備完了を待つ（sync_pipe）
    C->>C: sethostname（UTS名前空間）
    C->>C: setup_child_network（--network時、chrootより前）
    C->>C: setup_rootfs（chroot + chdir）
    C->>C: mount_procfs（--mount-proc時）
    C->>C: run_init（コマンド実行）
​```
```

- [ ] **Step 6: typst を生成して検証**

Run: `make typst`
Expected: 成功（01-overview から ```text 図が消え mermaid に置換されている）。

- [ ] **Step 7: コミット**

```bash
git add book/04-mini-container/01-overview.md book/04-mini-container/02-init.md book/04-mini-container/03-chroot.md book/04-mini-container/04-clone-and-init.md
git commit -m "docs: add core container diagrams (overview/init/chroot/clone)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 8: 第4章 名前空間と同期（pid-user / proc-file-write / parent-child-sync）

**Files:**
- Modify: `book/04-mini-container/05-pid-user-namespace.md`, `06-proc-file-write.md`, `07-parent-child-sync.md`

- [ ] **Step 1: 05-pid-user-namespace.md にuid_map対応図を追加**

`## User名前空間を見る` 節の段落（`...コンテナ内のUID 0がホスト側の実UIDへ対応付けられています．`）の直後に挿入する：

```markdown
**図: uid_mapでコンテナ内UID 0をホストの実UIDへ対応付ける**

​```mermaid
flowchart LR
    subgraph Container["コンテナ内（User名前空間）"]
        A["UID 0（rootに見える）"]
    end
    subgraph Host["ホスト側"]
        B["UID 1000（一般ユーザー）"]
    end
    A -->|"uid_map: 0 1000 1"| B
​```
```

- [ ] **Step 2: 05-pid-user-namespace.md にPID見え方の図を追加**

`## PID名前空間を見る` 節の段落（`...PID名前空間は，プロセスIDの番号空間を分離します．`）の直後に挿入する：

```markdown
**図: コンテナ内のPID 1はホストでは別のPIDに見える**

​```mermaid
flowchart LR
    subgraph Host["ホストのPID名前空間"]
        H["mini-containerの子<br/>例: PID=4321"]
    end
    subgraph NS["コンテナのPID名前空間"]
        N["init: PID=1"]
    end
    H -. 同一プロセス .- N
​```
```

- [ ] **Step 3: 06-proc-file-write.md にprocfs書き込みフロー図を追加**

冒頭2段落目（`これは普通の設定ファイルのように見えますが，実体はprocfsが提供するカーネルとのインターフェイスです．`）の直後に挿入する：

```markdown
**図: /procへのwriteはカーネルへの設定書き込みになる**

​```mermaid
flowchart LR
    P["親プロセス"] -->|"open(O_WRONLY)"| F["/proc/&lt;pid&gt;/uid_map"]
    P -->|"write(\"0 1000 1\")"| F
    F --> K["カーネル: User名前空間のマッピングを設定"]
​```
```

- [ ] **Step 4: 07-parent-child-sync.md に同期シーケンス図を追加（★中核）**

冒頭の導入末尾段落（`...親子のタイミングをそろえます．`）の直後、`## パイプを作る` 見出しの **直前** に挿入する：

```markdown
**図: sync_pipeで子を待たせ、親の準備完了後に解放する**

​```mermaid
sequenceDiagram
    participant P as 親プロセス
    participant C as 子プロセス
    Note over P,C: pipe(sync_pipe) を作成
    C->>C: read(sync_pipe[0]) でブロック
    P->>P: setup_user_map（uid_map/gid_map）
    P->>P: setup_parent_network（veth作成・移動）
    P->>P: setup_nat（必要時）
    P->>C: write(sync_pipe[1], "1")
    C->>C: readが1バイト返り、処理を続行
​```
```

- [ ] **Step 5: typst を生成して検証**

Run: `make typst`
Expected: 成功。

- [ ] **Step 6: コミット**

```bash
git add book/04-mini-container/05-pid-user-namespace.md book/04-mini-container/06-proc-file-write.md book/04-mini-container/07-parent-child-sync.md
git commit -m "docs: add userns/procfs/sync diagrams

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 9: 第4章 ネットワーク（netns / parent / child / cleanup / run / iptables）

**Files:**
- Modify: `book/04-mini-container/08-network-namespace.md`, `10-parent-network.md`, `11-child-network.md`, `12-network-cleanup.md`, `13-run-network.md`, `14-iptables.md`

- [ ] **Step 1: 08-network-namespace.md にトポロジ図を追加**

既存 ```text ブロックの後ろにある説明段落（`...コンテナ側のデフォルトゲートウェイは`10.200.0.1`にします．`）の直後に挿入する（既存 ```text は残す）：

```markdown
**図: mc-host0 ⇄ eth0 のveth構成**

​```mermaid
flowchart LR
    subgraph Host["ホストのNetwork名前空間"]
        A["mc-host0<br/>10.200.0.1/24"]
    end
    subgraph Container["コンテナのNetwork名前空間"]
        B["eth0（旧mc-child0）<br/>10.200.0.2/24<br/>default via 10.200.0.1"]
    end
    A <-->|veth| B
​```
```

- [ ] **Step 2: 10-parent-network.md に親側手順シーケンス図を追加**

冒頭段落（`...コンテナ側へ渡す一時的な名前を`mc-child0`にします．`）の直後、`## vethを作って子へ渡す` 見出しの **直前** に挿入する：

```markdown
**図: 親プロセスのネットワーク準備（setup_parent_network）**

​```mermaid
sequenceDiagram
    participant P as 親プロセス（ホスト側）
    P->>P: ip link del mc-host0（前回分を掃除・失敗可）
    P->>P: ip link add mc-host0 type veth peer name mc-child0
    P->>P: ip link set mc-child0 netns <子のPID>
    Note over P: mc-child0はもうホスト側から見えない
    P->>P: ip address add 10.200.0.1/24 dev mc-host0
    P->>P: ip link set mc-host0 up
​```
```

- [ ] **Step 3: 11-child-network.md に子側手順シーケンス図を追加**

冒頭2段落目（`子プロセス側では，ループバックを有効にし，...IPアドレスとデフォルトルートを設定します．`）の直後、最初のコードブロックの **直前** に挿入する：

```markdown
**図: 子プロセスのネットワーク設定（setup_child_network）**

​```mermaid
sequenceDiagram
    participant C as 子プロセス（コンテナ側）
    C->>C: ip link set lo up
    C->>C: ip link set mc-child0 name eth0
    C->>C: ip address add 10.200.0.2/24 dev eth0
    C->>C: ip link set eth0 up
    C->>C: ip route add default via 10.200.0.1
​```
```

- [ ] **Step 4: 12-network-cleanup.md にBefore/After図を追加**

冒頭段落（`...通常はホスト側の`mc-host0`を削除すれば十分です．`）の直後、最初のコードブロックの **直前** に挿入する：

```markdown
**図: 終了時にホスト側vethを消すとペアごと片付く**

​```mermaid
flowchart LR
    subgraph Before["コンテナ終了時"]
        A["mc-host0（ホスト側に残存）<br/>eth0（子のnetns）"]
    end
    subgraph After["cleanup後"]
        B["ip link del mc-host0<br/>→ ペアのeth0も消える"]
    end
    Before --> After
​```
```

- [ ] **Step 5: 13-run-network.md に全体配線シーケンス図を追加**

冒頭段落（`...子プロセスをまだ待たせたまま，必要な設定を行います．`）の直後、最初のコードブロックの **直前** に挿入する：

```markdown
**図: start_containerでの準備順（最後にnotifyで子を解放）**

​```mermaid
sequenceDiagram
    participant P as 親プロセス
    participant C as 子プロセス（待機中）
    P->>P: setup_user_map（--userns時）
    P->>P: setup_parent_network（--network時）
    P->>P: setup_nat（--nat-if時）
    P->>C: notify_child_ready（pipeへ1バイト）
    C->>C: 待機解除 → ネットワーク/chroot/init
​```
```

- [ ] **Step 6: 14-iptables.md にパケットフロー図を追加**

`## プログラムからNATを設定する` 節の段落（`...戻りのパケットはconntrackにより対応付けられ，コンテナ側へ戻されます．`）の直後、`## 片付ける` 見出しの **直前** に挿入する：

```markdown
**図: コンテナ(10.200.0.2)から外部への往復とNAT**

​```mermaid
flowchart LR
    C["コンテナ eth0<br/>10.200.0.2"] -->|"default via 10.200.0.1"| H["ホスト mc-host0<br/>FORWARD許可 + ip_forward=1"]
    H -->|"POSTROUTING MASQUERADE<br/>src→eth0のIP"| NET["インターネット 8.8.8.8"]
    NET -->|"戻りはconntrackで対応付け"| H
    H --> C
​```
```

- [ ] **Step 7: typst を生成して検証**

Run: `make typst && python3 -m unittest discover -s tests -v`
Expected: typst 生成成功・テスト全件 PASS。

- [ ] **Step 8: コミット**

```bash
git add book/04-mini-container/08-network-namespace.md book/04-mini-container/10-parent-network.md book/04-mini-container/11-child-network.md book/04-mini-container/12-network-cleanup.md book/04-mini-container/13-run-network.md book/04-mini-container/14-iptables.md
git commit -m "docs: add container network diagrams (veth/nat/run)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

## 完了基準

- `python3 -m unittest discover -s tests` が全件 PASS。
- `make typst` が成功し、`dist/mini-container-book.typ` を生成する。
- Phase 1 の30図がすべて該当ファイルに ```mermaid として入っている（第2章15図、第4章15図）。
- mmdc 導入環境では `make diagrams` または `make pdf` で `dist/assets/*.svg` が生成され、`.typ` 内が `#image("assets/...")` になる（任意・環境依存）。
- typst CLI はこの環境に未導入のため、PDFコンパイル自体は spec 同様に未検証。

## 第2段階（本計画の対象外）

規約確立後、`book/01-basics/`（users/groups/files/processes/pipes/signals）と
`book/03-linux-security/`（chroot/仮想化とコンテナ）へ同じ方式で横展開する。別計画で扱う。
