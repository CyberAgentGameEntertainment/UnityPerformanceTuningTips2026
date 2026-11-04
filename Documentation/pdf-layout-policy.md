# 原稿とPDFの共通方針

本リポジトリのプレビューと出版用PDFは、すべてRe:VIEWで組版します。個別記事も書籍全体も、同じ原稿変換・設定・スタイルを使います。

## ビルド経路

`articles/text_jp/*.md` → Google Docs記法の正規化 → `pandoc2review` → 一時的な `.re` → Re:VIEW 5.11 → PDF。

PandocはMarkdownの解析とRe:VIEW形式への変換にのみ使います。PandocからLaTeX/PDFを直接出力する経路は使いません。既存の `.re` 原稿はそのままRe:VIEWへ渡します。

- 共通入口: `scripts/build-review-pdf.py`
- 原稿変換: `scripts/prepare-review-source.py`。通常のRe:VIEW Rakeビルドでも同じ変換を呼び出します。
- 出版設定: `articles/config-epub-jp.yml`（画面閲覧・電子配布用・`scripts/build-review-pdf.py` の既定）と `articles/config-jp.yml`（印刷入稿用）の2つです。`config-epub-jp.yml` は書名・奥付・構成定義などを持つ共通の基底でもあり、入稿用・英語版・Web版が継承します。入稿用は `media`・裏表紙・奥付の印刷所だけを上書きします。版面・コードハイライト・章の始まり（右ページ）は同じなので、画面閲覧用で確認した改行と改ページは入稿版でも変わりません。プレビュー専用のフォント・余白設定を作りません。
- リンクの扱い: 印刷用は `media=print` でトンボ付きのA4面付けになり、`review-jsbook.cls` が `hyperref` を `draft` で読み込むため、PDF内のリンクとしおりは作られません。画面で読むPDFは既定の `config-epub-jp.yml`（`media=ebook`）でビルドし、目次・相互参照・脚注・URLのリンクとしおりを有効にします。版面は印刷用と同一で、トンボがなくB5に仕上がるだけの違いです。入稿用のPDFが要るときだけ `--config config-jp.yml` を指定します。
- 隠しノンブル: 印刷用（`config-jp.yml`）は `hiddenfolio=nikko-pc` で、扉・白ページ・目次・奥付を含む全ページのノド（仕上がり線の内側、天地中央）にページ番号を縦に入れます。印刷所が丁合で乱丁・落丁を確かめるためです。地色を塗る扉では白抜きにします。`review-jsbook.cls` のeverypageフックは現行のLaTeXでは箱の作り直しが捨てられるため、`articles/sty/review-custom.sty` でページ出力の直前に作り直しています。
- 共通スタイル: `articles/sty/reviewmacro.sty` と、そこから読み込むスタイル。
- 図表配置: 図は `review-style.sty` の `H` 設定で原稿順を維持。表は `review-ext.rb` と `longtable` で本文幅に収め、ページをまたぐ場合も行順を維持します。明示された表の列幅は優先します。コードは共通のRe:VIEWスタイル内で `fvextra` を使い、文字内容を保ったまま長い行を折り返します。

## 著者プロフィール

各記事の冒頭には、著者名・ローマ字・所属・リンクを整形したプロフィール枠を入れます。内容の正本は `articles/authors.json` で、キーは原稿のファイル名（拡張子なし）です。著者情報は原稿本文ではなくこのファイルを編集します。

`name` は必須、`romaji`・`affiliation`・`github`・`x`・`web` は任意です。`github` と `x` はアカウント名でもURLでも書けます。エントリーがある記事は、原稿冒頭の著者行（ラベル付き・スラッシュ区切り・`# 名前・所属` のような見出し節・区切り線）を変換時に取り除き、同じ位置にプロフィール枠を出力します。エントリーがない記事は警告を出し、原稿の記載をそのまま残します。

PDFでは `articles/sty/review-custom.sty` の `\reviewauthorprofile` が組版し、GitHub・X・Webのアイコンは `fontawesome5` を使います。HTMLとEPUBには同じ内容をインラインSVG付きの `author-profile` ブロックとして出力し、各スタイルシートで配色を合わせます。

## 実行例

```sh
# 全体: structure-order-previewと同じ4部・18記事の構成で作成
python3 scripts/build-review-pdf.py --output output/pdf/book-review2.pdf

# 単独プレビュー: 同じ出版スタイルで対象記事だけ選択
python3 scripts/build-review-pdf.py articles/text_jp/kim-minhyuk-1.md \
  --output output/pdf/kim-minhyuk-1-review2.pdf

# 全体の印刷入稿用（固定出力。output.pdf と同じく毎回上書き）
python3 scripts/build-review-pdf.py --config config-jp.yml \
  --output output/pdf/output-print.pdf

# 入稿用の設定で確認
python3 scripts/build-review-pdf.py articles/text_jp/kim-minhyuk-1.md \
  --config config-jp.yml --output output/pdf/kim-minhyuk-1-print-review2.pdf

# 本文モノクロ印刷の入稿用（Ghostscriptでグレースケール化。固定出力で毎回上書き）
python3 scripts/build-review-pdf.py --config config-jp.yml --grayscale \
  --output output/pdf/output-print-gray.pdf
```

