#!/usr/bin/env python3
"""Unityプロジェクトから環境プロファイル（tuning-env.md）を自動生成する。

レビューエージェントはこのプロファイルと各ルールの「適用条件」タグを突き合わせ、
明確に不一致のルールをスキップする（unknown は安全側で照合対象のまま）。

Usage: python3 detect-env.py <unity-project-or-repo-root> [output-path]
（このリポジトリでは tools/、配布パッケージでは scripts/ に配置される）
"""
import glob
import json
import os
import re
import sys


def find_first(root, pattern):
    hits = glob.glob(os.path.join(root, pattern)) or glob.glob(os.path.join(root, "*", pattern)) \
        or glob.glob(os.path.join(root, "*", "*", pattern))
    return hits[0] if hits else None


def detect(root):
    env = {
        "unity_version": "unknown",
        "render_pipeline": "unknown",   # urp / hdrp / builtin
        "scripting_backend": "unknown", # il2cpp / mono
        "addressables": "unknown",
        "vat": "unknown",               # SideFX VAT（自動検出困難、手動記入推奨）
        "platforms": "unknown",         # ios, android 等（必ず手動記入。自動推定しない）
        "platforms_hint": "なし",
    }

    pv = find_first(root, os.path.join("ProjectSettings", "ProjectVersion.txt"))
    if pv:
        m = re.search(r"m_EditorVersion:\s*(\S+)", open(pv, encoding="utf-8").read())
        if m:
            env["unity_version"] = m.group(1)

    manifest = find_first(root, os.path.join("Packages", "manifest.json"))
    deps = {}
    if manifest:
        deps = json.load(open(manifest, encoding="utf-8")).get("dependencies", {})
        env["addressables"] = "true" if "com.unity.addressables" in deps else "false"

    # レンダーパイプラインはパッケージ存在ではなくGraphicsSettingsのRP Asset参照で判定する
    # （パッケージが依存で入っているだけのケースを誤検出しないため）
    gs = find_first(root, os.path.join("ProjectSettings", "GraphicsSettings.asset"))
    srp_active = None
    if gs:
        try:
            text = open(gs, encoding="utf-8", errors="ignore").read()
            m = re.search(r"m_CustomRenderPipeline:\s*\{fileID:\s*(-?\d+)", text)
            if m:
                srp_active = m.group(1) != "0"
        except OSError:
            pass
    # アクティブなRPはGraphicsSettingsとQualitySettingsの組み合わせで決まる。
    # GraphicsSettings側が空でも、品質レベル別のRP Asset上書きがあればSRP有効
    if srp_active is not True:
        qs = find_first(root, os.path.join("ProjectSettings", "QualitySettings.asset"))
        if qs:
            try:
                qtext = open(qs, encoding="utf-8", errors="ignore").read()
                ids = re.findall(r"customRenderPipeline:\s*\{fileID:\s*(-?\d+)", qtext)
                if any(i != "0" for i in ids):
                    srp_active = True
            except OSError:
                pass
    if srp_active is False:
        env["render_pipeline"] = "builtin"
    elif srp_active is True or deps:
        if "com.unity.render-pipelines.universal" in deps:
            env["render_pipeline"] = "urp"
        elif "com.unity.render-pipelines.high-definition" in deps:
            env["render_pipeline"] = "hdrp"
        elif srp_active is True:
            env["render_pipeline"] = "unknown"  # SRP有効だが種別不明
        else:
            env["render_pipeline"] = "builtin"

    ps = find_first(root, os.path.join("ProjectSettings", "ProjectSettings.asset"))
    if ps:
        try:
            text = open(ps, encoding="utf-8", errors="ignore").read()
            m = re.search(r"scriptingBackend:\s*\n((?:\s+\w+:\s*\d+\s*\n)+)", text)
            if m:
                # プラットフォーム別設定のため、全プラットフォーム一致時のみ確定。混在はunknown（安全側=照合継続）
                values = set(re.findall(r":\s*(\d+)", m.group(1)))
                if values == {"1"}:
                    env["scripting_backend"] = "il2cpp"
                elif values == {"0"}:
                    env["scripting_backend"] = "mono"
                else:
                    env["scripting_backend"] = "unknown"
            # ProjectSettings内の文字列出現は出荷対象の証明にならないため、参考ヒントに留めてunknownを維持（手動記入が正）
            plats = []
            if re.search(r"iPhone", text):
                plats.append("ios")
            if re.search(r"Android", text):
                plats.append("android")
            if plats:
                env["platforms_hint"] = ", ".join(plats)
        except OSError:
            pass

    return env


TEMPLATE = """# チューニング環境プロファイル（tuning-env.md）

> detect-env.py による自動検出結果。誤り・不足は手動で修正してよい（このファイルが正）。
> レビューエージェントはルールの「適用条件」タグとこのプロファイルを突き合わせ、
> 明確に条件を満たさないルールをスキップする。`unknown` の項目は安全側（照合対象のまま）で扱われる。

| キー | 値 | 備考 |
|------|-----|------|
| unity_version | {unity_version} | ProjectVersion.txt から検出 |
| render_pipeline | {render_pipeline} | urp / hdrp / builtin |
| scripting_backend | {scripting_backend} | il2cpp / mono |
| addressables | {addressables} | manifest.json から検出 |
| vat | {vat} | SideFX VAT 使用有無（手動記入: true / false） |
| platforms | {platforms} | 出荷対象は自動判定しない。**手動記入必須**（例: ios, android）。参考ヒント: {platforms_hint}（ProjectSettings内の記述より。出荷対象の証明にはならない） |

## タグの判定規則

- `unity>=X`: unity_version と比較（例: 6000.3.15f1 は unity>=6000.3 を満たす）
- `urp` / `hdrp`: render_pipeline が一致すること。`srp` は urp または hdrp
- `il2cpp`: scripting_backend = il2cpp
- `ios` / `android`: platforms に含まれること
- `addressables` / `vat`: 各キーが true
- カンマ区切りは AND、`|` 区切りは OR（例: `ios|android` = いずれか）
- 値が `unknown` のキーに依存するタグは**スキップしない**（安全側）
"""


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    env = detect(root)
    out = TEMPLATE.format(**env)
    if len(sys.argv) > 2:
        with open(sys.argv[2], "w", encoding="utf-8") as f:
            f.write(out)
        print(f"wrote {sys.argv[2]}")
        for k, v in env.items():
            print(f"  {k}: {v}")
    else:
        print(out)


if __name__ == "__main__":
    main()
