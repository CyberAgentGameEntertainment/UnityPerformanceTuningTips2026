#!/bin/sh
# Compare the tool versions inside review-custom:5.11 with docker/tool-versions.txt.
# Usage: scripts/check-docker-tools.sh [--write]   (--write refreshes the record)
set -eu
cd "$(dirname "$0")/.."
IMAGE=review-custom:5.11
report() {
  docker run --rm "$IMAGE" sh -c '
    ruby -v | cut -d" " -f1-2
    echo "bundler $(bundler -v)"
    pandoc -v | head -1
    lualatex --version | head -1
    cat /etc/debian_version | sed "s/^/debian /"
    dpkg-query -W -f="\${Package} \${Version}\n" texlive-luatex texlive-lang-japanese texlive-lang-cjk \
      texlive-fonts-extra texlive-latex-extra ghostscript nodejs pandoc'
}
if [ "${1:-}" = "--write" ]; then
  report > docker/tool-versions.txt
  echo "wrote docker/tool-versions.txt"
else
  report | diff -u docker/tool-versions.txt - && echo "tool versions match docker/tool-versions.txt"
fi
