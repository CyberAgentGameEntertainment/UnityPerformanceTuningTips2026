#!/usr/bin/env python3
"""Normalize a Google Docs manuscript, then convert it with pandoc2review."""
import argparse
import importlib.util
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

SPEC = importlib.util.spec_from_file_location(
    'prepare_book', Path(__file__).with_name('prepare-google-drive-markdown-book.py'))
PREPARE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = PREPARE
SPEC.loader.exec_module(PREPARE)
sys.path.insert(0, str(Path(__file__).resolve().parent))
import author_profile
CAPTION = re.compile(r'^\[\]\{#([^} ]+)(?: tsize="([\d.,]+)")?\}\*\*(?:表|コード) \d+\.\d+: (.*)\*\*$')
# A line holding only {{pagebreak}} starts the following content on a new page.
PAGEBREAK = '{{pagebreak}}'
PAGEBREAK_MARKER = 'REVIEWPAGEBREAKMARKER'
# A line holding only {{enlargepage}} lets the current page take one more line,
# for a page whose last line would otherwise spill onto the next one.
ENLARGEPAGE = '{{enlargepage}}'
ENLARGEPAGE_MARKER = 'REVIEWENLARGEPAGEMARKER'
FIGURE = re.compile(r'^!\[.*\]\(([^)]+)\)\{#([^\s}]+)[^}]*\}$')
# Re:VIEW cannot nest inline commands, so pandoc2review's escaped form of a
# nested pair prints the inner command verbatim. Both decorations have to be
# carried by a single command instead.
NESTED_INLINE = re.compile(r'@<([a-z]+)>\$((?:(?!\$).)*@<(?:(?!\$).)*)\$')
INNER_INLINE = re.compile(r'@<([a-z]+)>\{([^{}]*)\}')
COMBINED_INLINE = {('b', 'tt'): 'ttb', ('i', 'tt'): 'tti'}
VERBATIM_BLOCKS = frozenset(
    ('list', 'listnum', 'emlist', 'emlistnum', 'source', 'cmd', 'embed', 'texequation'))
# media=print builds hyperref with the draft option, so a link whose URL is not
# typeset cannot be followed on paper. @<href> with a single argument is also the
# only form that lets TeX break a long URL across lines.
LABELED_LINK = re.compile(r'@<href>\{([^,}]+),((?:[^}\\]|\\.)*)\}')
BARE_URL = re.compile(
    r"(@<href>\{(?:[^}\\]|\\.)*\})|<?(https?://[-A-Za-z0-9._~:/?#@!$&'*+;=%]+)>?")


def balanced(text, index):
    """Return the contents of the bracket pair opening at index, and its end."""
    expected, position = [], index
    while position < len(text):
        character = text[position]
        if character == '\\':
            position += 2
            continue
        if character in '([':
            expected.append(')' if character == '(' else ']')
        elif expected and character == expected[-1]:
            expected.pop()
            if not expected:
                return text[index + 1:position], position + 1
        position += 1
    return None, index


def link_target(target):
    """Reduce a Google Docs autolinked target to the URL and the text after it."""
    autolink = re.match(r'^\[([^\]]*)\]\(.*\)\s*$', target)
    if autolink:
        target = autolink[1]
    url, _, trailing = target.partition(')')
    return url, trailing


def unescape_links(line):
    """Restore hand-written Markdown links that the Google Docs export escaped."""
    result, position = [], 0
    while True:
        start = line.find('\\[', position)
        if start < 0:
            break
        label_end = line.find('\\]', start + 2)
        target, after = (None, 0)
        if label_end > 0 and line.startswith('(', label_end + 2):
            target, after = balanced(line, label_end + 2)
            rest = line[label_end + 3:]
            # The export sometimes autolinks past the closing parenthesis, which
            # leaves the escaped link without one.
            if target is None and re.match(r'^\[[^\]]*\]\(.*\)\s*$', rest):
                target, after = rest, len(line)
        if target is None:
            result.append(line[position:start + 2])
            position = start + 2
            continue
        # @<href> cannot carry another inline command, so the label stays plain.
        label = line[start + 2:label_end].replace('\\`', '').replace('`', '')
        url, trailing = link_target(target)
        result.append(line[position:start])
        result.append(f'[{label}]({url}){trailing}')
        position = after
    result.append(line[position:])
    return ''.join(result)


