#!/usr/bin/env python3
"""Combine Google Docs Markdown exports into a Pandoc-ready chapter book."""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable


HEADING_RE = re.compile(r"^(#{1,6})[ \t]+(.*?)[ \t]*$")
IMAGE_REFERENCE_RE = re.compile(r"!\[([^]]*)\]\[([^]]+)\](?:\{([^{}\n]*)\})?")
# pandoc2review turns either attribute into the [scale=...] option of //image.
IMAGE_SIZE_RE = re.compile(r"^(?:width=\d+(?:\.\d+)?%|scale=\d*\.?\d+)$")
# Explicit PDF column widths in mm: 表: 比較{{table:id}}{{tsize:30,20,20}}
TABLE_TSIZE_RE = re.compile(r"\{\{tsize:([\d., ]+)\}\}")
IMAGE_DEFINITION_RE = re.compile(r"^\[([^]]+)\]:\s+(\S+)\s*$")
SPECIAL_REFERENCE_RE = re.compile(
    r"\{\{(?P<kind>fig|table|list|fn):(?P<identifier>[^}]+)\}\}"
)
GOOGLE_CODE_ESCAPE_RE = re.compile(r"\\([_=><\[\]*\-#+!).])")
ESCAPED_INLINE_CODE_RE = re.compile(r"\\`(.*?)\\`")
RAW_INLINE_CODE_RE = re.compile(r"(?<!\\)`([^`\n]+)`")
ESCAPED_QUADRUPLE_STRONG_RE = re.compile(
    r"\\\*\\\*\*\*(.+?)\*\*\\\*\\\*"
)
ESCAPED_STRONG_RE = re.compile(r"\\\*\\\*(.+?)\\\*\\\*")

# "記事タイトル" and the "タイトル:" label under the translated headings.
ARTICLE_TITLE_HEADINGS = {"記事タイトル", "Article Title", "기사 제목", "文章标题", "文章標題"}
TITLE_LABEL_RE = re.compile(r"^(?:タイトル|Title|제목|标题|標題)\s*[：:]\s*(.+?)\s*$")

GENERIC_HEADINGS = {
    "スタイルガイド",
    "本文",
    "原稿",
    "記事タイトル",
    *ARTICLE_TITLE_HEADINGS,
    # Translated editions keep the template section under translated headings.
    "Style Guide", "Body", "Manuscript",
    "스타일 가이드", "본문", "원고",
    "样式指南", "风格指南", "正文",
    "樣式指南", "風格指南", "內文",
}

BODY_HEADINGS = {"本文", "Body", "본문", "正文", "內文"}
TEMPLATE_GUIDE_MARKERS = (
    "原稿作成テンプレート", "Manuscript Template Guide", "원고 작성 템플릿",
    "稿件撰写模板", "稿件撰寫範本",
)

METADATA_HEADINGS = {
    "名前・所属",
    "名前",
    "所属",
    "GitHubアカウント",
    "GitHub",
    "Xアカウント",
    "X",
    "Twitterアカウント",
    "Webサイト",
    # Translated editions.
    "Name and Affiliation", "GitHub Account", "X Account",
    "이름・소속", "GitHub 계정", "X 계정",
    "姓名・所属", "GitHub 账号", "X 账号",
    "姓名・所屬", "GitHub 帳號", "X 帳號",
}

# Author metadata appears in several shapes because each manuscript follows the
# template loosely. articles/authors.json is the source of truth, so these lines
# are removed and replaced by a typeset profile.
AUTHOR_LABEL_RE = re.compile(
    r"^\s*(?:[*+-]\s+)?(?:\*\*)?\s*"
    r"(?:名前|氏名|ローマ字|所属|会社|Xアカウント|X|Twitterアカウント|Twitter"
    r"|GitHubアカウント|GitHub|Webサイト|サイト|ブログ|URL"
    r"|Name|Romanized name|Affiliation|X account|이름|로마자|소속|X 계정|GitHub 계정"
    r"|姓名|罗马字|羅馬字|羅馬拼音|所屬|X 账号|X 帳號|GitHub 账号|GitHub 帳號)"
    r"[^:：]{0,40}?[:：]",
    re.IGNORECASE,
)
AUTHOR_LINK_RE = re.compile(
    r"^\s*(?:[*+-]\s+)?(?:\[[^]]*\]\()?<?https?://\S+?>?\)?\s*$"
)
AUTHOR_RULE_RE = re.compile(r"^\s*\\?-{3,}\s*$")


