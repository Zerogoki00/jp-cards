# Japanese flashcards generator

Cross-platform Qt 6 / C++20 desktop app that turns a CSV vocabulary list
into a print-ready PDF of double-sided flashcards.

## Features

- **CSV input** — three columns (`word, furigana, translation`), UTF-8
  with or without BOM, RFC 4180 quoting (commas, embedded newlines,
  escaped quotes). The furigana column may be empty.
- **Card editor** (`Ctrl + E`) — modal dialog with an inline-editable
  table of all cards. Add, remove, reorder (Move Up / Move Down), and edit
  any field; on accept the preview re-renders against the new deck. Works
  with or without a CSV — start from scratch, or tweak an imported deck.
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
- Python 3 with `fonttools` available to CMake:
  `python -m pip install fonttools`

CMake downloads Noto Sans JP at configure time, generates a static
`NotoSansJP-Regular.ttf` from the upstream variable TTF, and embeds that
font into the binary. The font file is intentionally not stored in the
repository. See [resources/fonts/README.md](resources/fonts/README.md)
for details.

There are no third-party C/C++ dependencies beyond Qt itself.

At runtime the bundled font is loaded directly from the binary. The app
does not use external font paths or system-font fallback.

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

#### Arch Linux

A ready-to-use [`PKGBUILD`](packaging/arch/PKGBUILD) is provided. Build
and install with:

```bash
cd packaging/arch
makepkg -si
```


### Windows

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
| Card editor    | Ctrl + E    |
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

Three UTF-8 columns separated by commas: `word, furigana, translation`.
The furigana column may be left empty.

```csv
宴会を開く,えんかいをひらく,To throw a banquet
飲み会,のみかい,Drinking party
あふれる,,To overflow
ウィスキーの水割り,ウィスキーのみずわり,"Whisky, with water"
"She said ""hi""",,Translation
```

Empty rows are skipped silently. Rows with fewer than three columns are
skipped with a warning in the log.