`--grayscale` は組版後のPDFをGhostscript（`brew install ghostscript`）で `DeviceGray` に変換します。文字・コードの色分け・表の地色・スクリーンショット・部扉まで紙面全体がグレーになるので、モノクロ印刷で色の区別に頼った箇所が判別できるかを確認できます。組版は変換前と同一です。本書の本文はモノクロ印刷なので、全体ビルドの `output/pdf/output-print-gray.pdf` を入稿データにします（印刷所のグレースケール変換に任せず、仕上がりを手元で確認できるようにするため）。Ghostscriptの `pdfwrite` は既定で画像をJPEGに再圧縮するため、`-dAutoFilterGrayImages=false` などでFlateのまま変換し、スクリーンショットの画質を落としません。扉の特色（PANTONE Black C）は変換後も残るので、入稿時に印刷所へ伝えます。

全体ビルドは選んだ出版設定の表紙・扉・奥付・裏表紙をそのまま出力します。画面閲覧・電子配布用（`config-epub-jp.yml`）はカラー表紙・扉・奥付・裏表紙の4ページが付きます。印刷用（`config-jp.yml`）は `media=print` のため表紙と裏表紙は付かず、扉と奥付だけになります（表紙は印刷所へ別入稿する前提です）。目次・はじめに・部の概説といった前付も構成定義どおりに並びます。扉（`articles/title.tex` / `title-epub.tex` から呼ぶ `\reviewtitlepage`）はデザイナー支給の `articles/images/divider/tuning-tips-2026-interior.pdf` の1ページ目です（後述の部扉と同じファイル）。表紙と裏表紙はデザイナー支給の `articles/images_jp/cover/tuning-tips-2026-cover.pdf`（1ページ目が表紙、2ページ目が裏表紙）です。PDF版は `config-epub-jp.yml` の `pdfmaker.coverimage` で1ページ目を、`articles/backcover.tex` で2ページ目を貼ります。EPUB・Web版の表紙には、1ページ目をラスタライズした `tuning-tips-2026-cover.png` を使います。

対象記事を指定した場合は、表紙・奥付・目次を省略します。章番号は選択した記事の中で付け直されます。組版設定は選んだ出版設定から継承します。複数の記事も同じコマンドに並べて指定できます。

`npm run pdf -- ...`、`build-in-docker.sh`、`build-in-docker-epub.sh`、旧 `scripts/build-google-drive-markdown-pdf.sh` も共通入口を呼びます。旧スクリプト名でもPDF生成はRe:VIEWです。`--markdown-dir` を明示した場合のみ、そのディレクトリのMarkdownをファイル名の数字順で並べます。

Dockerイメージは `docker build -t review-custom:5.11 .` で用意します。Ruby依存関係は `Gemfile.lock` に従って専用Dockerボリュームに保存します。生成物は `tmp/pdfs/review-*/` に隔離し、成功したPDFだけ指定先へコピーします。ログと変換済み原稿もこの一時ディレクトリで確認できます。

既存PDFは上書きしません。出力先が存在する場合は、新しいファイル名を指定します。既存のカタログ、原稿、画像をビルドのために書き換えません。

## 配置と校正

本文・図・表・コード・見出しの読む順序を保ちます。図とキャプションがページ末に収まらない場合は次ページへ送り、後続の本文を先へ出しません。この結果生じる余白は許容します。長い表はページを分割し、見出し行を繰り返します。

最新原稿から毎回変換し直します。古い中間Markdownを最終入力として再利用しません。確認時は、図の前後の本文・小見出し、キャプションと参照番号、長い表の継続、コードの内容、文字の切れや重なりを原稿とPDFで照合します。

変換テスト:

```sh
# Docker内ではpandoc2reviewを使う結合テストも実行されます。
docker run --rm -v "$PWD:/book:ro" -v unity-tuning-review-gems:/usr/local/bundle \
  -w /book -e PYTHONDONTWRITEBYTECODE=1 review-custom:5.11 \
  bundle exec python3 -m unittest discover -s scripts/tests -v

docker run --rm -v "$PWD:/book:ro" -v unity-tuning-review-gems:/usr/local/bundle \
  -w /book review-custom:5.11 bundle exec ruby scripts/tests/test_review_layout.rb
```

