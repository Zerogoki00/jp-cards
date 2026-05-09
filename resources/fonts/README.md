# Bundled font

The build embeds a CJK-capable font into the binary. CMake will refuse
to configure until this static TTF file is present here:

- `NotoSansJP-Regular.ttf`

The file is intentionally not committed to the repository.

Do not use the OTF build for release PDFs: on Qt-Windows builds it may be
converted to glyph outlines instead of an embedded font subset, which can
shift printer output.

Static family:
<https://fonts.google.com/noto/specimen/Noto+Sans+JP>
