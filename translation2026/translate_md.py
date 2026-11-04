#!/usr/bin/env python3
"""Markdown manuscript translation harness (the translating itself is done by Claude).

articles/text_jp/X.md is split into one unit per line. Structure (heading and list
markers, code blocks, URLs, {{..}} labels, image references, structural labels such
as 脚注: / 図: / 表:) is kept by this script; only the prose is handed to the
translator, with protected spans replaced by tokens like ⟦P1⟧.

  prepare  LANG STEM...   write tmp/translation2026/LANG/STEM.pending.json
  apply    LANG STEM...   merge STEM.translated.json, validate, write text_LANG_2026/STEM.md
  pack     LANG STEM...   write STEM.pack.json: [{id, src, tgt, section}] for independent review
  sync     LANG STEM...   take hand edits of the committed output back into memory (reviewed) and
                          overrides/LANG.json (raw lines); dry run unless --write
  selftest STEM...        identity round trip; must reproduce the source byte for byte

A STEM starting with "editorial-" is a Re:VIEW front/back matter file
(articles/editorial/STEM.re, output articles/editorial_LANG_2026/STEM.re); the
inline commands @<br>{} @<fn>{..} @<href>{..} @<title>{..} @<tt>{..} and every
//block line are kept, @<b>{..} stays in the text and must be kept by the translator.

Reviewed or earlier translations are cached in translation2026/memory/LANG.json,
keyed by the sha256 of the masked source line, so unchanged lines are never
translated again and reviewed text is never overwritten.
"""
import argparse
import difflib
import hashlib
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC_DIR = ROOT / 'articles' / 'text_jp'
WORK = ROOT / 'tmp' / 'translation2026'
MEMORY_DIR = ROOT / 'translation2026' / 'memory'
OVERRIDES_DIR = ROOT / 'translation2026' / 'overrides'

TOKEN = '⟦P{}⟧'
TOKEN_RE = re.compile(r'⟦P(\d+)⟧')
JA_CHARS = re.compile(r'[぀-ヿ㐀-鿿]')
KANA = re.compile(r'[぀-ヺー-ヿ]')  # excludes the middle dot U+30FB

# Spans that must reach the output unchanged.
PROTECT = re.compile(
    r'\{\{[^}]*\}\}'                       # {{fn:..}} {{fig:..}} {{list:..}}
    r'|\\`[^`\n]*?\\`|`[^`\n]+`'          # inline code (Google export escapes backticks)
    r'|!\[[^\]]*\]\[[^\]]*\]'              # ![][image1]
    r'|\]\((?:[^)\n]*)\)'                  # link target of [text](url)
    r'|https?://[\x21-\x7e]+?(?=[)\]>]|[^\x21-\x7e]|$)'  # bare URLs (ASCII only: stop at full-width text)
)
# Structure in front of the prose: indent, heading / list marker, structural label.
PREFIX = re.compile(
    r'^(\s*(?:#{1,6}[ \t]+|[*+-][ \t]+|\d+\\?\.[ \t]+|>[ \t]*'
    r'|(?:脚注|図|表)[:：][ \t]*(?:\{\{[^}]*\}\}[ \t]*)?)?)')
CODE_START = re.compile(r'^(CODE START\s+\S+\s*\|\s*)(.*?)(\s*)$')
# Google export wraps the code-block heading in backticks: `CODE START lang | caption {{list:..}}`
CODE_CAPTION = re.compile(r'^(\s*`CODE START\s+\S+\s*\|\s*)(.*?)((?:\s*\{\{[^}]*\}\})?\s*`\s*)$')
RE_PROTECT = re.compile(r'@<(?:br|fn|href|title|tt)>\{[^}]*\}')
RE_PREFIX = re.compile(r'^(=+(?:\[nonum\])?(?:\{[^}]*\})?[ \t]*|[ \t]+\*[ \t]+)?')
TABLE_SEP = re.compile(r'^\s*\|[\s:|-]+\|?\s*$')
IMAGE_DEF = re.compile(r'^\[[^\]]+\]:\s')
AUTHOR_LINE = re.compile(
    r'^(\*\s+)?(名前(\s*/\s*ローマ字)?|所属)\s*[:：]'
    r'|^(\*\s+)?[^\s`#|].*[/／]\s*[A-Za-z][^/／]*[/／]\s*(所属[:：]\s*)?(株式会社|QualiArts)')