@dataclass(frozen=True)
class LabeledElement:
    kind: str
    number: int
    caption: str
    identifier: str | None
    anchor: str
    # Column widths in mm for the PDF table, from {{tsize:34,26,...}}.
    tsize: str | None = None


@dataclass
class DocumentReferences:
    chapter_number: int
    source_slug: str
    heading_base_level: int = 1
    figures_by_line: dict[int, LabeledElement] = field(default_factory=dict)
    tables_by_line: dict[int, LabeledElement] = field(default_factory=dict)
    code_by_line: dict[int, LabeledElement] = field(default_factory=dict)
    footnotes_by_line: dict[int, tuple[str, str]] = field(default_factory=dict)
    skipped_footnote_content_lines: set[int] = field(default_factory=set)
    image_captions: dict[tuple[int, int], LabeledElement] = field(default_factory=dict)
    image_paths: dict[str, str] = field(default_factory=dict)
    image_definition_lines: set[int] = field(default_factory=set)
    replacements: dict[tuple[str, str], str] = field(default_factory=dict)


def plain_text(value: str) -> str:
    """Return a conservative plain-text form suitable for a chapter title."""
    value = value.strip()
    while len(value) >= 4 and value.startswith("**") and value.endswith("**"):
        value = value[2:-2].strip()
    value = re.sub(r"!\[[^]]*\]\([^)]*\)", "", value)
    value = re.sub(r"\[([^]]+)\]\([^)]*\)", r"\1", value)
    value = value.replace("**", "").replace("__", "")
    value = re.sub(r"\\([#_*+.!()\[\]<>\-=])", r"\1", value)
    return re.sub(r"\s+", " ", value).strip()


def heading_at(line: str) -> tuple[int, str] | None:
    match = HEADING_RE.match(line.rstrip())
    if not match:
        return None
    return len(match.group(1)), match.group(2).strip()


def clean_heading_text(value: str) -> str:
    """Remove an extra escaped Markdown heading marker pasted into a heading."""
    return re.sub(r"^(\*\*)?(?:\\?#){1,6}[ \t]*", r"\1", value.strip())


def derive_title(lines: list[str]) -> tuple[str, set[int]]:
    """Derive the chapter title and source lines that should not be repeated."""
    for index, line in enumerate(lines):
        match = TITLE_LABEL_RE.match(line.rstrip())
        if match:
            return plain_text(match.group(1)), {index}

    for index, line in enumerate(lines):
        if not line.strip():
            continue
        match = re.match(r"^\s*[*+-]\s+(.+?)\s*$", line.rstrip())
        if match:
            return plain_text(match.group(1)), {index}
        break

    h1_entries: list[tuple[int, str, str]] = []
    for index, line in enumerate(lines):
        heading = heading_at(line)
        if heading and heading[0] == 1:
            h1_entries.append((index, heading[1], plain_text(heading[1])))

    if h1_entries and h1_entries[0][2] in ARTICLE_TITLE_HEADINGS:
        title_heading_index = h1_entries[0][0]
        for index in range(title_heading_index + 1, len(lines)):
            candidate = lines[index].strip()
            if not candidate:
                continue
            if heading_at(lines[index]):
                break
            return plain_text(candidate), {index}

    article_headings = [
        entry for entry in h1_entries if entry[2] and entry[2] not in GENERIC_HEADINGS
    ]
    if article_headings and any(m in article_headings[0][2] for m in TEMPLATE_GUIDE_MARKERS):
        for position, (_index, _raw, normalized) in enumerate(h1_entries):
            if normalized not in BODY_HEADINGS:
                continue
            for next_index, _next_raw, next_normalized in h1_entries[position + 1 :]:
                if next_normalized and next_normalized not in GENERIC_HEADINGS:
                    return next_normalized, set(range(next_index + 1))

    for index, _raw, normalized in article_headings:
        if normalized and normalized not in GENERIC_HEADINGS:
            return normalized, {index}

    raise ValueError("chapter title could not be derived")


