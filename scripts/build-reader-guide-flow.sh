#!/bin/sh
# 課題別の逆引きガイドの流れ図を、TikZの元データからPDF（組版用）とPNG（Web・EPUB用）に書き出す。
# 日本語版は reader_guide_flow.tex、翻訳版は scripts/gen-reader-guide-flow-tex.py が生成する reader_guide_flow_<lang>.tex。
# 引数なし: 日本語版と全翻訳版。引数あり: 指定した名前（例: reader_guide_flow_ko）だけ。
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
FIG_DIR="$ROOT/articles/images_jp/editorial-reader-guide"
WORK="$ROOT/tmp/fig"
mkdir -p "$WORK"
python3 "$ROOT/scripts/gen-reader-guide-flow-tex.py"
if [ $# -eq 0 ]; then
  set -- reader_guide_flow reader_guide_flow_en reader_guide_flow_ko reader_guide_flow_zh-hans reader_guide_flow_zh-hant
fi
for NAME in "$@"; do
  cp "$FIG_DIR/$NAME.tex" "$WORK/"
  docker run --rm -v "$WORK:/work" \
    -v "$ROOT/articles/fonts/bizud:/usr/share/fonts/truetype/bizud:ro" \
    -v "$ROOT/articles/fonts/noto-cjk:/usr/share/fonts/opentype/noto-cjk:ro" \
    -w /work review-custom:5.11 sh -c \
    "uplatex -interaction=nonstopmode -halt-on-error $NAME.tex >/dev/null && dvipdfmx -q $NAME.dvi"
  cp "$WORK/$NAME.pdf" "$FIG_DIR/"
  pdftoppm -r 300 -png -singlefile "$WORK/$NAME.pdf" "$FIG_DIR/$NAME"
done
