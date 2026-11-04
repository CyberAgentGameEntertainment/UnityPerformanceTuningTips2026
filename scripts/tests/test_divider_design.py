import json
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DESIGN = ROOT / 'articles/images/divider/tuning-tips-2026-interior.pdf'
STRUCTURE = ROOT / 'articles/editorial-structure.json'
STYLE = ROOT / 'articles/sty/review-custom.sty'


class DividerDesignTests(unittest.TestCase):
    def setUp(self):
        self.data = DESIGN.read_bytes()

    def test_page_count_matches_structure(self):
        # 大扉1ページ＋部の数。ずれると部扉が一つずつずれるか、組み版の扉に戻る。
        pages = max(int(n) for n in re.findall(rb'/Type\s*/Pages\b[^>]*?/Count\s+(\d+)', self.data)
                    + re.findall(rb'/Count\s+(\d+)[^>]*?/Type\s*/Pages\b', self.data))
        parts = len(json.loads(STRUCTURE.read_text())['parts'])
        declared = int(re.search(r'\\providecommand\{\\reviewdividerdesignpages\}\{(\d+)\}', STYLE.read_text()).group(1))
        self.assertEqual(pages, parts + 1)
        self.assertEqual(pages, declared)


if __name__ == '__main__':
    unittest.main()
