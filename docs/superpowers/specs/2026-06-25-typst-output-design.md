# Typst Output Design

## Goal

Add a build path that produces both Typst source and a PDF for the book.

The book remains authored in Markdown under `book/`. Generated outputs are written under `dist/`:

- `dist/mini-container-book.typ`
- `dist/mini-container-book.pdf`

## Source Order

The converter uses `book/README.md` as the book index. It reads Markdown links listed under the `Files` section and converts those files in that order.

This keeps the output order aligned with the existing web-readable entry point and avoids maintaining a second chapter list.

## Conversion Scope

The first implementation supports the Markdown constructs already used in the current book:

- ATX headings
- paragraphs
- unordered lists
- ordered lists
- fenced code blocks with optional language names
- inline code
- bold text
- Markdown tables
- horizontal rules
- Markdown links

Images are not part of the current source set. If images are added later, the converter can be extended to copy assets and emit Typst image calls.

## Architecture

Add `scripts/build_typst.py`.

The script:

1. Parses the ordered file list from `book/README.md`.
2. Reads each Markdown file.
3. Converts supported Markdown constructs into Typst markup.
4. Writes a complete Typst document to `dist/mini-container-book.typ`.

The generated Typst document includes page setup, Japanese-capable font fallbacks, heading formatting, code block styling, and table rendering.

Add a root `Makefile`.

Targets:

- `make typst` generates `dist/mini-container-book.typ`.
- `make pdf` generates the Typst source and then runs `typst compile`.
- `make clean` removes `dist/`.

If `typst` is not installed, `make typst` still works and `make pdf` fails with a clear message.

## Error Handling

The converter exits non-zero when:

- `book/README.md` cannot be read.
- a referenced Markdown file does not exist.
- a fenced code block is left unclosed.
- a Markdown table is malformed enough that it cannot be rendered.

Unsupported inline Markdown is left as plain text rather than causing a failure, unless it would produce invalid Typst syntax.

## Testing

Add focused tests for the converter behavior:

- file order extraction from `book/README.md`
- heading, paragraph, list, code block, inline code, bold, link, and table conversion
- failure on missing referenced Markdown files
- successful generation of `dist/mini-container-book.typ`

Verification commands:

- `python3 -m unittest discover -s tests`
- `make typst`
- `make pdf` when Typst CLI is installed

Because the current environment does not have `typst`, PDF generation can only be verified up to the expected dependency check here.