AUTHOR_LABELS = {  # longest first; personal names are kept as written
    'en': [('名前 / ローマ字', 'Name / Romanized name'), ('名前', 'Name'), ('所属', 'Affiliation'),
           ('株式会社サイバーエージェント SGEコア技術本部', 'CyberAgent, Inc. SGE Core Technology Division'),
           ('株式会社サムザップ', 'Sumzap, Inc.'), ('株式会社アプリボット', 'Applibot, Inc.'),
           ('株式会社Colorful Palette', 'Colorful Palette')],
    'ko': [('名前 / ローマ字', '이름 / 로마자'), ('名前', '이름'), ('所属', '소속'),
           ('株式会社サイバーエージェント SGEコア技術本部', '주식회사 사이버에이전트 SGE 코어 기술본부'),
           ('株式会社サムザップ', '주식회사 Sumzap'), ('株式会社アプリボット', '주식회사 Applibot'),
           ('株式会社Colorful Palette', '주식회사 Colorful Palette')],
    'zh-hans': [('名前 / ローマ字', '姓名 / 罗马字'), ('名前', '姓名'), ('所属', '所属'),
                ('株式会社サイバーエージェント SGEコア技術本部', '株式会社 CyberAgent SGE 核心技术本部'),
                ('株式会社サムザップ', '株式会社 Sumzap'), ('株式会社アプリボット', '株式会社 Applibot'),
                ('株式会社Colorful Palette', '株式会社 Colorful Palette')],
    'zh-hant': [('名前 / ローマ字', '姓名 / 羅馬字'), ('名前', '姓名'), ('所属', '所屬'),
                ('株式会社サイバーエージェント SGEコア技術本部', '株式會社 CyberAgent SGE 核心技術本部'),
                ('株式会社サムザップ', '株式會社 Sumzap'), ('株式会社アプリボット', '株式會社 Applibot'),
                ('株式会社Colorful Palette', '株式會社 Colorful Palette')],
}


def japanese_names():
    profiles = json.loads((ROOT / 'articles' / 'authors.json').read_text(encoding='utf-8'))['articles']
    names = set()
    for profile in profiles.values():
        names.update({profile['name'], profile['name'].replace(' ', '')})
    return names


# PDF 用の Noto Sans CJK（KR/TC）に字形がない日本語の新字体などを、その言語のフォントにある字形へ置き換える。
# 著者名（日本語表記）と、日本語の例文が残る箇所のため。字形は articles/fonts/noto-cjk の cmap で確認した。
FONT_FALLBACK = {
    'ko': str.maketrans('畳瀬戸択尚稲数変枠▸', '疊瀨戶擇尙稻數變框›'),
    'zh-hans': str.maketrans('▸', '›'),
    'zh-hant': str.maketrans('図畳戸徳択気数値変拡▸', '圖疊戶德擇氣數值變擴›'),
}


# Spans of Re:VIEW text that must never be rewritten: URLs, file paths and reference ids.
FALLBACK_KEEP = re.compile(
    r'https?://\S+'
    r'|@<href>\{[^},]*'
    r'|@<(?:img|list|table|chap|chapref|hd|column|fn|eq|bib|labelref)>\{[^}]*\}'
    r'|(?m:^)//\w+\[[^\]]*\]')  # first argument of a block command: //image[path], //list[id]


def font_fallback(text, lang):
    """Swap glyphs the PDF font lacks. Only prose and names change; URLs, paths and ids are kept."""
    table = FONT_FALLBACK.get(lang)
    if not table:
        return text
    kept = []

    def hold(match):
        kept.append(match.group(0))
        return f'\ue000{len(kept) - 1}\ue001'

    text = FALLBACK_KEEP.sub(hold, text).translate(table)
    return re.sub('\ue000(\\d+)\ue001', lambda m: kept[int(m.group(1))], text)


