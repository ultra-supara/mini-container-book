import tempfile
import unittest
from pathlib import Path

from scripts import build_typst


class BuildTypstTests(unittest.TestCase):
    def test_extract_book_files_reads_files_section_in_order(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            book = root / "book"
            book.mkdir()
            (book / "README.md").write_text(
                "# Title\n\n"
                "## Files\n\n"
                "- [First](01-first.md)\n"
                "- [Second](02-second.md)\n\n"
                "## Examples\n\n"
                "- [Example](../examples/README.md)\n",
                encoding="utf-8",
            )

            files = build_typst.extract_book_files(root)

            self.assertEqual(
                files,
                [book / "01-first.md", book / "02-second.md"],
            )

    def test_convert_markdown_handles_headings_lists_and_code_blocks(self):
        markdown = (
            "# Chapter\n\n"
            "Intro text.\n\n"
            "- first\n"
            "- second\n\n"
            "1. one\n"
            "1. two\n\n"
            "```c\n"
            "int main(void) { return 0; }\n"
            "```\n"
        )

        typst = build_typst.convert_markdown(markdown, Path("book/01.md"))

        self.assertIn("= Chapter", typst)
        self.assertIn("Intro text.", typst)
        self.assertIn("- first", typst)
        self.assertIn("+ one", typst)
        self.assertIn("```c", typst)
        self.assertIn("int main(void) { return 0; }", typst)

    def test_convert_markdown_handles_inline_markup_links_and_tables(self):
        markdown = (
            "Use `clone` and **namespaces**.\n\n"
            "See [examples](../../examples/README.md).\n\n"
            "| Name | Value |\n"
            "| --- | --- |\n"
            "| `PID` | Process ID |\n"
        )

        typst = build_typst.convert_markdown(markdown, Path("book/01.md"))

        self.assertIn('Use #raw("clone") and *namespaces*.', typst)
        self.assertIn('#link("../../examples/README.md")[examples]', typst)
        self.assertIn("#table(", typst)
        self.assertIn('[#raw("PID")]', typst)

    def test_convert_markdown_does_not_leave_placeholders_when_bold_wraps_code(self):
        typst = build_typst.convert_markdown(
            "Use **`ps`** and **`top`** commands.\n",
            Path("book/01.md"),
        )

        self.assertNotIn("\x00", typst)
        self.assertIn('*#raw("ps")*', typst)
        self.assertIn('*#raw("top")*', typst)

    def test_convert_markdown_converts_links_inside_bold_text(self):
        typst = build_typst.convert_markdown(
            "**Read [chapter](chapter.md) now**\n",
            Path("book/01.md"),
        )

        self.assertIn('*Read #link("chapter.md")[chapter] now*', typst)
        self.assertNotIn("[chapter](chapter.md)", typst)

    def test_convert_markdown_escapes_parentheses_after_inline_code(self):
        typst = build_typst.convert_markdown(
            "Linux has `netfilter`(`iptables` can configure it).\n",
            Path("book/01.md"),
        )

        self.assertIn('#raw("netfilter")\\(#raw("iptables")', typst)

    def test_convert_markdown_converts_angle_bracket_autolinks(self):
        typst = build_typst.convert_markdown(
            "- <https://example.com/doc.pdf>\n",
            Path("book/01.md"),
        )

        self.assertIn('#link("https://example.com/doc.pdf")[https://example.com/doc.pdf]', typst)
        self.assertNotIn("<https://example.com/doc.pdf>", typst)

    def test_build_typst_document_fails_on_missing_referenced_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            book = root / "book"
            book.mkdir()
            (book / "README.md").write_text(
                "# Title\n\n## Files\n\n- [Missing](missing.md)\n",
                encoding="utf-8",
            )

            with self.assertRaises(build_typst.BuildTypstError):
                build_typst.build_document(root)

    def test_write_typst_file_generates_complete_document(self):
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
            (book / "chapter.md").write_text("# Chapter\n\nBody\n", encoding="utf-8")
            output = root / "dist" / "mini-container-book.typ"

            build_typst.write_typst_file(root, output)

            text = output.read_text(encoding="utf-8")
            self.assertIn("#set document(title: [mini containerを作って学ぶLinux])", text)
            self.assertIn("= Chapter", text)

    def test_main_writes_output_to_requested_path(self):
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
            (book / "chapter.md").write_text("# Chapter\n", encoding="utf-8")
            output = root / "custom.typ"

            exit_code = build_typst.main(["--root", str(root), "--output", str(output)])

            self.assertEqual(exit_code, 0)
            self.assertTrue(output.exists())

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

    def test_render_mermaid_block_calls_renderer_and_emits_image(self):
        calls = []

        def fake_renderer(source, out_path):
            calls.append((source, out_path))
            out_path.write_text("<svg/>", encoding="utf-8")

        with tempfile.TemporaryDirectory() as tmp:
            result = build_typst.render_mermaid_block(
                "flowchart TB\n  A --> B", Path(tmp), fake_renderer
            )

        self.assertIn("#align(center, image(", result)
        self.assertEqual(len(calls), 1)

    def test_render_mermaid_block_falls_back_on_render_error(self):
        def bad_renderer(source, out_path):
            raise build_typst.MermaidRenderError("mmdc not found")

        with tempfile.TemporaryDirectory() as tmp:
            result = build_typst.render_mermaid_block(
                "flowchart TB\n  A --> B", Path(tmp), bad_renderer
            )

        self.assertIn("```mermaid", result)
        self.assertIn("A --> B", result)

    def test_render_mermaid_block_reuses_cached_svg_without_renderer(self):
        source = "flowchart TB\n  A --> B"
        with tempfile.TemporaryDirectory() as tmp:
            assets = Path(tmp)
            asset_id = build_typst.mermaid_asset_id(source)
            (assets / f"{asset_id}.svg").write_text("<svg/>", encoding="utf-8")

            result = build_typst.render_mermaid_block(source, assets, None)

        self.assertIn(f'image("assets/{asset_id}.svg"', result)

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