def is_author_metadata(line: str) -> bool:
    """Report whether a line below the title carries author metadata."""
    value = re.sub(r"^\s*(?:[*+-]\s+)?", "", line.strip())
    value = re.sub(r"^\*\*(.*)\*\*$", r"\1", value).strip()
    if not value:
        return False
    if AUTHOR_RULE_RE.match(line) or AUTHOR_LINK_RE.match(line):
        return True
    if AUTHOR_LABEL_RE.match(line):
        return True
    # "名前 / ローマ字 / 所属" written without labels, never a body sentence.
    separated = re.split(r"\s*[/／]\s*", value)
    return len(separated) >= 2 and len(value) <= 90 and "。" not in value


def author_block_lines(lines: list[str], start: int, content_end: int) -> set[int]:
    """Return the manuscript lines that repeat the author metadata."""
    dropped: set[int] = set()
    blanks: list[int] = []
    index = start
    while index < content_end:
        line = lines[index]
        if not line.strip():
            blanks.append(index)
            index += 1
            continue
        heading = heading_at(line)
        if heading:
            if heading[0] != 1 or plain_text(heading[1]) not in METADATA_HEADINGS:
                break
            dropped.update(blanks)
            blanks = []
            dropped.add(index)
            index += 1
            while index < content_end and not heading_at(lines[index]):
                dropped.add(index)
                index += 1
            continue
        if not is_author_metadata(line):
            break
        dropped.update(blanks)
        blanks = []
        dropped.add(index)
        index += 1
    return dropped


def marker_text(line: str) -> str:
    value = line.strip()
    if len(value) >= 2 and value.startswith("`") and value.endswith("`"):
        value = value[1:-1].strip()
    return value


def parse_code_start(line: str) -> tuple[str, str, str | None] | None:
    match = re.fullmatch(
        r"CODE START\s+([^\s|]+)(?:\s*\|\s*(.*?))?",
        marker_text(line),
    )
    if not match:
        return None
    language = match.group(1).lower()
    raw_caption = match.group(2) or ""
    identifier_match = re.search(r"\{\{list:([^}]+)\}\}", raw_caption)
    identifier = identifier_match.group(1) if identifier_match else None
    caption = re.sub(r"\s*\{\{list:[^}]+\}\}\s*", "", raw_caption).strip()
    return language, caption, identifier


def is_code_end(line: str) -> bool:
    return marker_text(line) == "CODE END"


def parse_markdown_fence(line: str) -> tuple[int, str] | None:
    value = line.strip().replace(r"\`", "`")
    match = re.fullmatch(r"(`{3,})([^`]*)", value)
    if not match:
        return None
    info = match.group(2).strip()
    if info.startswith("{.") and info.endswith("}"):
        info = info[2:-1].split()[0]
    return len(match.group(1)), info


def clean_code_line(line: str) -> str:
    value = line.rstrip()
    match = re.fullmatch(r"([ \t]*)`(.*)`", value)
    if match:
        value = f"{match.group(1)}{match.group(2)}"
    value = GOOGLE_CODE_ESCAPE_RE.sub(r"\1", value)
    return value.replace("\\\\", "\\")


def append_blank(output: list[str]) -> None:
    if output and output[-1] != "":
        output.append("")


def safe_anchor_part(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]+", "-", value).strip("-")


def caption_parts(line: str, kind: str, prefix: str) -> tuple[str, str | None] | None:
    match = re.search(rf"{prefix}\s*[：:]\s*(.*)$", line.rstrip())
    if not match:
        return None
    body = match.group(1).strip()
    identifier_match = re.search(rf"\*?\{{\{{{kind}:([^}}]+)\}}\}}\*?", body)
    identifier = identifier_match.group(1) if identifier_match else None
    if identifier_match:
        body = f"{body[:identifier_match.start()]}{body[identifier_match.end():]}".strip()
    return body, identifier


def previous_nonblank(lines: list[str], start: int) -> int | None:
    for index in range(start - 1, -1, -1):
        if lines[index].strip():
            return index
    return None


def next_nonblank(lines: list[str], start: int) -> int | None:
    for index in range(start + 1, len(lines)):
        if lines[index].strip():
            return index
    return None