def localize_author(line, lang):
    # Translated editions drop the Japanese-script name; whatever is already written in Latin script stays.
    for ja in sorted(japanese_names(), key=len, reverse=True):
        line = re.sub(re.escape(ja) + r'\s*(?:[/／]|\\-)?\s*', '', line)
    if lang == 'en':  # no Japanese name left to carry, so the romaji is the name
        line = line.replace('名前 / ローマ字', '名前')
        line = re.sub(r'^((?:\*\s+)?)ローマ字', r'\1名前', line)
    for ja, tr in AUTHOR_LABELS.get(lang, []):
        line = line.replace(ja, tr)
    if re.fullmatch(r'(?:\*\s+)?[^:：\s][^:：]*[:：]\s*', line):  # the name was the only value on the line
        return ''
    return line


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()[:16]


def mask(text, re_mode=False):
    protected = []

    def repl(match):
        protected.append(match.group(0))
        return TOKEN.format(len(protected) - 1)

    return (RE_PROTECT if re_mode else PROTECT).sub(repl, text), protected


def unmask(text, protected):
    return TOKEN_RE.sub(lambda m: protected[int(m.group(1))], text)


def split_units(source):
    """Return lines as a list of dicts: verbatim ones carry 'raw', units add prefix/body/suffix."""
    lines = source.split('\n')
    units = []
    in_code = fenced = False
    for line in lines:
        if in_code:
            is_end = line.strip().strip('`') == 'CODE END'
            units.append({'raw': line} if is_end else code_unit(line))
            in_code = not is_end
            continue
        if fenced:
            units.append({'raw': line})
            fenced = not line.startswith('```')
            continue
        if line.startswith('```'):
            units.append({'raw': line})
            fenced = True
            continue
        caption = CODE_CAPTION.match(line)
        if caption and caption.group(2):
            units.append(make_unit(caption.group(1), caption.group(2), caption.group(3)))
            in_code = True
            continue
        if re.match(r'^\s*`CODE START\b', line):
            units.append({'raw': line})
            in_code = True
            continue
        start = CODE_START.match(line)
        if line.startswith('CODE START'):
            in_code = True
            if start and start.group(2):
                units.append(make_unit(start.group(1), start.group(2), start.group(3)))
            else:
                units.append({'raw': line})
            continue
        if (not line.strip() or TABLE_SEP.match(line) or IMAGE_DEF.match(line)
                or AUTHOR_LINE.match(line)):
            units.append({'raw': line})
            continue
        prefix = PREFIX.match(line).group(1)
        rest = line[len(prefix):]
        body = rest.rstrip()
        units.append(make_unit(prefix, body, rest[len(body):]))
    return units


def split_units_re(source):
    """Re:VIEW front/back matter: //blocks and #@# comments stay verbatim, the rest is prose."""
    units = []
    for line in source.split('\n'):
        if line.startswith('//') or line.startswith('#@#'):
            units.append({'raw': line})
            continue
        prefix = RE_PREFIX.match(line).group(1) or ''
        rest = line[len(prefix):]
        body = rest.rstrip()
        units.append(make_unit(prefix, body, rest[len(body):], re_mode=True))
    return units


COMMENT_MARK = re.compile(r'(?://+|\\?#+|/\*+|<!--)')
CODE_CHARS = re.compile(r'[{};]')


def code_unit(line):
    """A line inside a code block that contains Japanese: keep the code, translate the text.

    The text starts at the nearest comment marker in front of the first Japanese
    character (when only plain words lie in between), otherwise at the whitespace-delimited
    token that holds it. Everything before is kept verbatim by this script.
    """
    if not JA_CHARS.search(line):
        return {'raw': line}
    lead = ''
    trail = ''
    inner = line
    wrapped = re.match(r'^(\s*`)(.*?)(`\s*)$', line)  # Google export wraps some code lines in backticks
    if wrapped and wrapped.group(2).count('`') == 0:
        lead, inner, trail = wrapped.groups()
    first = JA_CHARS.search(inner).start()
    if not COMMENT_MARK.search(inner[:first]) and re.search(r'["\']\s*\)*\s*;\s*$', inner):
        return {'raw': line}  # a Japanese string literal is code, not a comment
    start = inner.rfind(' ', 0, first) + 1
    head = inner[:start]
    marks = list(COMMENT_MARK.finditer(head))
    if marks and not CODE_CHARS.search(head[marks[-1].end():]):
        start = marks[-1].end()
        start += len(inner[start:]) - len(inner[start:].lstrip())
    body = inner[start:]
    rest = body.rstrip()
    unit = make_unit(lead + inner[:start], rest, body[len(rest):] + trail)
    unit['code'] = True
    return unit