def prepare(source, profile=None):
    markdown, _ = PREPARE.transform_document(source, drop_author_block=profile is not None)
    refs = {}
    lines = markdown.splitlines()
    for line in lines:
        figure = FIGURE.match(line)
        if figure:
            path, anchor = figure.groups()
            refs[anchor] = ('img', re.sub(r'\.[^/.]+$', '', path).removeprefix('images/'))
        caption = CAPTION.match(line)
        if caption:
            anchor, tsize, _text = caption.groups()
            refs[anchor] = ('table', anchor, tsize) if anchor.startswith('tbl-') else ('list', anchor)

    def reference(match):
        anchor = match.group(1)
        if anchor not in refs:
            raise ValueError(f'{source}: unresolved internal reference: {anchor}')
        kind, identifier = refs[anchor][:2]
        return f'[{identifier}]{{.{kind}}}'

    result, markers = [], {}
    fenced = False
    for line in lines:
        if line.startswith('```'):
            fenced = not fenced
            result.append(line)
            continue
        if fenced:
            result.append(line)
            continue
        if line.strip() == PAGEBREAK:
            result.extend(('', PAGEBREAK_MARKER, ''))
            continue
        if line.strip() == ENLARGEPAGE:
            result.extend(('', ENLARGEPAGE_MARKER, ''))
            continue
        line = re.sub(r'\[[^\]]+\]\(#((?:fig|tbl|lst)-[^)]+)\)', reference, line)
        line = line.replace('▸', '>')
        line = unescape_links(line)
        caption = CAPTION.match(line)
        if caption:
            anchor, _tsize, text = caption.groups()
            token = f'REVIEWCAPTIONMARKER{len(markers)}END'
            if token in markdown or PAGEBREAK_MARKER in markdown or ENLARGEPAGE_MARKER in markdown:
                raise ValueError('reserved caption marker in manuscript')
            markers[token] = refs[anchor]
            line = token + ' ' + text
        heading = re.match(r'^(#{2,6}) +(.+)$', line)
        if heading:
            title = PREPARE.plain_text(heading[2])
            title = re.sub(r'^\d+(?:\.\d+)*(?:\\\.|\.)?\s+', '', title)
            line = heading[1] + ' ' + title
        # Google export's escaped list markers need a paragraph boundary.
        is_list = bool(re.match(r'^\s*(?:\d+\.|[-+*]) ', line))
        if is_list and result and result[-1] and not re.match(r'^\s*(?:\d+\.|[-+*]) ', result[-1]):
            result.append('')
        if not is_list and line and not line[0].isspace() and result and re.match(r'^\s*(?:\d+\.|[-+*]) ', result[-1]):
            result.append('')
        result.append(line)
    return '\n'.join(result) + '\n', markers


def finish(review, markers):
    result, pending = [], None
    in_block = False
    for line in review.splitlines():
        if in_block:
            result.append(line)
            if line == '//}':
                in_block = False
            continue
        if line == PAGEBREAK_MARKER:
            if pending:
                raise ValueError('caption without associated table or code')
            result.append('//pagebreak')
            continue
        if line == ENLARGEPAGE_MARKER:
            if pending:
                raise ValueError('caption without associated table or code')
            result.extend(('//embed[latex]{', '\\enlargethispage{\\baselineskip}', '//}'))
            continue
        match = re.match(r'^(REVIEWCAPTIONMARKER\d+END) (.*)$', line)
        if match:
            if pending:
                raise ValueError('caption without associated table or code')
            kind, identifier, *tsize = markers[match[1]]
            pending = kind, identifier, match[2].replace(']', r'\]'), next(iter(tsize), None)
            continue
        if pending and line.strip():
            kind, identifier, caption, tsize = pending
            if kind == 'table' and line == '//table{':
                if tsize:
                    result.append(f'//tsize[|latex|{tsize}]')
                line = f'//table[{identifier}][{caption}]{{'
            elif kind == 'list' and line.startswith('//emlist'):
                language = re.search(r'\[([^\]]*)\]\{$', line)
                suffix = '[' + language[1] + ']' if language else ''
                line = f'//list[{identifier}][{caption}]{suffix}{{'
            else:
                raise ValueError(f'caption followed by unexpected block: {line}')
            pending = None
        # pandoc2review writes a captionless image's size where Re:VIEW reads the caption.
        line = re.sub(r'^(//indepimage\[[^\]]*\])(\[(?:scale|width)=[^\]]*\]\{)$', r'\1[]\2', line)
        result.append(line)
        if line.startswith('//') and line.endswith('{'):
            in_block = True
    if pending:
        raise ValueError('caption at end of manuscript')
    return '\n'.join(result) + '\n'


def inline(operation, text):
    """Pick the delimiter Re:VIEW accepts for this operand."""
    if '{' not in text and '}' not in text and not text.endswith('\\'):
        return f'@<{operation}>{{{text}}}'
    return f'@<{operation}>${text}$' if '$' not in text else f'@<{operation}>|{text}|'