def collect_references(
    lines: list[str],
    chapter_number: int,
    source_slug: str,
    content_end: int | None = None,
    skipped_lines: set[int] | None = None,
) -> DocumentReferences:
    refs = DocumentReferences(chapter_number, source_slug)
    counters = {"fig": 0, "table": 0, "list": 0}

    for index, line in enumerate(lines):
        image_definition = IMAGE_DEFINITION_RE.match(line.rstrip())
        if image_definition:
            refs.image_paths[image_definition.group(1)] = image_definition.group(2)
            refs.image_definition_lines.add(index)

    content_lines = lines if content_end is None else lines[:content_end]
    for index, line in enumerate(content_lines):
        if skipped_lines and index in skipped_lines:
            continue
        figure = caption_parts(line, "fig", "図")
        if figure and (line.lstrip().startswith(("図:", "図：")) or IMAGE_REFERENCE_RE.search(line)):
            caption, identifier = figure
            counters["fig"] += 1
            anchor_suffix = identifier or str(counters["fig"])
            element = LabeledElement(
                "fig", counters["fig"], caption, identifier,
                f"fig-{source_slug}-{safe_anchor_part(anchor_suffix)}",
            )
            refs.figures_by_line[index] = element
            if identifier:
                refs.replacements[("fig", identifier)] = (
                    f"[図 {chapter_number}.{element.number}](#{element.anchor})"
                )

        table = caption_parts(line, "table", "表")
        if table and line.lstrip().startswith(("表:", "表：")):
            caption, identifier = table
            tsize = TABLE_TSIZE_RE.search(caption)
            if tsize:
                caption = f"{caption[:tsize.start()]}{caption[tsize.end():]}".strip()
            counters["table"] += 1
            anchor_suffix = identifier or str(counters["table"])
            element = LabeledElement(
                "table", counters["table"], caption, identifier,
                f"tbl-{source_slug}-{safe_anchor_part(anchor_suffix)}",
                tsize.group(1).replace(" ", "") if tsize else None,
            )
            refs.tables_by_line[index] = element
            if identifier:
                refs.replacements[("table", identifier)] = (
                    f"[表 {chapter_number}.{element.number}](#{element.anchor})"
                )

        code_start = parse_code_start(line)
        if code_start:
            _language, caption, identifier = code_start
            if caption or identifier:
                counters["list"] += 1
                anchor_suffix = identifier or str(counters["list"])
                element = LabeledElement(
                    "list", counters["list"], caption, identifier,
                    f"lst-{source_slug}-{safe_anchor_part(anchor_suffix)}",
                )
                refs.code_by_line[index] = element
                if identifier:
                    refs.replacements[("list", identifier)] = (
                        f"[コード {chapter_number}.{element.number}](#{element.anchor})"
                    )

        footnote = caption_parts(line, "fn", "脚注")
        if footnote and line.lstrip().startswith(("脚注:", "脚注：")):
            content, identifier = footnote
            if identifier:
                if not content:
                    following = next_nonblank(content_lines, index)
                    if following is not None:
                        candidate = content_lines[following].strip()
                        if re.fullmatch(
                            r"(?:https?://\S+|\[[^]]+\]\(https?://[^)]+\))",
                            candidate,
                        ):
                            content = candidate
                            refs.skipped_footnote_content_lines.add(following)
                label = f"fn-{source_slug}-{safe_anchor_part(identifier)}"
                refs.footnotes_by_line[index] = (label, content)
                refs.replacements[("fn", identifier)] = f"[^{label}]"

    for index, line in enumerate(content_lines):
        if skipped_lines and index in skipped_lines:
            continue
        bare = re.fullmatch(r"\s*\{\{fn:([^}]+)\}\}\s*", line)
        if not bare or ("fn", bare.group(1)) in refs.replacements:
            continue
        following = next_nonblank(lines, index)
        if following is None or not re.match(r"https?://", lines[following].strip()):
            continue
        identifier = bare.group(1)
        label = f"fn-{source_slug}-{safe_anchor_part(identifier)}"
        refs.footnotes_by_line[index] = (label, lines[following].strip())
        refs.skipped_footnote_content_lines.add(following)
        refs.replacements[("fn", identifier)] = f"[^{label}]"

    for caption_line, element in refs.figures_by_line.items():
        images_on_caption_line = list(IMAGE_REFERENCE_RE.finditer(lines[caption_line]))
        if images_on_caption_line:
            refs.image_captions[(caption_line, len(images_on_caption_line) - 1)] = element
            continue

        previous = previous_nonblank(lines, caption_line)
        if previous is not None:
            images = list(IMAGE_REFERENCE_RE.finditer(lines[previous]))
            if images:
                refs.image_captions[(previous, len(images) - 1)] = element
                continue

        following = next_nonblank(lines, caption_line)
        if following is not None:
            images = list(IMAGE_REFERENCE_RE.finditer(lines[following]))
            if images:
                refs.image_captions[(following, 0)] = element

    return refs


