import importlib.util
import os
import shutil
import tempfile
import unittest
from unittest import mock
from pathlib import Path

SPEC = importlib.util.spec_from_file_location('prepare_review', Path(__file__).parents[1] / 'prepare-review-source.py')
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class ReviewSourceTests(unittest.TestCase):
    def source(self, text):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        path = Path(directory.name) / 'sample.md'
        path.write_text(text)
        return path

    def test_native_references_and_heading_numbers(self):
        source = self.source('# 記事\n\n## 3\\. 設定\n\n本文{{fig:setting}}。\n\n![][image1]\n図: 設定画面 {{fig:setting}}\n\n### 3.1 条件\n\n[image1]: assets/sample/image.png\n')
        prepared, _ = MODULE.prepare(source)
        self.assertIn('## 設定', prepared)
        self.assertIn('### 条件', prepared)
        self.assertIn('[assets/sample/image]{.img}', prepared)
        self.assertLess(prepared.index('![設定画面]'), prepared.index('### 条件'))

    @unittest.skipUnless(shutil.which('pandoc2review'), 'requires bundled pandoc2review')
    def test_image_size_becomes_scale_option(self):
        source = self.source('# 記事\n\n本文{{fig:setting}}。\n\n![][image1]{width=50%}\n図: 設定画面 {{fig:setting}}\n\n[image1]: assets/sample/image.png\n')
        result = MODULE.convert(source)
        self.assertIn('@<img>{assets/sample/image}', result)
        self.assertIn('//image[assets/sample/image][設定画面][scale=0.5]{', result)

    def test_font_fallback_keeps_urls_paths_and_ids(self):
        module = MODULE.translate_module()
        text = ('瀬戸 徳 @<href>{https://example.com/瀬戸,瀬戸} @<img>{図1_瀬戸}\n'
                '//image[assets/畳/瀬戸][図: 瀬戸]{\n//}\nhttps://example.com/畳 畳\n')
        result = module.font_fallback(text, 'ko')
        self.assertIn('瀨戶 徳', result)
        self.assertIn('@<href>{https://example.com/瀬戸,瀨戶}', result)
        self.assertIn('@<img>{図1_瀬戸}', result)
        self.assertIn('//image[assets/畳/瀬戸][図: 瀨戶]{', result)
        self.assertIn('https://example.com/畳 疊', result)

    def test_pdf_glyph_swap_applies_to_editorial_review_files_only_for_pdf(self):
        with tempfile.TemporaryDirectory() as root:
            source = Path(root) / 'editorial_ko_2026' / 'a.re'
            source.parent.mkdir()
            source.write_text('瀬戸 徳\n')
            with mock.patch.dict(os.environ, {'REVIEW_FONT_FALLBACK': '1'}):
                self.assertEqual(MODULE.pdf_font_fallback('瀬戸\n', source), '瀨戶\n')
            with mock.patch.dict(os.environ, {}, clear=True):
                self.assertEqual(MODULE.pdf_font_fallback('瀬戸\n', source), '瀬戸\n')

    def test_code_contents_are_not_normalized_as_headings_or_references(self):
        source = self.source('# 記事\n\nCODE START text | 例 {{list:example}}\n## 3. keep\n[link](#fig-missing)\nCODE END\n')
        prepared, _ = MODULE.prepare(source)
        self.assertIn('## 3. keep\n[link](#fig-missing)', prepared)

    def test_caption_attaches_to_native_list(self):
        result = MODULE.finish('REVIEWCAPTIONMARKER0END コード例\n\n//emlist[][cpp]{\nx = 1;\n//}\n', {'REVIEWCAPTIONMARKER0END': ('list', 'sample-code')})
        self.assertIn('//list[sample-code][コード例][cpp]{\nx = 1;', result)
        self.assertNotIn('REVIEWCAPTIONMARKER', result)

    def test_caption_attaches_to_table(self):
        result = MODULE.finish('REVIEWCAPTIONMARKER0END 比較\n\n//table{\nA\tB\n--------------\n1\t2\n//}\n', {'REVIEWCAPTIONMARKER0END': ('table', 'sample-table')})
        self.assertIn('//table[sample-table][比較]{', result)
        self.assertIn('1\t2', result)

    def test_table_column_widths_reach_review(self):
        prepared, markers = MODULE.prepare(self.source('# 記事\n\n表: 比較{{table:widths}}{{tsize:30, 20}}\n| A | B |\n|---|---|\n| 1 | 2 |\n'))
        self.assertNotIn('tsize', prepared)
        result = MODULE.finish(prepared.replace('| A | B |\n|---|---|\n| 1 | 2 |', '//table{\nA\tB\n--------------\n1\t2\n//}'), markers)
        self.assertIn('//tsize[|latex|30,20]\n//table[tbl-sample-widths][比較]{', result)

    def test_pagebreak_line_becomes_review_pagebreak(self):
        prepared, markers = MODULE.prepare(self.source('# 記事\n\n本文\n{{pagebreak}}\n### 次の項\n'))
        self.assertNotIn('{{pagebreak}}', prepared)
        self.assertIn('\n//pagebreak\n', MODULE.finish(prepared, markers))

    def test_enlargepage_line_lengthens_the_current_page(self):
        prepared, markers = MODULE.prepare(self.source('# 記事\n\n本文\n{{enlargepage}}\n### 次の項\n'))
        self.assertNotIn('{{enlargepage}}', prepared)
        self.assertIn('\n//embed[latex]{\n\\enlargethispage{\\baselineskip}\n//}\n', MODULE.finish(prepared, markers))

    def test_unattached_caption_is_an_error(self):
        with self.assertRaises(ValueError):
            MODULE.finish('REVIEWCAPTIONMARKER0END 比較\n本文\n', {'REVIEWCAPTIONMARKER0END': ('table', 'table')})

    def test_list_boundaries_are_preserved(self):
        prepared, _ = MODULE.prepare(self.source('# 記事\n\n本文\n1\\. 条件A\n2\\. 条件B\n次の本文\n'))
        self.assertIn('本文\n\n1. 条件A\n2. 条件B\n\n次の本文', prepared)

    @unittest.skipUnless(shutil.which('pandoc2review'), 'requires bundled pandoc2review')
    def test_real_conversion_preserves_references_code_and_footnotes(self):
        source = self.source('# 記事\n\n参照{{list:code}}、{{table:table}}、{{fig:figure}}、{{fn:note}}。\n\nCODE START cpp | 例 {{list:code}}\nif (x > 0) { x++; }\nCODE END\n\n表: 比較 {{table:table}}\n| A | B |\n|---|---|\n| 1 | 2 |\n\n![][image1]\n図: 画像 {{fig:figure}}\n\n### 次の項\n\n脚注: 説明 {{fn:note}}\n\n[image1]: assets/sample/image.png\n')
        result = MODULE.convert(source)
        self.assertIn('@<list>{lst-sample-code}', result)
        self.assertIn('@<table>{tbl-sample-table}', result)
        self.assertIn('@<img>{assets/sample/image}', result)
        self.assertIn('if (x > 0) { x++; }', result)
        self.assertIn('//footnote[', result)
        self.assertLess(result.index('//image['), result.index('次の項'))
        self.assertNotIn('{{', result)
        self.assertNotIn('REVIEWCAPTIONMARKER', result)


