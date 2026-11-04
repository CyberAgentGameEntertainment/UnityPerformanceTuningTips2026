import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).parents[1] / "prepare-google-drive-markdown-book.py"
SPEC = importlib.util.spec_from_file_location("prepare_book", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


class PrepareGoogleDriveMarkdownBookTests(unittest.TestCase):
    def transform(self, markdown: str, drop_author_block: bool = False) -> tuple[str, int]:
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "chapter.md"
            source.write_text(markdown, encoding="utf-8")
            return MODULE.transform_document(source, drop_author_block=drop_author_block)

    def body(self, markdown: str) -> str:
        output, _ = self.transform(markdown, drop_author_block=True)
        return output

    def test_file_title_becomes_only_level_one_heading(self):
        output, _ = self.transform(
            "# **記事名**\n\n著者名\n\n# **はじめに**\n\n## 詳細\n"
        )
        self.assertTrue(output.startswith("# 記事名\n"))
        self.assertIn("## **はじめに**", output)
        self.assertIn("### 詳細", output)
        self.assertEqual(1, sum(line.startswith("# ") for line in output.splitlines()))

    def test_body_marker_selects_the_following_article_heading(self):
        output, _ = self.transform(
            "# スタイルガイド\n\n"
            "# 原稿作成テンプレートのご案内\n\n"
            "説明\n\n# 本文\n\n# 本当の記事名\n\n# はじめに\n"
        )
        self.assertTrue(output.startswith("# 本当の記事名\n"))
        self.assertNotIn("# スタイルガイド", output)
        self.assertNotIn("# 本文", output)
        self.assertNotIn("原稿作成テンプレートのご案内", output)
        self.assertNotIn("説明", output)
        self.assertIn("## はじめに", output)

    def test_article_title_label_uses_following_paragraph(self):
        output, _ = self.transform(
            "# 記事タイトル\n\n実際の記事名\n\n# 名前・所属\n\n著者名\n\n# はじめに\n"
        )
        self.assertTrue(output.startswith("# 実際の記事名\n"))
        self.assertIn("**名前・所属**", output)
        self.assertIn("## はじめに", output)

    def test_labeled_author_metadata_is_removed(self):
        output = self.body(
            "# **記事名**\n\n"
            "名前 / ローマ字：財津 大睦 / Zaitsu Hiromu  \n"
            "所属：株式会社サムザップ  \n"
            "Xアカウント / Webサイト / GitHubアカウント（任意）：@RomGameclient\n\n"
            "本文です。\n"
        )
        self.assertTrue(output.startswith("# 記事名\n"))
        for removed in ("名前", "ローマ字", "所属", "Xアカウント", "RomGameclient"):
            self.assertNotIn(removed, output)
        self.assertIn("本文です。", output)

    def test_unlabeled_author_line_and_rule_are_removed(self):
        output = self.body(
            "# **記事名**\n\n"
            "向井 祐一郎 / Mukai Yuichiro / 所属: 株式会社アプリボット  \n"
            "yucchiy\\_  / blog.yucchiy.com / yucchiy  \n"
            "\\---\n\n"
            "本文です。\n"
        )
        self.assertNotIn("Mukai", output)
        self.assertNotIn("yucchiy", output)
        self.assertNotIn("---", output)
        self.assertIn("本文です。", output)

    def test_metadata_headings_and_their_contents_are_removed(self):
        output = self.body(
            "# 記事タイトル\n\n実際の記事名\n\n"
            "# 名前・所属\n\n永留遼平 / Nagatome Ryouhei / 株式会社X\n\n"
            "# GitHubアカウント\n\nhttps://github.com/rngtm\n\n"
            "# Xアカウント\n\nhttps://x.com/rn49rn49\n\n"
            "# はじめに\n\n本文です。\n"
        )
        self.assertTrue(output.startswith("# 実際の記事名\n"))
        for removed in ("名前・所属", "GitHub", "rngtm", "rn49rn49", "Nagatome"):
            self.assertNotIn(removed, output)
        self.assertIn("## はじめに", output)
        self.assertIn("本文です。", output)

    def test_bullet_author_line_after_bullet_title_is_removed(self):
        output = self.body(
            "* GraphicsStateCollectionによるシェーダーウォームアップ  \n"
            "* 天野 択郎/Amano Takuro/SGEコア技術本部\n\n"
            "本文です。\n"
        )
        self.assertTrue(output.startswith("# GraphicsStateCollectionによるシェーダーウォームアップ\n"))
        self.assertNotIn("Amano", output)
        self.assertIn("本文です。", output)

    def test_body_paragraph_below_the_title_is_kept(self):
        output = self.body(
            "# **記事名**\n\n"
            "名前：塩塚 勇気\n\n"
            "UIシステムの使い分け / 選択は、負荷構造を決めます。\n"
        )
        self.assertNotIn("塩塚", output)
        self.assertIn("UIシステムの使い分け / 選択は、負荷構造を決めます。", output)

    def test_author_metadata_is_kept_without_a_profile(self):
        output, _ = self.transform("# **記事名**\n\n所属：株式会社X\n\n本文です。\n")
        self.assertIn("所属：株式会社X", output)

    def test_code_markers_and_google_escapes_become_fenced_code(self):
        output, count = self.transform(
            "タイトル：コード記事\n\n"
            "CODE START csharp | 利用例 {{list:sample}}  \n"
            "var value \\= items\\[0\\];  \n"
            "Debug.Log(\\\"quoted\\\");  \n"
            "CODE END  \n"
        )
        self.assertEqual(1, count)
        self.assertIn("**コード 1.1: 利用例**", output)
        self.assertIn("```csharp\nvar value = items[0];", output)
        self.assertIn(r'Debug.Log(\"quoted\");', output)
        self.assertNotIn("CODE START", output)
        self.assertNotIn("CODE END", output)

    def test_inline_code_wrapped_export_is_unwrapped(self):
        output, _ = self.transform(
            "# 記事名\n\n"
            "`CODE START csharp`  \n"
            "    `var value = 1;`  \n"
            "`CODE END`  \n"
        )
        self.assertIn("```csharp\n    var value = 1;\n```", output)

    def test_leading_bullet_is_used_when_document_has_no_title_heading(self):
        output, _ = self.transform(
            "* 箇条書きの章タイトル\n* 著者名\n\n# \n\n# 最初の節\n"
        )
        self.assertTrue(output.startswith("# 箇条書きの章タイトル\n"))
        self.assertNotIn("\n#\n", output)
        self.assertIn("## 最初の節", output)

    def test_escaped_inline_and_fenced_code_are_restored(self):
        output, count = self.transform(
            "# 記事名\n\n"
            "本文の\\`value\\[0\\]\\`です。\n\n"
            "\\`\\`\\`\n"
            "flow \\= start \\> end\n"
            "\\`\\`\\`\n"
        )
        self.assertIn("本文の`value[0]`です。", output)
        self.assertIn("```text\nflow = start > end\n```", output)
        self.assertEqual(1, count)

    def test_image_and_caption_become_a_labeled_figure(self):
        output, _ = self.transform(
            "# 記事名\n\n本文（{{fig:sample}}）。\n"
            "![][image1]  \n"
            "図: サンプル画像 {{fig:sample}}\n\n"
            "[image1]: assets/image1.png\n"
        )
        self.assertIn("[図 1.1](#fig-chapter-sample)", output)
        self.assertIn("![サンプル画像](assets/image1.png){#fig-chapter-sample}", output)
        self.assertNotIn("図: サンプル画像", output)

    def test_image_size_is_carried_to_the_figure(self):
        output, _ = self.transform(
            "# 記事名\n\n本文（{{fig:sample}}）。\n"
            "![][image1]{width=50%}  \n"
            "図: サンプル画像 {{fig:sample}}\n\n"
            "[image1]: assets/image1.png\n"
        )
        self.assertIn("![サンプル画像](assets/image1.png){#fig-chapter-sample width=50%}", output)
        self.assertNotIn("{width=50%}", output)

    def test_unsupported_image_size_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "unsupported image size"):
            self.transform(
                "# 記事名\n\n![][image1]{width=300px}\n"
                "図: サンプル画像 {{fig:sample}}\n\n[image1]: assets/image1.png\n"
            )

    def test_uncaptioned_image_keeps_its_size(self):
        output, _ = self.transform("# 記事名\n\n![][image1]{scale=0.5}\n\n[image1]: assets/image1.png\n")
        self.assertIn("![](assets/image1.png){scale=0.5}", output)

    def test_escaped_and_tsv_tables_are_normalized(self):
        escaped, _ = self.transform(
            "# 記事名\n\n本文 {{table:comparison}}\n\n"
            "表: 比較 {{table:comparison}}\n"
            "| A | B |\n"
            "| \\--- | \\---: |\n"
            "| 1 | 2 |\n"
        )
        self.assertIn("**表 1.1: 比較**", escaped)
        self.assertIn("| --- | ---: |", escaped)
        self.assertIn("[表 1.1](#tbl-chapter-comparison)", escaped)

        tsv, _ = self.transform(
            "# 記事名\n\n表: 分類 {{table:kinds}}\n"
            "種類\t説明\nA\t最初\nB\t次\n"
        )
        self.assertIn("| 種類 | 説明 |\n| --- | --- |", tsv)

    def test_footnote_definition_is_native_markdown(self):
        output, _ = self.transform(
            "# 記事名\n\n本文{{fn:source}}。\n\n# 脚注\n\n"
            "{{fn:source}}\nhttps://example.com/source\n"
        )
        self.assertIn("本文[^fn-chapter-source]。", output)
        self.assertIn("[^fn-chapter-source]: https://example.com/source", output)
        self.assertNotIn("{{fn:source}}", output)

    def test_empty_labeled_footnote_uses_following_markdown_link(self):
        output, _ = self.transform(
            "# 記事名\n\n本文{{fn:source}}。\n\n## 脚注\n\n"
            "脚注: {{fn:source}}   \n"
            "[https://example.com/source](https://example.com/source)  \n"
            "\n## 次の節\n"
        )
        self.assertIn(
            "[^fn-chapter-source]: "
            "[https://example.com/source](https://example.com/source)",
            output,
        )
        self.assertEqual(
            1,
            output.count(
                "[https://example.com/source](https://example.com/source)"
            ),
        )
        self.assertIn("## 次の節", output)
        self.assertNotIn("[^fn-chapter-source]: \n", output)

    def test_trailing_template_is_removed(self):
        output, _ = self.transform(
            "# 記事名\n\n本文です。\n\n# テンプレート\n\n執筆ルールです。\n"
        )
        self.assertIn("本文です。", output)
        self.assertNotIn("テンプレート", output)
        self.assertNotIn("執筆ルール", output)

    def test_level_two_sections_do_not_gain_an_empty_numbering_level(self):
        output, _ = self.transform(
            "# 記事名\n\n## 最初の節\n\n### 詳細\n"
        )
        self.assertIn("## 最初の節", output)
        self.assertIn("### 詳細", output)
        self.assertNotIn("#### 詳細", output)

    def test_escaped_heading_marker_inside_heading_is_removed(self):
        output, _ = self.transform(
            "# 記事名\n\n### **\\#\\#\\# Managed Stripping Level**\n"
        )
        self.assertIn("## **Managed Stripping Level**", output)
        self.assertNotIn("\\#\\#\\#", output)

    def test_escaped_ordered_list_markers_become_a_markdown_list(self):
        output, _ = self.transform(
            "# 記事名\n\n1\\. 最初  \n2\\. 次\n"
        )
        self.assertIn("1. 最初", output)
        self.assertIn("2. 次", output)
        self.assertNotIn("1\\.", output)


if __name__ == "__main__":
    unittest.main()