def transform_inline_markup(value: str, refs: DocumentReferences) -> str:
    protected: list[str] = []

    def protect(content: str) -> str:
        token = f"@@PANDOCINLINECODE{len(protected)}@@"
        cleaned = GOOGLE_CODE_ESCAPE_RE.sub(r"\1", content)
        protected.append(f"`{cleaned}`")
        return token

    value = ESCAPED_INLINE_CODE_RE.sub(lambda match: protect(match.group(1)), value)
    value = RAW_INLINE_CODE_RE.sub(lambda match: protect(match.group(1)), value)
    value = ESCAPED_QUADRUPLE_STRONG_RE.sub(r"**\1**", value)
    value = ESCAPED_STRONG_RE.sub(r"**\1**", value)

    def replace_reference(match: re.Match[str]) -> str:
        replacement = refs.replacements.get((match.group("kind"), match.group("identifier")))
        if replacement is not None:
            return replacement
        labels = {
            "fig": "図参照ID",
            "table": "表参照ID",
            "list": "コード参照ID",
            "fn": "出典ID",
        }
        return f"（{labels[match.group('kind')]}: {match.group('identifier')}）"

    value = SPECIAL_REFERENCE_RE.sub(replace_reference, value)
    value = value.replace(r"\`", "`")
    for index, code in enumerate(protected):
        value = value.replace(f"@@PANDOCINLINECODE{index}@@", code)
    return value.rstrip()


def emit_text_line(line: str, output: list[str], refs: DocumentReferences) -> None:
    if re.fullmatch(r"\s*#{1,6}\s*", line):
        return

    heading = heading_at(line)
    if heading:
        level, raw_heading = heading
        raw_heading = clean_heading_text(raw_heading)
        normalized = plain_text(raw_heading)
        if not normalized or (level == 1 and normalized in GENERIC_HEADINGS):
            return
        if level == 1 and normalized in METADATA_HEADINGS:
            append_blank(output)
            output.extend((f"**{normalized}**", ""))
            return
        transformed = transform_inline_markup(raw_heading, refs)
        mapped_level = max(2, min(level - refs.heading_base_level + 2, 6))
        output.append(f"{'#' * mapped_level} {transformed}")
        return

    line = re.sub(r"^(\s*)(\d+)\\\.[ \t]+", r"\1\2. ", line)
    line = re.sub(r"^(\s*)\\([-+*])[ \t]+", r"\1\2 ", line)
    output.append(transform_inline_markup(line, refs))


def emit_image_line(
    line: str,
    line_index: int,
    output: list[str],
    refs: DocumentReferences,
) -> None:
    matches = list(IMAGE_REFERENCE_RE.finditer(line))
    cursor = 0
    for occurrence, match in enumerate(matches):
        prefix = line[cursor : match.start()]
        if prefix.strip() and not prefix.lstrip().startswith(("図:", "図：")):
            emit_text_line(prefix.rstrip(), output, refs)

        append_blank(output)
        caption = refs.image_captions.get((line_index, occurrence))
        label = match.group(2)
        image_path = refs.image_paths.get(label)
        if image_path is None:
            raise ValueError(f"missing image definition for {label!r}")
        size = match.group(3)
        if size is not None:
            size = size.strip()
            if not IMAGE_SIZE_RE.match(size):
                raise ValueError(f"unsupported image size for {label!r}: {{{size}}}")
        if caption:
            caption_text = transform_inline_markup(caption.caption, refs).replace("]", r"\]")
            attributes = f"#{caption.anchor}" + (f" {size}" if size else "")
            output.append(f"![{caption_text}]({image_path}){{{attributes}}}")
        else:
            # Stays its own paragraph, so it becomes //indepimage, which takes a size.
            output.append(f"![]({image_path})" + (f"{{{size}}}" if size else ""))
        output.append("")
        cursor = match.end()

    suffix = line[cursor:]
    if suffix.strip() and not suffix.lstrip().startswith(("図:", "図：")):
        emit_text_line(suffix.rstrip(), output, refs)


