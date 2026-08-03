# PolyBee report

Source of truth is `report.md` (Pandoc Markdown + YAML metadata). Build
both a PDF (for yourself) and a Word doc (for handing to your boss) from
the same file:

```
./build.sh
```

## Layout

- `report.md` — the report. Sections stubbed out for intro/inspiration,
  design, experiments, results, conclusion.
- `references.bib` — BibTeX bibliography, cited with `[@key]`.
- `figures/` — put images here, reference with standard Markdown image
  syntax plus a `{#fig:label}` attribute for cross-referencing.
- `reference.docx` — Word style template. Pandoc uses its paragraph/
  heading/table styles when producing `report.docx`; edit this file's
  styles in Word (Home > Styles > Modify) to control what the boss-facing
  doc looks like — don't hand-format `report.md`.
- `tools/` — pinned `pandoc` (3.10.1) and `pandoc-crossref` (0.3.25)
  binaries, gitignored. See "Why pinned tools" below.

## Cross-referencing

Handled by `pandoc-crossref`:

- Figures: `![Caption](figures/x.png){#fig:x}`, referenced as `@fig:x`
- Tables: a caption line `: Caption {#tbl:x}` under a pipe table,
  referenced as `@tbl:x`
- Equations: `$$ ... $$ {#eq:x}`, referenced as `@eq:x`
- Sections: `@sec:label` if headings have `{#sec:label}`

## Why pinned tools

The system `pandoc` on this machine (3.1.12.1, from apt) doesn't parse
`{#tbl:id}` attributes on table captions into the AST, so
`pandoc-crossref` can't find table labels and `@tbl:...` refs silently
fail — figures and equations still work, tables don't. `tools/pandoc`
(3.10.1) does support this and matches the `pandoc-crossref` build, so
`build.sh` prepends `tools/` to `PATH` rather than relying on whatever
`pandoc` happens to be installed system-wide. If you regenerate
`tools/`, keep both binaries at matching versions.

To fetch them again on a new machine:

```
# pandoc
curl -sL -o /tmp/pandoc.tar.gz \
  https://github.com/jgm/pandoc/releases/download/3.10.1/pandoc-3.10.1-linux-amd64.tar.gz
tar xf /tmp/pandoc.tar.gz -C /tmp
cp /tmp/pandoc-3.10.1/bin/pandoc tools/

# pandoc-crossref
curl -sL -o /tmp/pcr.tar.xz \
  https://github.com/lierdakil/pandoc-crossref/releases/download/v0.3.25/pandoc-crossref-Linux-X64.tar.xz
tar xf /tmp/pcr.tar.xz -C /tmp
cp /tmp/pandoc-crossref tools/
chmod +x tools/pandoc tools/pandoc-crossref
```

## Citation style

Default is Pandoc's built-in Chicago author-date. To use a different
style (e.g. IEEE, APA), download a `.csl` file and add `csl: yourstyle.csl`
to the YAML header in `report.md`.