def make_unit(prefix, body, suffix, re_mode=False):
    masked, protected = mask(body, re_mode)
    if not JA_CHARS.search(masked):
        return {'raw': prefix + body + suffix}
    return {'prefix': prefix, 'suffix': suffix, 'masked': masked,
            'protected': protected, 'hash': sha(masked)}


def load_memory(lang):
    path = MEMORY_DIR / f'{lang}.json'
    return json.loads(path.read_text()) if path.exists() else {}


def save_memory(lang, memory):
    MEMORY_DIR.mkdir(parents=True, exist_ok=True)
    (MEMORY_DIR / f'{lang}.json').write_text(
        json.dumps(memory, ensure_ascii=False, indent=1, sort_keys=True) + '\n')


def load_overrides(lang):
    """Hand edits of verbatim (raw) lines: {stem: {source line: [replacement lines]}}."""
    path = OVERRIDES_DIR / f'{lang}.json'
    return json.loads(path.read_text()) if path.exists() else {}


def save_overrides(lang, overrides):
    OVERRIDES_DIR.mkdir(parents=True, exist_ok=True)
    (OVERRIDES_DIR / f'{lang}.json').write_text(
        json.dumps(overrides, ensure_ascii=False, indent=1, sort_keys=True) + '\n')


def is_re(stem):
    return stem.startswith('editorial-')


def read_source(stem):
    if is_re(stem):
        return (ROOT / 'articles' / 'editorial' / f'{stem}.re').read_text()
    return (SRC_DIR / f'{stem}.md').read_text()


def units_of(stem):
    source = read_source(stem)
    return split_units_re(source) if is_re(stem) else split_units(source)


def out_path(lang, stem):
    if is_re(stem):
        return ROOT / 'articles' / f'editorial_{lang}_2026' / f'{stem}.re'
    return ROOT / 'articles' / f'text_{lang}_2026' / f'{stem}.md'


def render_lines(units, translations, lang=None, overrides=None):
    """Rendered output as [(unit index, line)]; an override may replace one raw line by any number of lines."""
    out = []
    for index, unit in enumerate(units):
        if 'raw' in unit:
            raw = unit['raw']
            if overrides and raw in overrides:
                out.extend((index, line) for line in overrides[raw])
                continue
            raw = localize_author(raw, lang) if lang and AUTHOR_LINE.match(raw) else raw
            if raw or 'raw' not in unit or not AUTHOR_LINE.match(unit['raw']):
                out.append((index, raw))
            continue
        if overrides:
            source_line = unit['prefix'] + unmask(unit['masked'], unit['protected']) + unit['suffix']
            if source_line in overrides:
                out.extend((index, line) for line in overrides[source_line])
                continue
        body = unmask(translations[unit['hash']], unit['protected'])
        out.append((index, unit['prefix'] + body + unit['suffix']))
    if lang == 'en':  # the romanized name is already the name itself
        out = [(u, line) for i, (u, line) in enumerate(out) if not (
            (m := re.match(r'^(?:\*\s+)?Romanized name\s*[:：]\s*(.+?)\s*$', line))
            and i and out[i - 1][1].rstrip().endswith(m[1]))]
        # with the Japanese name line gone, a lone romanized-name line is the name
        out = [(u, re.sub(r'^((?:\*\s+)?)Romanized name', r'\1Name', line)) for u, line in out]
    return out


def source_line_of(unit):
    if 'raw' in unit:
        return unit['raw']
    return unit['prefix'] + unmask(unit['masked'], unit['protected']) + unit['suffix']


def render(units, translations, lang=None, overrides=None):
    return '\n'.join(line for _, line in render_lines(units, translations, lang, overrides))


def section_of(units, index):
    for unit in reversed(units[:index]):
        if 'raw' not in unit and not unit.get('code') and unit['prefix'].lstrip().startswith('#'):
            return unit['masked']
    return ''


