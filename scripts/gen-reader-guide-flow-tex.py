#!/usr/bin/env python3
"""課題別逆引きガイドの流れ図（日本語TikZ）から、翻訳版の元データ reader_guide_flow_<lang>.tex を生成する。

日本語の文字列を言語別の訳語に置き換え、ko/zh はフォント指定を Noto Sans CJK に差し替える。
訳語は各言語の editorial-reader-guide.re と translation2026 の章タイトル訳に揃える。
"""
import pathlib
import sys

FIG = pathlib.Path(__file__).resolve().parent.parent / 'articles/images_jp/editorial-reader-guide'

# 日本語 -> {lang: 訳}。長い文字列から先に置換する。
T = {
    '計測する': dict(en='Measure', ko='계측하기', **{'zh-hans': '测量', 'zh-hant': '測量'}),
    '進め方・基本ツール：前作 第1章・第3章': dict(
        en='Approach and basic tools: previous book, Chapters 1 and 3',
        ko='진행 방법·기본 도구: 전작 제1장·제3장',
        **{'zh-hans': '推进方式与基本工具：前作 第1章、第3章', 'zh-hant': '進行方式與基本工具：前作 第1章、第3章'}),
    '本書の計測手段：「まずは計測から」': dict(
        en='Measurement methods in this book: "Start with Measurement"',
        ko='이 책의 계측 수단: 「먼저 계측부터」',
        **{'zh-hans': '本书的测量手段：「首先从测量开始」', 'zh-hant': '本書的測量手段：「先從測量開始」'}),
    'アプリ容量が大きい': dict(en='App size is large', ko='앱 용량이 크다',
        **{'zh-hans': '应用体积过大', 'zh-hant': '應用程式容量過大'}),
    'メモリ超過・クラッシュ': dict(en='Out of memory / crashes', ko='메모리 초과·크래시',
        **{'zh-hans': '内存超限、崩溃', 'zh-hant': '記憶體超出、當機'}),
    '処理落ち・スパイク': dict(en='Frame drops / spikes', ko='프레임 저하·스파이크',
        **{'zh-hans': '掉帧、尖峰', 'zh-hant': '掉幀、尖峰'}),
    # セクション見出し（\foreach の /名前/ラベル 形式）
    '/c1/容量,': dict(en='/c1/Size,', ko='/c1/용량,', **{'zh-hans': '/c1/体积,', 'zh-hant': '/c1/容量,'}),
    '/c2/メモリ,': dict(en='/c2/Memory,', ko='/c2/메모리,', **{'zh-hans': '/c2/内存,', 'zh-hant': '/c2/記憶體,'}),
    # 症状
    'アセットが増え続ける': dict(en='Assets keep growing', ko='에셋이 계속 늘어난다',
        **{'zh-hans': '资源持续增加', 'zh-hant': '資產持續增加'}),
    'コードが大きい': dict(en='Code is large', ko='코드가 크다', **{'zh-hans': '代码体积大', 'zh-hant': '程式碼過大'}),
    '上限で落ちる': dict(en='Crashes at the limit', ko='상한에서 종료된다', **{'zh-hans': '到上限时崩溃', 'zh-hant': '到上限時當機'}),
    '解放されない': dict(en='Not released', ko='해제되지 않는다', **{'zh-hans': '未被释放', 'zh-hant': '未被釋放'}),
    '描画のメモリ': dict(en='Rendering memory', ko='렌더링 메모리', **{'zh-hans': '绘制内存', 'zh-hant': '繪製記憶體'}),
    'キャラクターが多い': dict(en='Many characters', ko='캐릭터가 많다', **{'zh-hans': '角色数量多', 'zh-hant': '角色數量多'}),
    '単発のスパイク': dict(en='One-off spike', ko='단발성 스파이크', **{'zh-hans': '单次尖峰', 'zh-hant': '單次尖峰'}),
    '描画の登録・UI': dict(en='Draw registration / UI', ko='렌더링 등록·UI', **{'zh-hans': '绘制注册、UI', 'zh-hant': '繪製註冊、UI'}),
    'C\\#コード全体': dict(en='C\\# code overall', ko='C\\# 코드 전체', **{'zh-hans': 'C\\# 代码整体', 'zh-hant': 'C\\# 程式碼整體'}),
    'メモリ帯域': dict(en='Memory bandwidth', ko='메모리 대역폭', **{'zh-hans': '内存带宽', 'zh-hant': '記憶體頻寬'}),
    'シェーダーの演算': dict(en='Shader arithmetic', ko='셰이더 연산', **{'zh-hans': '着色器运算', 'zh-hant': '著色器運算'}),
    '初回描画のスタッター': dict(en='Stutter at first draw', ko='첫 렌더링 시 끊김', **{'zh-hans': '首次绘制卡顿', 'zh-hant': '首次繪製卡頓'}),
    '大量描画・端末差': dict(en='Mass drawing / device gaps', ko='대량 렌더링·기기 차이', **{'zh-hans': '大量绘制、设备差异', 'zh-hant': '大量繪製、裝置差異'}),
    # 章タイトル（短縮）
    'AssetBundleのライフタイム管理': dict(en='Lifetime Management of AssetBundles', ko='AssetBundle의 라이프타임 관리',
        **{'zh-hans': 'AssetBundle 的生命周期管理', 'zh-hant': 'AssetBundle 的生命週期管理'}),
    'アセット肥大化の調査': dict(en='Investigating Asset Bloat', ko='에셋 비대화 조사',
        **{'zh-hans': '资源膨胀的调查', 'zh-hant': '資產膨脹的調查'}),
    'Code Strippingによるコードの削減': dict(en='Reducing Code with Code Stripping', ko='Code Stripping을 통한 코드 축소',
        **{'zh-hans': '通过 Code Stripping 缩减代码', 'zh-hant': '以 Code Stripping 縮減程式碼'}),
    'LTOによるコードの最適化': dict(en='Optimizing Code with LTO', ko='LTO를 통한 코드 최적화',
        **{'zh-hans': '通过 LTO 优化代码', 'zh-hant': '以 LTO 最佳化程式碼'}),
    'iOSのメモリ上限': dict(en='iOS Memory Limits', ko='iOS의 메모리 상한', **{'zh-hans': 'iOS 的内存上限', 'zh-hant': 'iOS 的記憶體上限'}),
    'UniTaskのライフタイム管理': dict(en='Lifetime Management in UniTask', ko='UniTask의 라이프타임 관리',
        **{'zh-hans': 'UniTask 的生命周期管理', 'zh-hant': 'UniTask 的生命週期管理'}),
    'Render Graphのタイルメモリ最適化': dict(en='Render Graph Tile Memory Optimization', ko='Render Graph 타일 메모리 최적화',
        **{'zh-hans': 'Render Graph 图块内存优化', 'zh-hant': 'Render Graph 圖塊記憶體最佳化'}),
    'RSUVによる描画の最適化': dict(en='Rendering Optimization with RSUV', ko='RSUV를 통한 렌더링 최적화',
        **{'zh-hans': '通过 RSUV 优化绘制', 'zh-hant': '以 RSUV 最佳化繪製'}),
    'VATによるボーンアニメーションの軽量化': dict(en='Lighter Bone Animation with VAT', ko='VAT를 통한 본 애니메이션 경량화',
        **{'zh-hans': '通过 VAT 减轻骨骼动画开销', 'zh-hant': '以 VAT 減輕骨骼動畫負載'}),
    'キャラクターアニメーションのチューニング': dict(en='Tuning Character Animation', ko='캐릭터 애니메이션 튜닝',
        **{'zh-hans': '角色动画的调优', 'zh-hant': '角色動畫的調優'}),
    'Animatorプーリングの初期化': dict(en='Initializing Pooled Animators', ko='Animator 풀링의 초기화',
        **{'zh-hans': 'Animator 池化的初始化', 'zh-hant': 'Animator 物件池化的初始化'}),
    'ParticleSystemのSimulateのスパイク': dict(en='ParticleSystem Simulate Spikes', ko='ParticleSystem Simulate의 스파이크',
        **{'zh-hans': 'ParticleSystem Simulate 的尖峰', 'zh-hant': 'ParticleSystem Simulate 的尖峰'}),
    'UnsafePassによる描画登録の軽量化': dict(en='Lighter Draw Registration with UnsafePass', ko='UnsafePass를 통한 렌더링 등록 경량화',
        **{'zh-hans': '通过 UnsafePass 减轻绘制注册开销', 'zh-hant': '以 UnsafePass 減輕繪製註冊負擔'}),
    'UIシステムの使い分け': dict(en='Choosing UI Systems', ko='UI 시스템의 구분 사용',
        **{'zh-hans': 'UI 系统的选用', 'zh-hant': 'UI 系統的取捨'}),
    'FinalBlitPassの省略': dict(en='Omitting FinalBlitPass', ko='FinalBlitPass 생략',
        **{'zh-hans': '省略 FinalBlitPass', 'zh-hant': '省略 FinalBlitPass'}),
    'シェーダーの超越関数の軽量化': dict(en='Lighter Transcendental Functions', ko='셰이더 초월 함수의 경량화',
        **{'zh-hans': '着色器超越函数的轻量化', 'zh-hant': '著色器超越函數的輕量化'}),
    'シェーダーウォームアップ': dict(en='Shader Warmup', ko='셰이더 워밍업', **{'zh-hans': '着色器预热', 'zh-hant': '著色器預熱'}),
    '端末別の描画設定': dict(en='Per-Device Rendering Settings', ko='기기별 렌더링 설정',
        **{'zh-hans': '按设备的绘制设置', 'zh-hant': '依裝置的繪製設定'}),
}
FONT_JA = '\\setgothicfont{BIZUDGothic-Regular.ttf}\n\\setboldgothicfont{BIZUDGothic-Bold.ttf}\n'
NOTO = {'ko': 'KR', 'zh-hans': 'SC', 'zh-hant': 'TC'}


