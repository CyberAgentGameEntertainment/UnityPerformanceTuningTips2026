#!/usr/bin/env python3
"""チェックリストの grep_pattern を全行機械実行する一次スキャナ。

レビューエージェントの一次スキャン（SKILL.md 手順4）を決定的に行う。
LLM にgrepを1本ずつ実行させる方式の弱点（行の読み飛ばし・正規表現方言の差・
Markdownエスケープの復元ミス）を除去し、Coverage の数値を機械生成する。

- 通常行: パターンにヒットしたファイル:行を列挙する
- ABSENT 行: 全対象でパターンが見つからない場合に「該当疑い」として報告する
- MANUAL 行: 実行対象外。精読が必要な行として列挙する
- --env で tuning-env.md を渡すと適用条件タグを評価し、明確に条件を
  満たさない行をスキップする（unknown は安全側＝照合対象のまま）

出力はそのままレポートの素材になる Markdown。ヒットの採用・誤検出除外
（二次精査）はエージェントが詳細ルールを読んで行う。

Usage:
  python3 scan.py <checklist.md> [checklist2.md ...] --targets <file|dir> ... [--env tuning-env.md]
（このリポジトリでは tools/、配布パッケージでは scripts/ に配置される。
 配布パッケージ内での例: python3 scripts/scan.py references/checklist-code.md \\
   references/checklist-asset.md --env tuning-env.md --targets Assets/ ProjectSettings/）
"""
import os
import re
import sys

MAX_FILE_BYTES = 2 * 1024 * 1024
MAX_LINES_PER_RULE_FILE = 5
SKIP_DIRS = {".git", "Library", "Temp", "obj", "Logs", "node_modules"}


def parse_checklist(path):
    rules = []
    for line in open(path, encoding="utf-8"):
        if not line.startswith("| "):
            continue
        cells = [c.strip() for c in re.split(r"(?<!\\)\|", line)][1:-1]
        if len(cells) != 6 or cells[0] in ("ID", ""):
            continue
        rid, level, cond, pat, name, detail = cells
        pat = pat.strip("`")
        kind = "grep"
        if pat.startswith("MANUAL") or "手動確認" in pat:
            kind = "manual"
            pat = ""
        elif pat.startswith("ABSENT"):
            kind = "absent"
            pat = re.sub(r"^ABSENT:?\s*", "", pat).strip("`")
        pat = pat.replace("\\|", "|")
        cond = cond.replace("\\|", "|")
        rules.append({"id": rid, "level": level, "cond": cond, "pattern": pat,
                      "name": name, "detail": detail, "kind": kind,
                      "src": os.path.basename(path)})
    return rules


def parse_env(path):
    env = {}
    for line in open(path, encoding="utf-8"):
        m = re.match(r"^\|\s*([a-z_]+)\s*\|\s*([^|]+?)\s*\|", line)
        if m and m.group(1) != "キー":
            env[m.group(1)] = m.group(2).strip()
    return env


def version_tuple(s):
    return tuple(int(x) for x in re.findall(r"\d+", s)[:3])


def eval_tag(tag, env):
    """タグを評価する。True=満たす / False=明確に満たさない / None=不明（安全側で照合）"""
    tag = tag.strip()
    if tag in ("", "-"):
        return True
    m = re.match(r"unity>=([\d.]+)", tag)
    if m:
        v = env.get("unity_version", "unknown")
        if v == "unknown" or not re.search(r"\d", v):
            return None
        return version_tuple(v) >= version_tuple(m.group(1))
    def known(key):
        val = env.get(key, "unknown")
        return None if val in ("unknown", "") else val
    if tag in ("urp", "hdrp", "builtin"):
        v = known("render_pipeline")
        return None if v is None else v == tag
    if tag == "srp":
        v = known("render_pipeline")
        return None if v is None else v in ("urp", "hdrp")
    if tag in ("il2cpp", "mono"):
        v = known("scripting_backend")
        return None if v is None else v == tag
    if tag in ("ios", "android"):
        v = known("platforms")
        return None if v is None else tag in re.split(r"[,\s/]+", v.lower())
    if tag in ("addressables", "vat"):
        v = known(tag)
        return None if v is None else v.lower() not in ("false", "no", "none", "0")
    return None  # 未知のタグは安全側


