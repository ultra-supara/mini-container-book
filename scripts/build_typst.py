#!/usr/bin/env python3
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
from collections.abc import Callable
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "dist" / "mini-container-book.typ"


class BuildTypstError(Exception):
    pass


class MermaidRenderError(Exception):
    pass


def extract_book_files(root: Path) -> list[Path]:
    readme = root / "book" / "README.md"
    try:
        lines = readme.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise BuildTypstError(f"failed to read {readme}: {exc}") from exc

    in_files = False
    files: list[Path] = []
    link_pattern = re.compile(r"^- \[[^\]]+\]\(([^)]+)\)")

    for line in lines:
        if line.startswith("## "):
            in_files = line.strip() == "## Files"
            continue
        if not in_files:
            continue
        match = link_pattern.match(line.strip())
        if not match:
            continue
        target = match.group(1)
        if target.startswith("../"):
            continue
        files.append(readme.parent / target)

    return files


def mermaid_asset_id(source: str) -> str:
    normalized = "\n".join(line.rstrip() for line in source.splitlines())
    digest = hashlib.sha256(normalized.encode("utf-8")).hexdigest()
    return digest[:12]


def render_mermaid_block(
    source: str,
    assets_dir: Path | None,
    renderer: Callable[[str, Path], None] | None,
) -> str:
    """Return Typst markup for a mermaid block.

    When ``renderer`` and ``assets_dir`` are given, the SVG is written to
    ``assets_dir / "<id>.svg"`` and an image reference ``assets/<id>.svg`` is
    emitted. This assumes the generated Typst file lives at ``assets_dir.parent``;
    the caller must guarantee that. ``renderer`` should raise on failure;
    rendering errors degrade gracefully to a raw mermaid fenced block so the
    build never fails.
    """
    asset_id = mermaid_asset_id(source)
    if renderer is not None and assets_dir is not None:
        svg_path = assets_dir / f"{asset_id}.svg"
        try:
            if not svg_path.exists():
                renderer(source, svg_path)
            return f'#align(center, image("assets/{asset_id}.svg", width: 90%))'
        except (MermaidRenderError, OSError, subprocess.SubprocessError) as exc:
            print(f"warning: mermaid render failed: {exc}", file=sys.stderr)
    return "```mermaid\n" + source + "\n```"


def find_mermaid_renderer() -> Callable[[str, Path], None] | None:
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