def generate(lang):
    src = (FIG / 'reader_guide_flow.tex').read_text(encoding='utf-8')
    for ja in sorted(T, key=len, reverse=True):
        key = ja if ja.startswith('/') else '{' + ja + '}' if ja not in (
            '計測する', '進め方・基本ツール：前作 第1章・第3章', '本書の計測手段：「まずは計測から」') else ja
        new = T[ja][lang]
        new = new if ja.startswith('/') else ('{' + new + '}' if key.startswith('{') else new)
        assert key in src, ja
        src = src.replace(key, new)
    if lang in NOTO:
        n = NOTO[lang]
        font = (f'\\setgothicfont{{NotoSans{n}-Regular.otf}}\n\\setboldgothicfont{{NotoSans{n}-Bold.otf}}\n')
        assert FONT_JA in src
        src = src.replace(FONT_JA, font)
    if lang == 'en':
        # 狭い欄での単語の途中改行を避ける
        src = src.replace('\\begin{document}\n', '\\begin{document}\n\\hyphenpenalty=10000 \\exhyphenpenalty=10000 \\tolerance=9999 \\emergencystretch=3em\n', 1)
    src = src.replace('% 課題別の逆引きガイドの流れ図。', f'% 課題別の逆引きガイドの流れ図（{lang} 版）。reader_guide_flow.tex から scripts/gen-reader-guide-flow-tex.py で生成。', 1)
    (FIG / f'reader_guide_flow_{lang}.tex').write_text(src, encoding='utf-8')


if __name__ == '__main__':
    for lang in sys.argv[1:] or ['en', 'ko', 'zh-hans', 'zh-hant']:
        generate(lang)
