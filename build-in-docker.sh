#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
review_language="${REVIEW_LANG:-jp}"
exec python3 "${repo_root}/scripts/build-review-pdf.py" --config "${REVIEW_CONFIG_FILE:-config-${review_language}.yml}" --output "${OUTPUT_PDF:-output/pdf/book-print-${review_language}.pdf}" "$@"
