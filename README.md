# Japanese flashcards generator

Cross-platform Qt 6 / C++20 desktop app that turns a CSV vocabulary list
into a print-ready PDF of double-sided flashcards.

## Features

- **CSV input** — two columns (front, back), UTF-8 with or without BOM,
  RFC 4180 quoting (commas, embedded newlines, escaped quotes).
- **HTML in cells** — `<br>` for hard line breaks and
  `<span class='furigana'>…</span>` for the small grey reading line above
  the translation. Other HTML tags are stripped.
- **3 × 6 grid on A4** — 18 cards per logical page, 10 mm page margins,
  3 mm cell padding. Dashed cut guides on every cell.
- **Auto-fit text** — main font shrinks from 14 pt down to 8 pt to keep
  the cell from overflowing; furigana scales proportionally (floor 7 pt).
- **Two physical pages per logical page** — front (left → right, top → bottom)
  and a column-mirrored back, ready for long-edge duplex printing.
- **In-app preview** — `QPdfView` with zoom (Ctrl + / Ctrl − / Ctrl 0)
  and page navigation (PgUp / PgDn).
- **Save PDF** — `Ctrl+S`, default name derived from the CSV file.
- **Headless mode** — `--generate <input.csv> [output.pdf]` runs without
  a display, useful for scripts and CI.
- **Operation log** — skipped CSV rows, card / page counts, errors all
  surface in a panel at the bottom of the window.

Print support (per-page filtering for non-duplex printers) is on the
roadmap; the toolbar buttons are present but disabled.

## Dependencies

- CMake ≥ 3.21
- **Ninja** — required by all build presets
  (Arch: `pacman -S ninja`, Debian: `apt install ninja-build`,
   Windows: `choco install ninja` or download from
   https://ninja-build.org/)
- A C++20 compiler (GCC 11+, Clang 14+, MSVC 2022)
- Qt ≥ 6.4 with the components
  `Core Gui Widgets Pdf PdfWidgets PrintSupport`
  (plus `Test` if you want to build the test suite)
- A CJK-capable font file, **required at build time** and embedded
  into the binary. Place either of these into `resources/fonts/` before
  running CMake:
  - `NotoSansJP-Regular.otf` (preferred)
  - `NotoSansJP-Regular.ttf`

  One-liner:
  ```bash
  curl -L -o resources/fonts/NotoSansJP-Regular.otf \
    https://github.com/notofonts/noto-cjk/raw/main/Sans/SubsetOTF/JP/NotoSansJP-Regular.otf
  ```

  CMake will refuse to configure if the file is missing. See
  [resources/fonts/README.md](resources/fonts/README.md) for download
  alternatives.

There are no third-party C/C++ dependencies beyond Qt itself.

At runtime the bundled font is loaded directly from the binary; an
optional `$FLASHCARDS_FONT_PATH` environment variable lets you override
it with any TTF / OTF on disk. If both are unavailable the resolver
falls back to a system CJK font.

## Build

### Linux

```bash
cmake --preset linux-release
cmake --build --preset linux-release
./build/linux-release/jp-cards
```

Available presets (all use the Ninja generator):

| Preset            | Configuration | Notes                       |
|-------------------|---------------|-----------------------------|
| `linux-release`   | Release       | Default                     |
| `linux-debug`     | Debug         | Enables `BUILD_TESTING=ON`  |
| `windows-release` | Release       | MSVC + Ninja on Windows     |

### Windows (not tested yet)

Run from a "x64 Native Tools Command Prompt for VS 2022" (or after invoking
`vcvarsall.bat x64`) so the Ninja generator can find `cl.exe`.

```cmd
set CMAKE_PREFIX_PATH=C:\Qt\6.x.y\msvc2022_64
cmake --preset windows-release
cmake --build --preset windows-release
build\windows-release\jp-cards.exe
```

For a step-by-step guide aimed at someone new to building C++/Qt on
Windows — installing Visual Studio Build Tools, Qt, configuring the
environment, fixing common errors — see [windows-build.md](windows-build.md).

## Tests

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug -j
ctest --test-dir build/linux-debug --output-on-failure
```

The suite exercises the CSV parser, the deck padding logic, end-to-end
PDF generation (rendered and re-parsed via `QPdfDocument`), and the
controller's CSV → PDF flow.

## Usage

### GUI

Launch the binary, click **Open CSV…**, pick your CSV, then **Save PDF…**
to write the result. Keyboard shortcuts:

| Action         | Shortcut    |
|----------------|-------------|
| Open CSV       | Ctrl + O    |
| Save PDF       | Ctrl + S    |
| Zoom in        | Ctrl + +    |
| Zoom out       | Ctrl + −    |
| Fit page       | Ctrl + 0    |
| Previous page  | Page Up     |
| Next page      | Page Down   |

### Headless

```bash
jp-cards --generate vocab.csv             # → vocab.pdf next to the CSV
jp-cards --generate vocab.csv out.pdf     # → custom output path
```

## CSV format

Two UTF-8 columns separated by commas. Either column may contain
the supported HTML subset:

```csv
宴会を開く<br><span class='furigana'>(えんかいをひらく)</span>,To throw a banquet
飲み会<br><span class='furigana'>(のみかい)</span>,Drinking party
"Field, with comma",Translation
"She said ""hi""",Translation
```

Empty rows are skipped silently. Rows with fewer than two columns are
skipped with a warning in the log.

## Project layout

```
src/
  main.cpp           — entry point + CLI parser
  ui/MainWindow      — toolbar, QPdfView, log pane
  app/
    FlashcardsController — orchestrates CSV → deck → PDF → preview
    PrintController      — drives QPrintDialog and page rasterisation
  domain/
    Card, CardDeck, CsvLoader
  render/
    FontProvider     — resolves a CJK font family
    PdfGenerator     — QPdfWriter + QPainter + QTextDocument
resources/
  fonts/             — bundled NotoSansJP-Regular (build-time, not in git)
tests/               — Qt Test
```