def flatten_nested_inline(review, source):
    """Rewrite nested inline commands, which Re:VIEW would print as text."""
    def flatten(match):
        outer, content = match.groups()
        pieces, position = [], 0
        for nested in INNER_INLINE.finditer(content):
            combined = COMBINED_INLINE.get((outer, nested[1]))
            if combined is None:
                raise ValueError(f'{source}: cannot flatten nested inline command: {match[0]}')
            pieces.append((outer, content[position:nested.start()]))
            pieces.append((combined, nested[2]))
            position = nested.end()
        pieces.append((outer, content[position:]))
        return ''.join(inline(operation, text) for operation, text in pieces if text)

    result, verbatim = [], False
    for line in review.splitlines():
        if verbatim:
            verbatim = line != '//}'
        else:
            line = NESTED_INLINE.sub(flatten, line)
            opening = re.match(r'^//([a-z]+).*\{$', line)
            verbatim = bool(opening) and opening[1] in VERBATIM_BLOCKS
        result.append(line)
    return '\n'.join(result) + '\n'


def print_link_urls(review):
    """Typeset the URL of every link, and let long URLs break across lines."""
    def label_before_url(match, item=False):
        url, label = match.groups()
        if label == url:
            return f'@<href>{{{url}}}'
        # In a bulleted list such as references, a URL that follows its label
        # on the same line leaves too few break points, so the justified line
        # spreads the label's spaces and the URL's characters differently in
        # each item. Starting the URL on its own line keeps the items uniform.
        return f'{label}@<br>{{}}@<href>{{{url}}}' if item else f'{label} @<href>{{{url}}}'

    def wrap(match):
        if match[1]:
            return match[1]
        url = match[2].rstrip('.,;:')
        return f'@<href>{{{url}}}' + match[2][len(url):]

    result, verbatim = [], False
    for line in review.splitlines():
        if verbatim:
            verbatim = line != '//}'
        else:
            item = bool(re.match(r'^ +(?:\*|\d+\.) ', line))
            line = LABELED_LINK.sub(lambda match: label_before_url(match, item), line)
            line = BARE_URL.sub(wrap, line)
            opening = re.match(r'^//([a-z]+).*\{$', line)
            verbatim = bool(opening) and opening[1] in VERBATIM_BLOCKS
        result.append(line)
    return '\n'.join(result) + '\n'


def insert_profile(review, profile):
    """Place the typeset author profile directly below the chapter title."""
    lines = review.splitlines()
    for index, line in enumerate(lines):
        if line.startswith('= '):
            block = author_profile.review_block(profile).splitlines()
            return '\n'.join(lines[:index + 1] + [''] + block + lines[index + 1:]) + '\n'
    raise ValueError('converted manuscript has no chapter title')


def translate_module():
    path = Path(__file__).resolve().parents[1] / 'translation2026' / 'translate_md.py'
    spec = importlib.util.spec_from_file_location('translate_md', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def localize_affiliation(profile, source):
    """Translated editions (text_{lang}_2026) show the affiliation in their own language."""
    match = re.fullmatch(r'text_(.+)_2026', source.resolve().parent.name)
    if profile is None or not match:
        return profile
    module = translate_module()
    localized = {**profile, 'affiliation': module.localize_author(profile['affiliation'], match[1])}
    if profile.get('romaji'):  # translated editions show names in Latin script only
        localized.update(name=profile['romaji'], romaji='')
    return localized


def convert(source):
    profile = localize_affiliation(author_profile.load().get(source.stem), source)
    if profile is None:
        print(f'{source.name}: no author profile in articles/authors.json', file=sys.stderr)
    markdown, markers = prepare(source, profile)
    with tempfile.TemporaryDirectory(prefix='review-source-') as directory:
        prepared = Path(directory) / (source.stem + '.md')
        prepared.write_text(markdown)
        process = subprocess.run(['pandoc2review', str(prepared)], check=True,
                                 capture_output=True, text=True)
    if process.stderr:
        print(process.stderr, file=sys.stderr, end='')
    review = print_link_urls(flatten_nested_inline(finish(process.stdout, markers), source))
    review = insert_profile(review, profile) if profile else review
    return pdf_font_fallback(review, source)


def pdf_font_fallback(review, source):
    """Glyph swaps are for the PDF fonts only (build-review-pdf.py sets the variable); the Web build keeps the text."""
    match = re.fullmatch(r'(?:text|editorial)_(.+)_2026', source.resolve().parent.name)
    if match and os.environ.get('REVIEW_FONT_FALLBACK') == '1':
        return translate_module().font_fallback(review, match[1])
    return review


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    # Editorial .re files are already Re:VIEW; they only need the PDF glyph swap.
    converted = pdf_font_fallback(args.source.read_text(), args.source) if args.source.suffix == '.re' else convert(args.source)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(converted)