def normalized_table_rows(rows: list[str], refs: DocumentReferences) -> list[str]:
    normalized: list[str] = []
    for row in rows:
        row = re.sub(r"\\([:-])", r"\1", row.rstrip())
        normalized.append(transform_inline_markup(row, refs))
    return normalized


def tsv_to_pipe_table(rows: list[str], refs: DocumentReferences) -> list[str]:
    cells = [
        [transform_inline_markup(cell.strip(), refs) for cell in row.split("\t")]
        for row in rows
    ]
    column_count = len(cells[0])
    if column_count < 2 or any(len(row) != column_count for row in cells):
        raise ValueError("inconsistent TSV table column count")
    output = [f"| {' | '.join(cells[0])} |"]
    output.append(f"| {' | '.join('---' for _ in range(column_count))} |")
    output.extend(f"| {' | '.join(row)} |" for row in cells[1:])
    return output


def emit_table_caption(
    element: LabeledElement, output: list[str], refs: DocumentReferences
) -> None:
    append_blank(output)
    caption = transform_inline_markup(element.caption, refs)
    tsize = f' tsize="{element.tsize}"' if element.tsize else ""
    output.extend(
        (
            f"[]{{#{element.anchor}{tsize}}}**表 {refs.chapter_number}.{element.number}: {caption}**",
            "",
        )
    )