class AuthorProfileTests(unittest.TestCase):
    PROFILE = {'name': '山崎 雅大', 'romaji': 'Yamazaki Masahiro',
               'affiliation': '株式会社アプリボット', 'github': 'mao-test-h',
               'x': '@TEST_H_', 'web': 'blog.example.com'}

    def test_links_normalize_handles_and_urls(self):
        self.assertEqual(
            [('github', 'https://github.com/mao-test-h', 'mao-test-h'),
             ('x', 'https://x.com/TEST_H_', '@TEST_H_'),
             ('web', 'https://blog.example.com', 'blog.example.com')],
            MODULE.author_profile.links(self.PROFILE))
        self.assertEqual(
            [('github', 'https://github.com/ruccho', 'ruccho')],
            MODULE.author_profile.links({'github': 'https://github.com/ruccho'}))

    def test_latex_escapes_text_but_keeps_the_url_usable(self):
        latex = MODULE.author_profile.latex(self.PROFILE)
        self.assertIn(r'\reviewauthorname{山崎 雅大}', latex)
        self.assertIn(r'{@TEST\_H\_}', latex)
        self.assertIn('{https://x.com/TEST_H_}', latex)

    def test_optional_fields_emit_no_command(self):
        latex = MODULE.author_profile.latex({'name': '稲田 健人', 'affiliation': 'SGEコア技術本部'})
        self.assertNotIn(r'\reviewauthorromaji', latex)
        self.assertNotIn(r'\reviewauthorlinks', latex)
        self.assertIn(r'\reviewauthoraffiliation{SGEコア技術本部}', latex)

    def test_escaped_link_becomes_a_link(self):
        line = MODULE.unescape_links(r'参考: \[Managed code stripping\](https://example.com/a.html) です')
        self.assertEqual('参考: [Managed code stripping](https://example.com/a.html) です', line)

    def test_escaped_link_label_drops_inline_code(self):
        line = MODULE.unescape_links(r'\[\`IAnimationJob\`\](https://example.com/b.html) — 説明')
        self.assertEqual('[IAnimationJob](https://example.com/b.html) — 説明', line)

    def test_autolinked_escaped_link_keeps_the_trailing_text(self):
        line = MODULE.unescape_links(
            r'\[RequiredMemberAttribute\]([https://example.com/c.html)：説明](https://example.com/c.html\)：説明)')
        self.assertEqual('[RequiredMemberAttribute](https://example.com/c.html)：説明', line)

    def test_escaped_brackets_without_a_target_are_kept(self):
        line = r'環境変数 \[任意\] を設定します'
        self.assertEqual(line, MODULE.unescape_links(line))

    def test_bold_around_code_becomes_a_single_command(self):
        review = MODULE.flatten_nested_inline(
            '前半@<b>$太字と@<tt>{Animator.Update()}の続き$後半\n', 'sample.md')
        self.assertEqual(
            '前半@<b>{太字と}@<ttb>{Animator.Update()}@<b>{の続き}後半\n', review)

    def test_leading_code_emits_no_empty_bold(self):
        review = MODULE.flatten_nested_inline('@<b>$@<tt>{Stream}の読み書き$\n', 'sample.md')
        self.assertEqual('@<ttb>{Stream}@<b>{の読み書き}\n', review)

    def test_unnested_dollar_operand_is_left_alone(self):
        review = MODULE.flatten_nested_inline('@<tt>${id: 1001}$\n', 'sample.md')
        self.assertEqual('@<tt>${id: 1001}$\n', review)

    def test_code_block_contents_are_left_alone(self):
        source = '//emlist[][cs]{\n@<b>$x@<tt>{y}$\n//}\n'
        self.assertEqual(source, MODULE.flatten_nested_inline(source, 'sample.md'))

    def test_unsupported_nesting_fails_the_build(self):
        with self.assertRaises(ValueError):
            MODULE.flatten_nested_inline('@<b>$@<href>{https://example.com}$\n', 'sample.md')

    def test_labelled_link_prints_the_url_after_the_label(self):
        review = MODULE.print_link_urls('参考: @<href>{https://example.com/a.html,Managed code stripping}\n')
        self.assertEqual(review, '参考: Managed code stripping @<href>{https://example.com/a.html}\n')

    def test_list_item_link_prints_the_url_on_its_own_line(self):
        review = MODULE.print_link_urls(' * @<href>{https://example.com/a.html,Unity Manual}\n')
        self.assertEqual(review, ' * Unity Manual@<br>{}@<href>{https://example.com/a.html}\n')

    def test_label_equal_to_the_url_is_not_repeated(self):
        review = MODULE.print_link_urls('@<href>{https://example.com/,https://example.com/}\n')
        self.assertEqual(review, '@<href>{https://example.com/}\n')

    def test_label_may_contain_a_comma(self):
        review = MODULE.print_link_urls('@<href>{https://example.com/a,事例（Qiita, 2021）}。\n')
        self.assertEqual(review, '事例（Qiita, 2021） @<href>{https://example.com/a}。\n')

    def test_bare_url_is_wrapped_so_that_it_can_break(self):
        review = MODULE.print_link_urls('//footnote[fn1][仕様 https://example.com/spec]\n')
        self.assertEqual(review, '//footnote[fn1][仕様 @<href>{https://example.com/spec}]\n')

    def test_wrapping_stops_before_japanese_punctuation(self):
        review = MODULE.print_link_urls('//footnote[fn1][https://example.com/ca2000（参照日：2026年9月8日）]\n')
        self.assertEqual(
            review, '//footnote[fn1][@<href>{https://example.com/ca2000}（参照日：2026年9月8日）]\n')

    def test_angle_brackets_around_a_bare_url_are_dropped(self):
        review = MODULE.print_link_urls('記事<https://example.com/lto>。\n')
        self.assertEqual(review, '記事@<href>{https://example.com/lto}。\n')

    def test_sentence_final_period_stays_outside_the_link(self):
        review = MODULE.print_link_urls('See https://example.com/a.html.\n')
        self.assertEqual(review, 'See @<href>{https://example.com/a.html}.\n')

    def test_an_existing_link_is_not_wrapped_again(self):
        review = MODULE.print_link_urls('@<href>{https://example.com/a}\n')
        self.assertEqual(review, '@<href>{https://example.com/a}\n')

    def test_urls_in_code_blocks_are_left_alone(self):
        review = MODULE.print_link_urls('//emlist[][xml]{\n<!DOCTYPE plist SYSTEM "http://example.com/p.dtd">\n//}\nhttps://example.com/a\n')
        self.assertIn('SYSTEM "http://example.com/p.dtd"', review)
        self.assertIn('@<href>{https://example.com/a}', review)

    def test_profile_is_inserted_below_the_chapter_title(self):
        review = MODULE.insert_profile('= 記事名\n\n本文です。\n', self.PROFILE)
        lines = review.splitlines()
        self.assertEqual('= 記事名', lines[0])
        self.assertEqual('//embed[latex]{', lines[2])
        self.assertLess(review.index('//embed[html]{'), review.index('本文です。'))

    def test_every_catalog_article_has_a_profile(self):
        articles = MODULE.author_profile.load()
        catalog = (Path(__file__).parents[2] / 'articles' / 'catalog.yml').read_text()
        for manuscript in (Path(__file__).parents[2] / 'articles' / 'text_jp').glob('*.md'):
            if f'{manuscript.stem}.re' in catalog:
                self.assertIn(manuscript.stem, articles)