## 全体構成を維持するハーネス

日本語の全体ビルドは `articles/editorial-structure.json` を構成の基準とし、`articles/catalog.yml` が一致することをビルド前に検証します。基準PDFは `output/pdf/unity-performance-tuning-tips-2026-structure-order-preview.pdf` です。記事本文は毎回最新のMarkdownから変換します。

- 第1部: グラフィックス最適化（6記事）
- 第2部: スクリプティング・アニメーション最適化（5記事）
- 第3部: アセット・設定・ビルド最適化（5記事）
- 第4部: その他: 計測・UI選択（2記事）

はじめに、目次、課題別の逆引きガイド、序章、部の概説、おわりにの位置も保持します。未執筆の構成ページは `articles/editorial/*.re` に見出しとして保存し、勝手に本文を補いません。章番号は部ごとに1から振り直し、目次と柱には「第1章」と出ます。本文から他の章を指すときは「第N部M章」と書きます。

部扉はデザイナー支給の `articles/images/divider/tuning-tips-2026-interior.pdf` の2〜5ページ目（第1〜4部）を全面に置きます（1ページ目は大扉）。支給データは仕上がりサイズで塗り足しがないため、同じページを塗り足し寸法まで伸ばして下に敷き、その上に仕上がりサイズのページを重ねて塗り足しを補います。塗り足しの幅は印刷用ではgentombowのBleedBoxと同じ値（`\@tombowbleed`、3mm）を使います。扉の端は一様な地色なので、伸ばしても色空間ごと同じ色になります（別の色を塗ると画面表示でICCベースの地色と濃さがずれます）。

支給データはレイヤー（オプショナルコンテンツ）を統合したIllustrator PDFで受け取り、変換せずにそのまま置きます。dvipdfmxはページを取り込むときにレイヤーの定義を落とすため、レイヤーが残ったPDFは使いません。レイヤー付きで届いた場合はデザイナーに統合を依頼します。以前はGhostscriptでレイヤーを統合していましたが、スポットカラーの代替色空間から `/Range` が落ちてmacOSのプレビューで文字が白く消えるなどの副作用があったため、統合済みのデータに差し替えました。

デザインを差し替えるときはこのPDFを上書きします。ページ数は大扉1ページ＋部の数（`editorial-structure.json`）で、`review-custom.sty` の `\reviewdividerdesignpages` と一致する必要があります。`python3 -m unittest scripts/tests/test_divider_design.py` がページ数を検査します。配置用のPDFがない場合は組み版の大扉（書名だけ）と次の組み版の部扉を出し、部の数がページ数を超えた部も組み版の部扉に戻します。

組み版の部扉と章扉は `articles/sty/review-custom.sty` で生成します。旧版の章扉イラストは新しい構成と内容が合わないため使いません。扉は全面を地色（`reviewdividerink`、既定は `#2B2B2B`）で塗り、中央に白フチのパネルを置き、書名の板・`PART`/`CHAPTER` と番号・部名または記事タイトル・下部のクレジットを載せます。扉が出るのは構成定義を使う全体ビルド（`\reviewstructuredbook` が定義されるビルド）だけで、単独記事プレビューには出ません。パネルの文字は次の3つで差し替えられます。

| マクロ | 既定値 |
| --- | --- |
| `\reviewdividermarkupper` | `UNITY` |
| `\reviewdividermarklower` | `PERFORMANCE TUNING TIPS 2026` |
| `\reviewdividerfooter` | `CyberAgent Smartphone Games & Entertainment` |

地色は `atbegshi` の `\AtBeginShipoutNext` で紙面に直接置きます。`eso-pic` は `plautopatch` が読み込む `pxesopic` と衝突してビルドが止まるため使いません。`media=print`（トンボ付き）では `atbegshi` の原点がTeXの原点より1インチ左上にずれるので、`\reviewdividerorigin` で1インチ分を戻してから塗ります。塗り足しは仕上がりから3mmです。パネルは版面ではなく仕上がり紙面の中央にそろえるため、`\oddsidemargin` / `\evensidemargin` と `\topmargin` から中央までの差を打ち消しています。

`ruby scripts/tests/test_review_structure.rb` は基準の順序と採番に加え、部の順序変更、前付の欠落、記事の重複・欠落、部タイトルの不一致が拒否されることを確認します。通常の全体ビルドでも `scripts/review-structure.rb` による検証が実行されます。構成変更は利用者の明示的な指示がある場合だけ行います。単独記事プレビューには全体構成を適用しません。

```sh
python3 scripts/build-review-pdf.py \
  --output output/pdf/unity-performance-tuning-tips-2026-structure-order-review2.pdf
```
