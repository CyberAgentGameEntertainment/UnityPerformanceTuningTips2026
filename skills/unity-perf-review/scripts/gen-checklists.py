#!/usr/bin/env python3
"""rules-*.md から対象別チェックリスト（checklist-code / checklist-asset / checklist-profile）を自動生成する。

チェックリストはレビューエージェントの一次スキャン用（1ルール1行）。
詳細（検出パターン・理由・対処）はヒット時に元のカテゴリファイルを参照する。

Usage: python3 tools/gen-checklists.py [rules_dir]
（配布パッケージ内では python3 scripts/gen-checklists.py references/ で再生成できる）
"""
import glob
import os
import re
import sys

RULES_DIR = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "rules")

CODE_KEYS = ("コードレビュー", "設計レビュー", "シェーダーレビュー")
ASSET_KEYS = (
    "アセット設定", "アセット構成", "シーン構成", "ProjectSettings",
    "ビルド設定", "CI", "プロジェクト構成", "URP設定", "設定検査",
)
PROFILE_KEYS = ("プロファイル解析",)

HEADER = """# {title}

> このファイルは gen-checklists.py（配布パッケージでは scripts/ に同梱）により rules-*.md から自動生成される。
> 直接編集せず、ルールを修正・追加したら再生成すること（例: python3 scripts/gen-checklists.py references/）。
> 用途: レビュー一次スキャン。全行の grep を機械実行し、ヒットしたルールのみ元ファイルの詳細（検出パターン・理由・対処）を読む。
> `MANUAL` は grep 不能ルール。対象ファイル種別に該当する場合は精読で確認する。
> `ABSENT` は不在検出ルール。grep でパターンが**見つからない場合**に該当を疑い、詳細を読んで適用条件を確認する（ヒットがあれば通常は非該当）。
> 適用条件列: tuning-env.md の環境プロファイルと突き合わせ、明確に条件を満たさない行はスキップしてよい
> （スキップ数はCoverageの「環境対象外」に計上）。カンマ=AND、`|`=OR。tuning-env.md が無い/unknown の場合はスキップしない。
> 注意: `-flto` など `-` で始まるパターンをシェルの grep に渡すときは `grep -e 'パターン'` を使う（引数と誤解釈される）。

| ID | レベル | 適用条件 | grep_pattern | ルール名 | 詳細 |
|----|--------|----------|--------------|----------|------|
"""


def parse_rules(path):
    rules = []
    cur = None
    for line in open(path, encoding="utf-8"):
        m = re.match(r"^### (\S+) (.+)$", line)
        if m:
            cur = {"id": m.group(1), "title": m.group(2).strip(), "level": "?",
                   "target": "", "grep": None, "absent": False, "requires": "-",
                   "file": os.path.basename(path)}
            rules.append(cur)
            continue
        if cur is None:
            continue
        m = re.match(r"^- \*\*レベル\*\*: (.+)$", line)
        if m:
            lv = re.search(r"(Critical|Warning|Info)", m.group(1))
            cur["level"] = lv.group(1) if lv else m.group(1).strip()
            if "Critical" in m.group(1) and cur["level"] != "Critical":
                cur["level"] += "+"  # 条件付きCritical
            continue
        m = re.match(r"^- \*\*対象\*\*: (.+)$", line)
        if m:
            cur["target"] = m.group(1).strip()
            continue
        m = re.match(r"^- \*\*適用条件\*\*: (.+)$", line)
        if m:
            cur["requires"] = m.group(1).strip()
            continue
        m = re.match(r"^- \*\*grep_pattern\*\*: (.+)$", line)
        if m:
            value = m.group(1).strip()
            # 「(手動確認」始まり（「(手動確認)」「(手動確認 — ...)」とも）は説明中にコード片があってもMANUAL扱い（(半機械)はgrep併用可）
            if value.startswith("(手動確認"):
                cur["grep"] = None
            elif value.startswith("(不在検出)"):
                # パターンの不在が指摘条件（ヒット=非該当）。チェックリストでは ABSENT を付す
                g = re.search(r"`([^`]+)`", value)
                cur["grep"] = g.group(1) if g else None
                cur["absent"] = cur["grep"] is not None
            else:
                g = re.search(r"`([^`]+)`", value)
                cur["grep"] = g.group(1) if g else None
    return rules


def sheet_membership(rule):
    t = rule["target"]
    sheets = set()
    if any(k in t for k in CODE_KEYS):
        sheets.add("code")
    if any(k in t for k in ASSET_KEYS):
        sheets.add("asset")
    if any(k in t for k in PROFILE_KEYS):
        sheets.add("profile")
    if not sheets:
        sheets.update(("code", "asset"))
        print(f"  [warn] 対象を判定できず両方へ: {rule['id']} 対象='{t}'")
    return sheets


def esc(text):
    return text.replace("|", "\\|")


def main():
    all_rules = []
    for path in sorted(glob.glob(os.path.join(RULES_DIR, "rules-*.md"))):
        if os.path.basename(path).startswith("rules-overrides"):
            continue
        all_rules.extend(parse_rules(path))

    sheets = {"code": [], "asset": [], "profile": []}
    for r in all_rules:
        for s in sheet_membership(r):
            sheets[s].append(r)

    titles = {
        "code": "レビューチェックリスト: コード（C#/シェーダーコード）",
        "asset": "レビューチェックリスト: アセット・設定（.meta/Prefab/ProjectSettings/ビルド）",
        "profile": "レビューチェックリスト: プロファイル解析",
    }
    for name, rules in sheets.items():
        out = HEADER.format(title=titles[name])
        for r in rules:
            if not r["grep"]:
                grep = "MANUAL"
            elif r["absent"]:
                grep = f"ABSENT `{esc(r['grep'])}`"
            else:
                grep = f"`{esc(r['grep'])}`"
            out += f"| {r['id']} | {r['level']} | {esc(r['requires'])} | {grep} | {esc(r['title'])} | {r['file']} |\n"
        dst = os.path.join(RULES_DIR, f"checklist-{name}.md")
        with open(dst, "w", encoding="utf-8") as f:
            f.write(out)
        print(f"wrote {dst} ({len(rules)} rules, {os.path.getsize(dst)} bytes)")

    total = len(all_rules)
    print(f"total rules parsed: {total}")
    missing_grep = [r["id"] for r in all_rules if not r["grep"]]
    print(f"MANUAL rules: {len(missing_grep)}")


if __name__ == "__main__":
    main()