def transform_document(
    source: Path, chapter_number: int = 1, drop_author_block: bool = False
) -> tuple[str, int]:
    lines = source.read_text(encoding="utf-8").splitlines()
    title, skipped_title_lines = derive_title(lines)
    title_source_index = max(skipped_title_lines)
    content_end = len(lines)
    for candidate_index in range(title_source_index + 1, len(lines)):
        heading = heading_at(lines[candidate_index])
        if not heading or heading[0] != 1:
            continue
        normalized = plain_text(heading[1])
        if (normalized in BODY_HEADINGS | {"テンプレート"}
                or any(m in normalized for m in TEMPLATE_GUIDE_MARKERS)):
            content_end = candidate_index
            break

    skipped_lines = set(skipped_title_lines)
    if drop_author_block:
        skipped_lines |= author_block_lines(lines, title_source_index + 1, content_end)

    refs = collect_references(
        lines, chapter_number, source.stem, content_end, skipped_lines
    )
    active_heading_levels: list[int] = []
    for heading_index in range(content_end):
        if heading_index in skipped_lines:
            continue
        heading = heading_at(lines[heading_index])
        if not heading:
            continue
        level, raw_heading = heading
        normalized = plain_text(raw_heading)
        if normalized and normalized not in GENERIC_HEADINGS and normalized not in METADATA_HEADINGS:
            active_heading_levels.append(level)
    if active_heading_levels:
        refs.heading_base_level = min(active_heading_levels)
    output = [f"# {title}", ""]
    code_blocks = 0
    code_mode: str | None = None
    box_mode = False
    index = 0

    while index < content_end:
        line = lines[index]

        if index in skipped_lines:
            index += 1
            continue

        if code_mode:
            fence = parse_markdown_fence(line)
            if (code_mode == "marker" and is_code_end(line)) or (
                code_mode == "fence" and fence is not None and not fence[1]
            ):
                output.extend(("```", ""))
                code_mode = None
                index += 1
                continue
            output.append(clean_code_line(line))
            index += 1
            continue

        if box_mode:
            if marker_text(line) == "BOX END":
                output.append("")
                box_mode = False
            else:
                transformed = transform_inline_markup(line, refs)
                output.append(f"> {transformed}" if line.strip() else ">")
            index += 1
            continue

        if marker_text(line) == "BOX START":
            append_blank(output)
            box_mode = True
            index += 1
            continue

        code_start = parse_code_start(line)
        if code_start:
            language, _caption, _identifier = code_start
            append_blank(output)
            element = refs.code_by_line.get(index)
            if element:
                caption = transform_inline_markup(element.caption, refs)
                output.extend(
                    (
                        f"[]{{#{element.anchor}}}**コード {chapter_number}.{element.number}: {caption}**",
                        "",
                    )
                )
            output.append(f"```{language}")
            code_mode = "marker"
            code_blocks += 1
            index += 1
            continue

        if is_code_end(line):
            raise ValueError(f"{source}: CODE END without CODE START at line {index + 1}")

        fence = parse_markdown_fence(line)
        if fence:
            _length, info = fence
            append_blank(output)
            output.append(f"```{info or 'text'}")
            code_mode = "fence"
            code_blocks += 1
            index += 1
            continue

        if index in refs.footnotes_by_line or index in refs.skipped_footnote_content_lines:
            index += 1
            continue

        if index in refs.image_definition_lines:
            index += 1
            continue

        if index in refs.figures_by_line and line.lstrip().startswith(("図:", "図：")):
            element = refs.figures_by_line[index]
            if element not in refs.image_captions.values():
                append_blank(output)
                caption = transform_inline_markup(element.caption, refs)
                output.extend(
                    (
                        f"[]{{#{element.anchor}}}*図 {chapter_number}.{element.number}: {caption}*",
                        "",
                    )
                )
            index += 1
            continue

        table_caption = refs.tables_by_line.get(index)
        if table_caption:
            emit_table_caption(table_caption, output, refs)
            cursor = index + 1
            while cursor < len(lines) and not lines[cursor].strip():
                cursor += 1
            if cursor < len(lines) and lines[cursor].lstrip().startswith("|"):
                rows: list[str] = []
                while cursor < len(lines) and lines[cursor].lstrip().startswith("|"):
                    rows.append(lines[cursor])
                    cursor += 1
                output.extend(normalized_table_rows(rows, refs))
                output.append("")
                index = cursor
                continue
            if cursor < len(lines) and "\t" in lines[cursor]:
                rows = []
                while cursor < len(lines) and "\t" in lines[cursor]:
                    rows.append(lines[cursor].rstrip())
                    cursor += 1
                output.extend(tsv_to_pipe_table(rows, refs))
                output.append("")
                index = cursor
                continue
            index += 1
            continue

        if line.lstrip().startswith("|"):
            rows = []
            cursor = index
            while cursor < len(lines) and lines[cursor].lstrip().startswith("|"):
                rows.append(lines[cursor])
                cursor += 1
            append_blank(output)
            output.extend(normalized_table_rows(rows, refs))
            output.append("")
            index = cursor
            continue

        if "\t" in line:
            rows = []
            cursor = index
            while cursor < len(lines) and "\t" in lines[cursor]:
                rows.append(lines[cursor].rstrip())
                cursor += 1
            append_blank(output)
            output.extend(tsv_to_pipe_table(rows, refs))
            output.append("")
            index = cursor
            continue

        if IMAGE_REFERENCE_RE.search(line):
            emit_image_line(line, index, output, refs)
            index += 1
            continue

        emit_text_line(line.rstrip(), output, refs)
        index += 1

    if code_mode:
        raise ValueError(f"{source}: code block was not closed")
    if box_mode:
        raise ValueError(f"{source}: BOX START without BOX END")

    footnotes: list[str] = []
    rendered_body = "\n".join(output)
    for label, content in refs.footnotes_by_line.values():
        if f"[^{label}]" in rendered_body:
            footnotes.append(f"[^{label}]: {transform_inline_markup(content, refs)}")
    if footnotes:
        append_blank(output)
        output.extend(footnotes)

    while output and output[-1] == "":
        output.pop()
    output.extend(("", ""))
    return "\n".join(output), code_blocks


def combine_documents(sources: Iterable[Path]) -> tuple[str, int]:
    documents: list[str] = []
    total_code_blocks = 0
    for chapter_number, source in enumerate(sources, 1):
        document, code_blocks = transform_document(source, chapter_number)
        documents.append(document)
        total_code_blocks += code_blocks
    return "".join(documents), total_code_blocks


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("sources", nargs="+", type=Path)
    args = parser.parse_args()

    combined, code_blocks = combine_documents(args.sources)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(combined, encoding="utf-8")
    print(f"Prepared {len(args.sources)} chapters and {code_blocks} code blocks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
