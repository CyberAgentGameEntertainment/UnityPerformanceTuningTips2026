#!/usr/bin/env bash
# Compatibility entry point: PDF typesetting is always Re:VIEW.
set -euo pipefail
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -n "${MARKDOWN_DIR:-}" ]]; then set -- --markdown-dir "${MARKDOWN_DIR}" "$@"; fi
if [[ -n "${OUTPUT_PDF:-}" ]]; then set -- --output "${OUTPUT_PDF}" "$@"; fi
exec python3 "${script_dir}/build-review-pdf.py" "$@"
