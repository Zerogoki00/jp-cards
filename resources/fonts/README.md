# Bundled font

CMake downloads Noto Sans JP at configure time, generates a static
`NotoSansJP-Regular.ttf` from the upstream variable TTF with `fonttools`,
and embeds that generated file into the binary.

Downloaded source:
<https://github.com/notofonts/noto-cjk/raw/main/Sans/Variable/TTF/Subset/NotoSansJP-VF.ttf>
