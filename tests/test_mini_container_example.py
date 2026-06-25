import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EXAMPLE_DIR = ROOT / "examples" / "04-mini-container"
CHAPTER_DIR = ROOT / "book" / "04-mini-container"


EXPECTED_CHAPTER_FILES = [
    "01-overview.md",
    "02-init.md",
    "03-chroot.md",
    "04-clone-and-init.md",
    "05-pid-user-namespace.md",
    "06-proc-file-write.md",
    "07-parent-child-sync.md",
    "08-network-namespace.md",
    "09-run-program.md",
    "10-parent-network.md",
    "11-child-network.md",
    "12-network-cleanup.md",
    "13-run-network.md",
    "14-iptables.md",
    "15-rootless-container.md",
    "16-custom-linux-syscall.md",
]


class MiniContainerExampleTests(unittest.TestCase):
    def test_example_files_exist(self):
        for relative in ["README.md", "CMakeLists.txt", "src/main.c"]:
            with self.subTest(relative=relative):
                self.assertTrue((EXAMPLE_DIR / relative).is_file())

    def test_source_contains_container_building_blocks(self):
        source = (EXAMPLE_DIR / "src" / "main.c").read_text(encoding="utf-8")

        for token in [
            "CLONE_NEWPID",
            "CLONE_NEWNS",
            "CLONE_NEWUTS",
            "CLONE_NEWNET",
            "CLONE_NEWUSER",
            "chroot(",
            "sethostname(",
            "wait_parent_ready",
            "setup_user_map",
            "setup_parent_network",
            "setup_child_network",
            "iptables",
        ]:
            with self.subTest(token=token):
                self.assertIn(token, source)

    def test_check_examples_tracks_mini_container_as_linux_only(self):
        check_script = (ROOT / "scripts" / "check_examples.sh").read_text(encoding="utf-8")

        self.assertIn('"04-mini-container"', check_script)

    def test_book_links_to_complete_example(self):
        chapter = (CHAPTER_DIR / "01-overview.md").read_text(encoding="utf-8")

        self.assertIn(
            "[examples/04-mini-container](../../examples/04-mini-container/README.md)",
            chapter,
        )

    def test_mini_container_chapter_is_split_like_other_chapters(self):
        chapter_files = sorted(path.name for path in CHAPTER_DIR.glob("*.md"))
        self.assertEqual(EXPECTED_CHAPTER_FILES, chapter_files)

    def test_book_readme_includes_split_mini_container_chapters(self):
        readme = (ROOT / "book" / "README.md").read_text(encoding="utf-8")

        for filename in EXPECTED_CHAPTER_FILES:
            with self.subTest(filename=filename):
                self.assertIn(f"(04-mini-container/{filename})", readme)
