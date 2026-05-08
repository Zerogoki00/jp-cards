# Bundled font

The build embeds a CJK-capable font into the binary. CMake will refuse
to configure until one of these files is present here:

- `NotoSansJP-Regular.otf` (preferred — upstream Noto distribution)
- `NotoSansJP-Regular.ttf` (Google Fonts download, also accepted)

The file is intentionally not committed to the repository.

## Quick download

```bash
curl -L -o resources/fonts/NotoSansJP-Regular.otf \
  https://github.com/notofonts/noto-cjk/raw/main/Sans/SubsetOTF/JP/NotoSansJP-Regular.otf
```

