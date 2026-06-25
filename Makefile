.PHONY: typst pdf clean

TYPST_SOURCE := dist/mini-container-book.typ
PDF_OUTPUT := dist/mini-container-book.pdf

typst:
	python3 scripts/build_typst.py --output $(TYPST_SOURCE)

pdf: typst
	@if ! command -v typst >/dev/null 2>&1; then \
		echo "error: typst CLI is required to build $(PDF_OUTPUT)" >&2; \
		echo "install it from https://typst.app/docs/" >&2; \
		exit 1; \
	fi
	typst compile $(TYPST_SOURCE) $(PDF_OUTPUT)

clean:
	rm -rf dist