def cmd_prepare(lang, stems):
    memory = load_memory(lang)
    out_dir = WORK / lang
    out_dir.mkdir(parents=True, exist_ok=True)
    for stem in stems:
        units = units_of(stem)
        pending, seen = [], set()
        total = 0
        for index, unit in enumerate(units):
            if 'raw' in unit:
                continue
            total += 1
            if unit['hash'] in memory or unit['hash'] in seen:
                continue
            seen.add(unit['hash'])
            pending.append({'id': unit['hash'], 'text': unit['masked'],
                            'section': section_of(units, index)})
        (out_dir / f'{stem}.pending.json').write_text(
            json.dumps(pending, ensure_ascii=False, indent=1) + '\n')
        chars = sum(len(p['text']) for p in pending)
        print(f'{stem}: {total} units, {len(pending)} pending ({chars} chars)')


def cmd_pack(lang, stems):
    memory = load_memory(lang)
    out_dir = WORK / lang
    out_dir.mkdir(parents=True, exist_ok=True)
    for stem in stems:
        units = units_of(stem)
        pairs, seen = [], set()
        for index, unit in enumerate(units):
            if 'raw' in unit or unit['hash'] in seen or unit['hash'] not in memory:
                continue
            seen.add(unit['hash'])
            pairs.append({'id': unit['hash'], 'src': unit['masked'], 'tgt': memory[unit['hash']]['t'],
                          'section': section_of(units, index)})
        (out_dir / f'{stem}.pack.json').write_text(json.dumps(pairs, ensure_ascii=False, indent=1) + '\n')
        print(f'{stem}: {len(pairs)} pairs')


def validate_unit(unit, translated):
    errors = []
    want = sorted(re.findall(r'⟦P\d+⟧', unit['masked']))
    got = sorted(TOKEN_RE.sub(lambda m: m.group(0), t) for t in re.findall(r'⟦P\d+⟧', translated))
    if want != got:
        errors.append(f'tokens differ: {want} -> {got}')
    if '\n' in translated:
        errors.append('newline inside a unit')
    if unit['masked'].count('@<b>{') != translated.count('@<b>{'):
        errors.append('@<b>{ count differs')
    if unit['masked'].count('**') != translated.count('**'):
        errors.append('** count differs')
    if unit['masked'].count('|') != translated.count('|'):
        errors.append('| count differs')
    return errors


def cmd_apply(lang, stems, force=False, overwrite_reviewed=False):
    memory = load_memory(lang)
    overrides = load_overrides(lang)
    out_dir = ROOT / 'articles' / f'text_{lang}_2026'
    failed = False
    for stem in stems:
        units = units_of(stem)
        pending_path = WORK / lang / f'{stem}.translated.json'
        new = json.loads(pending_path.read_text()) if pending_path.exists() else {}
        problems = []
        by_hash = {u['hash']: u for u in units if 'raw' not in u}
        staged = {}
        for key, text in new.items():
            unit = by_hash.get(key)
            if unit is None:
                problems.append(f'{key}: unknown id')
                continue
            errors = validate_unit(unit, text)
            if errors:
                problems.append(f'{key}: {errors}  src={unit["masked"][:50]}')
            elif memory.get(key, {}).get('reviewed') and memory[key]['t'] != text and not overwrite_reviewed:
                problems.append(f'{key}: reviewed translation would be overwritten (--overwrite-reviewed)  '
                                f'src={unit["masked"][:50]}')
            else:
                staged[key] = {'t': text, 'reviewed': False}
        missing = [h for h in by_hash if h not in memory and h not in staged]
        if missing:
            problems.append(f'{len(missing)} units untranslated')
        text = None
        if not problems:
            merged = {**memory, **staged}
            text = render(units, {h: merged[h]['t'] for h in by_hash}, lang, overrides.get(stem))
            problems += [f'check: {m}' for m in
                         check_output(read_source(stem), text, units, {h: merged[h]['t'] for h in by_hash}, lang)]
            target = out_path(lang, stem)
            if (not force and target.exists() and not staged
                    and font_fallback(target.read_text(), lang) != font_fallback(text, lang)):
                problems.append(f'{target.relative_to(ROOT)} differs from the memory rebuild '
                                f'(hand edit not in memory; run sync, or --force to discard it)')
        if problems:
            failed = True
            print(f'{stem}: FAILED, nothing written')
            for problem in problems[:20]:
                print('  ', problem)
            continue
        memory.update(staged)
        if pending_path.exists():  # consumed: a stale file must not overwrite later hand edits
            pending_path.rename(pending_path.with_suffix('.applied'))
        target = out_path(lang, stem)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)
        print(f'{stem}: wrote {target.relative_to(ROOT)}')
    save_memory(lang, memory)
    link = out_dir / 'assets'
    if out_dir.exists() and not link.exists():
        link.symlink_to('../text_jp/assets')
    return 1 if failed else 0