def rule_applicable(cond, env):
    """カンマ=AND、|=OR。いずれかのANDグループが明確にFalseならスキップ可"""
    for group in cond.split(","):
        results = [eval_tag(t, env) for t in group.split("|")]
        if all(r is False for r in results):
            return False
    return True


def collect_files(targets):
    files = []
    for t in targets:
        if os.path.isfile(t):
            files.append(t)
        elif os.path.isdir(t):
            for root, dirs, names in os.walk(t):
                dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
                files.extend(os.path.join(root, n) for n in names)
        else:
            print(f"warning: 対象が見つからない: {t}", file=sys.stderr)
    out = []
    for f in sorted(set(files)):
        try:
            if os.path.getsize(f) > MAX_FILE_BYTES:
                continue
            with open(f, "rb") as fp:
                head = fp.read(8000)
            if b"\x00" in head:
                continue  # バイナリ
            out.append(f)
        except OSError:
            continue
    return out


def main():
    args = sys.argv[1:]
    if not args or "--targets" not in args:
        print(__doc__)
        sys.exit(1)
    env_path = None
    if "--env" in args:
        i = args.index("--env")
        env_path = args[i + 1]
        args = args[:i] + args[i + 2:]
    i = args.index("--targets")
    checklists, targets = args[:i], args[i + 1:]

    rules, seen = [], set()
    for c in checklists:
        for r in parse_checklist(c):
            key = (r["id"], r["kind"], r["pattern"])
            if key not in seen:
                seen.add(key)
                rules.append(r)
    env = parse_env(env_path) if env_path else {}
    files = collect_files(targets)
    contents = {}
    for f in files:
        with open(f, encoding="utf-8", errors="ignore") as fp:
            contents[f] = fp.read()

    hits, absents, manuals, skipped, errors = [], [], [], [], []
    scanned = 0
    for r in rules:
        if env and not rule_applicable(r["cond"], env):
            skipped.append(r)
            continue
        if r["kind"] == "manual":
            manuals.append(r)
            continue
        scanned += 1
        try:
            rx = re.compile(r["pattern"])
        except re.error as e:
            errors.append((r, str(e)))
            continue
        matches = []
        for f, content in contents.items():
            if not rx.search(content):
                continue
            lines = [i + 1 for i, ln in enumerate(content.splitlines()) if rx.search(ln)]
            matches.append((f, lines))
        if r["kind"] == "absent":
            if not matches:
                absents.append(r)
        elif matches:
            hits.append((r, matches))

    print("# 一次スキャン結果（scan.py による機械実行）\n")
    print(f"対象ファイル: {len(files)} / チェックリスト行（重複除去後）: {len(rules)}\n")
    print("## grep ヒット（二次精査対象。詳細ファイルの検出条件で誤検出を除外すること）\n")
    if not hits:
        print("なし\n")
    for r, matches in hits:
        print(f"### {r['id']} [{r['level']}] {r['name']}（詳細: {r['detail']}）")
        for f, lines in matches:
            shown = ", ".join(map(str, lines[:MAX_LINES_PER_RULE_FILE]))
            more = f" ほか計{len(lines)}行" if len(lines) > MAX_LINES_PER_RULE_FILE else ""
            print(f"- {f}: L{shown}{more}" if lines else f"- {f}: (複数行パターン一致)")
        print()
    print("## ABSENT 該当疑い（パターン不在。詳細を読み適用条件を確認すること）\n")
    print("\n".join(f"- {r['id']} [{r['level']}] {r['name']}（詳細: {r['detail']}）" for r in absents) or "なし")
    print("\n## MANUAL（grep 不能。対象種別に該当する場合は精読すること）\n")
    print("\n".join(f"- {r['id']} {r['name']}（詳細: {r['detail']}）" for r in manuals) or "なし")
    print("\n## Coverage（機械生成）\n")
    print(f"- チェックリスト行数（重複除去後）: {len(rules)}")
    print(f"- 環境対象外スキップ: {len(skipped)}件"
          + (f"（{', '.join(r['id'] for r in skipped)}）" if skipped else ""))
    print(f"- grep実行: {scanned}件 / ヒット: {len(hits)}件 / ABSENT該当疑い: {len(absents)}件 / MANUAL: {len(manuals)}件")
    if errors:
        print(f"- 正規表現エラー: {len(errors)}件 " + ", ".join(r["id"] for r, _ in errors))
    if not env_path:
        print("- 環境フィルタ: 未適用（--env 未指定。全ルール照合）")


if __name__ == "__main__":
    main()