def convert_markdown(
    markdown: str,
    source_path: Path,
    assets_dir: Path | None = None,
    renderer: Callable[[str, Path], None] | None = None,
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
                index += 1  # skip closing fence
                source = "\n".join(item.rstrip() for item in block_lines)
                output.append(render_mermaid_block(source, assets_dir, renderer))
                continue
            output.append(line)
            in_code = not in_code
            index += 1
            continue
        if in_code:
            output.append(raw_line)
            index += 1
            continue
        if line.strip().startswith("|"):
            table, index = parse_table(lines, index, source_path)
            output.append(table)
            continue
        if line.startswith("#"):
            hashes, _, title = line.partition(" ")
            if hashes and set(hashes) == {"#"} and title:
                output.append(f"{'=' * len(hashes)} {convert_inline(title)}")
                index += 1
                continue
        if re.match(r"^\s*\d+\. ", line):
            prefix = re.match(r"^(\s*)\d+\. (.*)$", line)
            if prefix:
                output.append(f"{prefix.group(1)}+ {convert_inline(prefix.group(2))}")
                index += 1
                continue
        if re.match(r"^\s*- ", line):
            prefix = re.match(r"^(\s*)- (.*)$", line)
            if prefix:
                output.append(f"{prefix.group(1)}- {convert_inline(prefix.group(2))}")
                index += 1
                continue
        if line.strip() == "---":
            output.append("#v(0.5em)\n#line(length: 100%)\n#v(0.5em)")
            index += 1
            continue
        output.append(convert_inline(line))
        index += 1

    if in_code:
        raise BuildTypstError(f"unclosed code block in {source_path}")

    return "\n".join(output).strip() + "\n"


def escape_typst_text(text: str) -> str:
    replacements = {
        "\\": "\\\\",
        "#": "\\#",
        "$": "\\$",
        "*": "\\*",
        "_": "\\_",
        "(": "\\(",
        ")": "\\)",
        "<": "\\<",
        ">": "\\>",
    }
    return "".join(replacements.get(char, char) for char in text)


def escape_typst_string(text: str) -> str:
    return text.replace("\\", "\\\\").replace('"', '\\"')


def convert_inline(text: str) -> str:
    placeholders: list[str] = []

    def stash(value: str) -> str:
        placeholders.append(value)
        return f"@@TYPSTPH{len(placeholders) - 1}@@"

    text = re.sub(
        r"`([^`]+)`",
        lambda match: stash(f'#raw("{escape_typst_string(match.group(1))}")'),
        text,
    )
    text = re.sub(
        r"\[([^\]]+)\]\(([^)]+)\)",
        lambda match: stash(
            f'#link("{escape_typst_string(match.group(2))}")[{escape_typst_text(match.group(1))}]'
        ),
        text,
    )
    text = re.sub(
        r"<(https?://[^>]+)>",
        lambda match: stash(
            f'#link("{escape_typst_string(match.group(1))}")[{escape_typst_text(match.group(1))}]'
        ),
        text,
    )
    text = re.sub(
        r"\*\*([^*]+)\*\*",
        lambda match: stash(f"*{escape_typst_text(match.group(1))}*"),
        text,
    )

    text = escape_typst_text(text)
    for index in range(len(placeholders) - 1, -1, -1):
        text = text.replace(f"@@TYPSTPH{index}@@", placeholders[index])
    return text


def parse_table(lines: list[str], start: int, source_path: Path) -> tuple[str, int]:
    header = split_table_row(lines[start])
    separator = split_table_row(lines[start + 1]) if start + 1 < len(lines) else []
    if not header or not separator or not all(
        set(cell) <= {"-", ":"} and "-" in cell for cell in separator
    ):
        raise BuildTypstError(f"malformed table in {source_path}")

    rows = [header]
    index = start + 2
    while index < len(lines) and lines[index].strip().startswith("|"):
        rows.append(split_table_row(lines[index]))
        index += 1

    column_count = len(header)
    cells: list[str] = []
    for row in rows:
        padded = row + [""] * (column_count - len(row))
        cells.extend(f"[{convert_inline(cell.strip())}]" for cell in padded[:column_count])

    body = ",\n  ".join(cells)
    table = f"#table(\n  columns: {column_count},\n  inset: 6pt,\n  stroke: 0.5pt,\n  {body},\n)"
    return table, index


def split_table_row(line: str) -> list[str]:
    stripped = line.strip()
    if stripped.startswith("|"):
        stripped = stripped[1:]
    if stripped.endswith("|"):
        stripped = stripped[:-1]
    return [cell.strip() for cell in stripped.split("|")]


def document_preamble() -> str:
    return """#set document(title: [mini containerを作って学ぶLinux])
#set page(paper: "a4", margin: (x: 22mm, y: 24mm))
#set text(font: ("Hiragino Sans", "Apple SD Gothic Neo", "YuGothic"), lang: "ja", size: 10.5pt)
#set par(justify: true, leading: 0.7em)

#show heading.where(level: 1): it => {
  pagebreak(weak: true)
  set text(size: 18pt, weight: "bold")
  block(above: 1em, below: 0.8em, it)
}

#show heading.where(level: 2): it => {
  set text(size: 14pt, weight: "bold")
  block(above: 0.9em, below: 0.5em, it)
}

#show raw.where(block: true): block.with(
  fill: luma(245),
  inset: 8pt,
  radius: 2pt,
  width: 100%,
)
"""


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


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate Typst source for the book.")
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        write_typst_file(args.root.resolve(), args.output)
    except BuildTypstError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