def remask(body, protected):
    for number, span in enumerate(protected):
        body = body.replace(span, TOKEN.format(number), 1)
    return body


def cmd_sync(lang, stems, write=False):
    """Bring hand edits of the committed output back into memory (units) and overrides (raw lines)."""
    memory = load_memory(lang)
    overrides = load_overrides(lang)
    notes, unresolved = [], []
    for stem in stems:
        units = units_of(stem)
        by_hash = {u['hash'] for u in units if 'raw' not in u}
        if by_hash - set(memory):
            unresolved.append(f'{stem}: units missing in memory, run prepare/apply first')
            continue
        target = out_path(lang, stem)
        if not target.exists():
            continue
        cur = target.read_text().split('\n')
        base = render_lines(units, {h: memory[h]['t'] for h in by_hash}, lang, overrides.get(stem))
        sm = difflib.SequenceMatcher(None, [line for _, line in base], cur, autojunk=False)
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if tag == 'equal':
                continue
            block = base[i1:i2]
            if tag == 'replace' and i2 - i1 == j2 - j1:
                for (u, was), now in zip(block, cur[j1:j2]):
                    unit = units[u]
                    if 'raw' in unit:
                        if now == font_fallback(was, lang):
                            notes.append(f'{stem}: raw line carries font fallback (apply restores it): {now[:50]}')
                            continue
                        overrides.setdefault(stem, {})[unit['raw']] = [now]
                        notes.append(f'{stem}: override  {unit["raw"][:40]!r} -> {now[:50]!r}')
                        continue
                    if font_fallback(now, lang) == font_fallback(was, lang):
                        notes.append(f'{stem}: unit carries font fallback only (apply restores it): {now[:50]}')
                        continue
                    pre, suf = unit['prefix'], unit['suffix']
                    if not now.startswith(pre) or (suf and not now.endswith(suf)):
                        unresolved.append(f'{stem}: prefix/suffix changed: {now[:60]}')
                        continue
                    body = remask(now[len(pre):len(now) - len(suf)], unit['protected'])
                    errors = validate_unit(unit, body)
                    if errors:
                        unresolved.append(f'{stem}: {errors} {now[:60]}')
                        continue
                    memory[unit['hash']] = {'t': body, 'reviewed': True}
                    notes.append(f'{stem}: memory    {unit["masked"][:40]!r} -> {body[:50]!r}')
            elif block and tag in ('replace', 'delete'):
                first, *rest = (source_line_of(units[u]) for u, _ in block)
                overrides.setdefault(stem, {})[first] = cur[j1:j2]
                for raw in rest:
                    overrides[stem][raw] = []
                notes.append(f'{stem}: override  {len(block)} lines -> {len(cur[j1:j2])} lines: {cur[j1:j2][:1]}')
            else:
                unresolved.append(f'{stem}: cannot align lines {i1}-{i2} vs {j1}-{j2}')
    # verify what sync would leave behind
    for stem in stems:
        units = units_of(stem)
        by_hash = {u['hash'] for u in units if 'raw' not in u}
        target = out_path(lang, stem)
        if by_hash - set(memory) or not target.exists():
            continue
        rebuilt = render(units, {h: memory[h]['t'] for h in by_hash}, lang, overrides.get(stem)).split('\n')
        left = [(a, b) for a, b in zip(target.read_text().split('\n'), rebuilt)
                if a != b and a != font_fallback(b, lang)]
        if left or len(rebuilt) != len(target.read_text().split('\n')):
            unresolved.append(f'{stem}: still differs after sync ({len(left)} lines), e.g. {left[:1]}')
    for note in notes + unresolved:
        print(note)
    print(f'{lang}: {len(notes)} changes, {len(unresolved)} unresolved' + ('' if write else ' (dry run; --write to save)'))
    if write and not unresolved:
        save_memory(lang, memory)
        save_overrides(lang, overrides)
    return 1 if unresolved else 0


