#!/bin/bash
set -e

WEBROOT="articles/webroot"
WORK="tmp/web"

# lang directory : config under articles/. The translated editions use their
# own editorial front/back matter (articles/editorial_{lang}_2026).
EDITIONS=(
  "ja:config-web-jp.yml"
  "en:config-web-en_2026.yml"
  "ko:config-web-ko_2026.yml"
  "zh-hans:config-web-zh-hans_2026.yml"
  "zh-hant:config-web-zh-hant_2026.yml"
)
# WEB_LANGS="ja en" builds a subset (default: all).
WEB_LANGS="${WEB_LANGS:-ja en ko zh-hans zh-hant}"

rm -rf "$WEBROOT"
mkdir -p "$WEBROOT"

for edition in "${EDITIONS[@]}"; do
  lang="${edition%%:*}"
  config="${edition#*:}"
  case " $WEB_LANGS " in *" $lang "*) ;; *) continue ;; esac
  echo "=== Building $lang ==="
  bundle exec ruby scripts/build-review-web.rb "$config" "$WORK/$lang"
  cp -R "$WORK/$lang/webroot" "$WEBROOT/$lang"
  # Part dividers: pages 2-5 of articles/images/divider/tuning-tips-2026-interior.pdf,
  # rendered with `pdftoppm -png -singlefile -f N -l N -scale-to-x 900 -scale-to-y -1`.
  cp -R articles/images/divider/web "$WEBROOT/$lang/divider"
  echo "=== Building search index ($lang) ==="
  node build-search-index.js "$WEBROOT/$lang"
done

# Create root redirect
cat > "$WEBROOT/index.html" << 'REDIRECT'
<!DOCTYPE html>
<html>
<head><meta http-equiv="refresh" content="0; url=ja/index.html" /></head>
<body><a href="ja/index.html">日本語版へ</a></body>
</html>
REDIRECT

echo "=== Done: $WEBROOT/ ==="
