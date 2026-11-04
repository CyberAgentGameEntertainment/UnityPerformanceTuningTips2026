#!/usr/bin/env python3
"""Build a book or selected manuscripts with the repository's Re:VIEW styles."""
import argparse
import json
import os
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def inside_repo(value):
    path = (ROOT / value).resolve()
    if not path.is_relative_to(ROOT):
        raise ValueError(f'path must be inside repository: {value}')
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sources', nargs='*', help='repository-relative .md or .re files; omit to use catalog.yml')
    parser.add_argument('--config', default=os.environ.get('REVIEW_CONFIG_FILE', 'config-epub-jp.yml'), help='configuration under articles/; config-epub-jp.yml is for screen and e-distribution, config-jp.yml is the print master (no hyperlinks)')
    parser.add_argument('--output', default='output/pdf/output.pdf', help='output/pdf/output.pdf (screen), output/pdf/output-print.pdf (print) and output/pdf/output-print-gray.pdf (monochrome print) are the fixed outputs and are overwritten; other paths must be new')
    parser.add_argument('--markdown-dir', help='build all Markdown in a directory in numeric order')
    parser.add_argument('--grayscale', action='store_true', help='convert the built PDF to grayscale with Ghostscript for monochrome printing')
    args = parser.parse_args()
    if args.grayscale and not shutil.which('gs'):
        parser.error('--grayscale requires Ghostscript (brew install ghostscript)')
    output = inside_repo(args.output)
    if output.suffix.lower() != '.pdf':
        parser.error('--output must end in .pdf')
    print_output = output == ROOT / 'output/pdf/output-print.pdf'
    if print_output and (args.config != 'config-jp.yml' or args.sources or args.markdown_dir or args.grayscale):
        parser.error('output/pdf/output-print.pdf is only for the whole print build: --config config-jp.yml without sources, --markdown-dir, or --grayscale')
    gray_output = output == ROOT / 'output/pdf/output-print-gray.pdf'
    if gray_output and (args.config != 'config-jp.yml' or args.sources or args.markdown_dir or not args.grayscale):
        parser.error('output/pdf/output-print-gray.pdf is only for the whole print build: --config config-jp.yml --grayscale without sources or --markdown-dir')
    fixed_output = output == ROOT / 'output/pdf/output.pdf' or print_output or gray_output
    if output.exists() and not fixed_output:
        parser.error(f'output already exists; choose a new filename: {output}')
    config = inside_repo('articles/' + args.config)
    if config.parent != ROOT / 'articles' or not config.is_file():
        parser.error('--config must name an existing articles/*.yml file')
    sources = args.sources
    if args.markdown_dir:
        if sources:
            parser.error('use sources or --markdown-dir, not both')
        directory = inside_repo(args.markdown_dir)
        sources = sorted(directory.glob('*.md'), key=lambda p: [int(x) if x.isdigit() else x for x in re.split(r'(\d+)', p.name)])
        if not sources:
            parser.error('no Markdown manuscripts found')
    source_paths = [inside_repo(s) for s in sources]
    if any(not p.is_file() or p.suffix not in ('.md', '.re') for p in source_paths):
        parser.error('sources must be existing .md or .re files')
    if len({p.stem for p in source_paths}) != len(source_paths):
        parser.error('selected manuscripts must have distinct filenames')
    if subprocess.run(['docker', 'image', 'inspect', 'review-custom:5.11'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode:
        subprocess.run(['docker', 'build', '-t', 'review-custom:5.11', str(ROOT)], check=True)
    scratch = ROOT / 'tmp' / 'pdfs'
    scratch.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='review-', dir=scratch))
    (work / 'request.json').write_text(json.dumps({
        'config': args.config,
        'sources': [str(p.relative_to(ROOT)) for p in source_paths] or None,
    }))
    print(f'Build workspace: {work}', flush=True)
    command = ['docker', 'run', '--rm', '--volume', f'{ROOT}:/book:ro',
               '--volume', f'{work}:/work',
               '--volume', 'unity-tuning-review-gems:/usr/local/bundle',
               '--volume', f'{ROOT}/articles/fonts/bizud:/usr/share/fonts/truetype/bizud:ro',
               '--volume', f'{ROOT}/articles/fonts/noto-cjk:/usr/share/fonts/opentype/noto-cjk:ro',
               '--workdir', '/book', '--env', 'BUNDLE_GEMFILE=/book/Gemfile',
               '--env', 'BUNDLE_FROZEN=true', '--env', 'REVIEW_FONT_FALLBACK=1', 'review-custom:5.11', 'sh', '-c',
               'set -e; bundle check || bundle install; bundle exec ruby /book/scripts/build-review-pdf.rb']

    with (work / 'build.log').open('w') as log:
        process = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
    if process.returncode:
        print((work / 'build.log').read_text()[-10000:])
        raise SystemExit(f'Build failed; see {work / "build.log"}')
    book = work / 'book.pdf'
    if args.grayscale:
        book = work / 'book-gray.pdf'
        subprocess.run(['gs', '-q', '-dSAFER', '-dBATCH', '-dNOPAUSE', '-sDEVICE=pdfwrite',
                        '-sColorConversionStrategy=Gray', '-dProcessColorModel=/DeviceGray',
                        # Keep screenshots lossless: pdfwrite re-encodes Flate images as JPEG by default.
                        '-dAutoFilterColorImages=false', '-dColorImageFilter=/FlateEncode',
                        '-dAutoFilterGrayImages=false', '-dGrayImageFilter=/FlateEncode',
                        '-o', str(book), str(work / 'book.pdf')], check=True)
    output.parent.mkdir(parents=True, exist_ok=True)
    with book.open('rb') as src, output.open('wb' if fixed_output else 'xb') as dst:
        shutil.copyfileobj(src, dst)
    print(f'Created {output}\nBuild log: {work / "build.log"}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