def code_blocks(lines):
    """Bodies of the code blocks, one list per block."""
    blocks, current = [], None
    for line in lines:
        if current is not None:
            if line.strip().strip('`') == 'CODE END':
                blocks.append(current)
                current = None
            else:
                current.append(line)
        elif line.startswith('CODE START') or re.match(r'^\s*`CODE START\b', line):
            current = []
    return blocks


def check_output(src, out, units, translations, lang):
    """Consistency checks between the Japanese source and the translation; every message is an error.

    Kana is looked for in the translated prose only (protected spans are masked), so verbatim
    lines (author names, string literals, Re:VIEW comments) are not flagged.
    """
    messages = []
    for name, pattern in (('{{..}}', r'\{\{[^}]*\}\}'), ('URL', r'https?://[\x21-\x7e]+?(?=[)\]>]|[^\x21-\x7e]|$)'),
                          ('image ref', r'!\[[^\]]*\]\[[^\]]*\]'), ('heading', r'(?m)^#{1,6} ')):
        if sorted(re.findall(pattern, src)) != sorted(re.findall(pattern, out)):
            messages.append(f'{name} set differs')
    src_blocks, out_blocks = code_blocks(src.split('\n')), code_blocks(out.split('\n'))
    if len(src_blocks) != len(out_blocks):
        messages.append(f'code block count differs: {len(src_blocks)} -> {len(out_blocks)}')
    else:
        for number, (a, b) in enumerate(zip(src_blocks, out_blocks), 1):
            if len(a) != len(b):
                messages.append(f'code block {number}: line count differs')
            elif [x for x in a if not JA_CHARS.search(x)] != [y for x, y in zip(a, b) if not JA_CHARS.search(x)]:
                messages.append(f'code block {number}: body differs')  # lines with Japanese are translated
    if lang != 'ja':
        leftover = [translations[u['hash']] for u in units
                    if 'raw' not in u and KANA.search(translations[u['hash']])]
        if leftover:
            messages.append(f'{len(leftover)} translated units still contain kana: {leftover[0][:60]}')
    return messages


def cmd_selftest(stems):
    ok = True
    for stem in stems:
        source = read_source(stem)
        units = units_of(stem)
        identity = {u['hash']: u['masked'] for u in units if 'raw' not in u}
        same = render(units, identity) == source
        translatable = sum(1 for u in units if 'raw' not in u)
        print(f'{stem}: round trip {"OK" if same else "MISMATCH"} ({translatable} units)')
        ok &= same
    return 0 if ok else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    for name in ('prepare', 'apply', 'pack', 'sync'):
        p = sub.add_parser(name)
        p.add_argument('lang')
        p.add_argument('stems', nargs='+')
        if name == 'apply':
            p.add_argument('--force', action='store_true', help='overwrite a file that differs from the memory rebuild')
            p.add_argument('--overwrite-reviewed', action='store_true', help='let new translations replace reviewed ones')
        if name == 'sync':
            p.add_argument('--write', action='store_true', help='save memory/overrides (default: dry run)')
    p = sub.add_parser('selftest')
    p.add_argument('stems', nargs='+')
    args = parser.parse_args()
    if args.command == 'prepare':
        cmd_prepare(args.lang, args.stems)
    elif args.command == 'pack':
        cmd_pack(args.lang, args.stems)
    elif args.command == 'apply':
        return cmd_apply(args.lang, args.stems, args.force, args.overwrite_reviewed)
    elif args.command == 'sync':
        return cmd_sync(args.lang, args.stems, args.write)
    else:
        return cmd_selftest(args.stems)


if __name__ == '__main__':
    sys.exit(main())
