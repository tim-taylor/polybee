#!/usr/bin/env bash
# Build the PolyBee report from report.md into PDF and Word.
#
# Uses a pinned pandoc + pandoc-crossref in ./tools rather than the system
# pandoc: table-caption attributes (`{#tbl:id}`), which pandoc-crossref
# needs for @tbl:... refs, aren't parsed by the system pandoc (3.1.12.1)
# on this machine. tools/pandoc is 3.10.1, matching the pandoc-crossref
# build, so leave both at that version rather than bumping independently.
set -euo pipefail
cd "$(dirname "$0")"

export PATH="$PWD/tools:$PATH"

FILTERS=(--filter pandoc-crossref --citeproc)

pandoc report.md "${FILTERS[@]}" \
  --pdf-engine=pdflatex \
  --include-in-header=preamble.tex \
  -o report.pdf

pandoc report.md "${FILTERS[@]}" \
  --reference-doc=reference.docx \
  -o report.docx

echo "Built report.pdf and report.docx"
