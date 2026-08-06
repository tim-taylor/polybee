#!/usr/bin/env bash
# Build the PolyBee report from report.md into PDF, Word, and/or LaTeX source.
#
# Uses a pinned pandoc + pandoc-crossref in ./tools rather than the system
# pandoc: table-caption attributes (`{#tbl:id}`), which pandoc-crossref
# needs for @tbl:... refs, aren't parsed by the system pandoc (3.1.12.1)
# on this machine. tools/pandoc is 3.10.1, matching the pandoc-crossref
# build, so leave both at that version rather than bumping independently.
set -euo pipefail
cd "$(dirname "$0")"

# --pdf/-p, --docx/-d, --latex/-l: build only the given format(s). Short
# flags may be combined in any order in a single argument (e.g. -lpd, -ld,
# -p, -dlp). With no flags at all, build pdf + docx (the original default);
# --latex/-l is opt-in only and is never implied.
do_pdf=false
do_docx=false
do_latex=false

usage() {
  echo "Usage: $0 [--pdf] [--docx] [--latex] [-p] [-d] [-l] [-pd, -ld, -lpd, ... combined, any order]" >&2
}

for arg in "$@"; do
  case "$arg" in
    --pdf) do_pdf=true ;;
    --docx) do_docx=true ;;
    --latex) do_latex=true ;;
    -*)
      flag_chars="${arg#-}"
      if [[ -z "$flag_chars" || "$flag_chars" =~ [^lpd] ]]; then
        usage
        exit 1
      fi
      for ((i = 0; i < ${#flag_chars}; i++)); do
        case "${flag_chars:$i:1}" in
          p) do_pdf=true ;;
          d) do_docx=true ;;
          l) do_latex=true ;;
        esac
      done
      ;;
    *)
      usage
      exit 1
      ;;
  esac
done
if ! $do_pdf && ! $do_docx && ! $do_latex; then
  do_pdf=true
  do_docx=true
fi

export PATH="$PWD/tools:$PATH"

FILTERS=(--filter pandoc-crossref --citeproc)

built=()

if $do_pdf; then
  pandoc report.md "${FILTERS[@]}" \
    --pdf-engine=pdflatex \
    -o report.pdf
  built+=(report.pdf)
fi

if $do_docx; then
  pandoc report.md "${FILTERS[@]}" \
    --reference-doc=reference.docx \
    -o report.docx
  built+=(report.docx)
fi

if $do_latex; then
  pandoc report.md "${FILTERS[@]}" \
    --pdf-engine=pdflatex \
    --standalone \
    -o report.tex
  built+=(report.tex)
fi

echo "Built ${built[*]}"
