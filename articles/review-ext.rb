# Keep tables in manuscript order and allow them to continue onto the next page.
require 'review/latexbuilder'
require 'review/htmlbuilder'
require 'review/book/chapter'
require 'review/tocprinter'
require 'strscan'

# Re:VIEW ships labels only for ja/en/zh-TW. The ko/zh-Hans/zh-Hant editions (text_*_2026) derive
# their labels from the English table, so figure/table captions and chapter headings are not
# left as raw keys such as "chapter_postfix".
require 'review/i18n'
module TranslatedEditionLabels
  LABELS = {
    'ko' => { 'image' => '그림 ', 'table' => '표 ', 'list' => '리스트 ', 'equation' => '수식 ',
              'column' => '칼럼 %s', 'columnname' => '칼럼', 'column_head' => '칼럼',
              'chapter' => '제%d장', 'chapter_quote' => '%s "%s"', 'appendix' => '부록 %s',
              'numberless_image' => '그림:', 'toctitle' => '목차', 'memo_head' => '메모',
              'note_head' => '참고', 'tip_head' => '팁', 'info_head' => '정보',
              'warning_head' => '경고', 'important_head' => '중요', 'caution_head' => '주의',
              'notice_head' => '알림',
              'aut' => '저자', 'csl' => '감수', 'trl' => '번역', 'dsr' => '디자인', 'ill' => '일러스트',
              'cov' => '표지', 'edt' => '편집', 'pbl' => '발행처', 'contact' => '연락처', 'prt' => '인쇄소' },
    'zh-Hans' => { 'image' => '图 ', 'table' => '表 ', 'list' => '清单 ', 'equation' => '公式 ',
                   'column' => '专栏 %s', 'columnname' => '专栏', 'column_head' => '专栏',
                   'chapter' => '第%d章', 'chapter_quote' => '%s “%s”', 'appendix' => '附录 %s',
                   'numberless_image' => '图:', 'toctitle' => '目录', 'memo_head' => '备忘',
                   'note_head' => '注', 'tip_head' => '提示', 'info_head' => '信息',
                   'warning_head' => '警告', 'important_head' => '重要', 'caution_head' => '注意',
                   'notice_head' => '通知',
                   'aut' => '作者', 'csl' => '监修', 'trl' => '翻译', 'dsr' => '设计', 'ill' => '插图',
                   'cov' => '封面', 'edt' => '编辑', 'pbl' => '发行方', 'contact' => '联系方式', 'prt' => '印刷厂' },
    'zh-Hant' => { 'image' => '圖 ', 'table' => '表 ', 'list' => '清單 ', 'equation' => '公式 ',
                   'column' => '專欄 %s', 'columnname' => '專欄', 'column_head' => '專欄',
                   'chapter' => '第%d章', 'chapter_quote' => '%s「%s」', 'appendix' => '附錄 %s',
                   'numberless_image' => '圖:', 'toctitle' => '目錄', 'memo_head' => '備忘',
                   'note_head' => '註', 'tip_head' => '提示', 'info_head' => '資訊',
                   'warning_head' => '警告', 'important_head' => '重要', 'caution_head' => '注意',
                   'notice_head' => '通知',
                   'aut' => '作者', 'csl' => '監修', 'trl' => '翻譯', 'dsr' => '設計', 'ill' => '插圖',
                   'cov' => '封面', 'edt' => '編輯', 'pbl' => '發行方', 'contact' => '聯絡方式', 'prt' => '印刷廠' }
  }.freeze

  def load_default
    super
    LABELS.each { |locale, labels| @store[locale] = @store['en'].merge(labels) }
  end
end
ReVIEW::I18n.prepend(TranslatedEditionLabels)
# review-ext.rb can be loaded after I18n.setup has already built the store.
if (i18n = ReVIEW::I18n.instance_variable_get(:@i18n))
  TranslatedEditionLabels::LABELS.each { |l, v| i18n.update(i18n.get('image', 'en') ? i18n.instance_variable_get(:@store)['en'].merge(v) : v, l) }
end

module ManuscriptTableLayout
  # Apply break opportunities only to plain text, never to LaTeX commands/URLs.
  def break_table_text(text)
    # Cells are compiled before #table, so undo IdentifierBreaks and let
    # \reviewbreakall handle long words as before.
    text = text.gsub(/\\reviewnohyphen\{((?:[^{}]|\{[^{}]*\})*)\}/) do
      Regexp.last_match(1).gsub(/\\(?:reviewidbreak|allowbreak)\{\}/, '')
    end
    scanner = StringScanner.new(text)
    output = +''
    until scanner.eos?
      if (command = scanner.scan(/\\[A-Za-z]+|\\./))
        output << command
        while scanner.peek(1) == '{'
          depth = 0
          loop do
            character = scanner.getch
            output << character
            if character == '\\'
              output << scanner.getch.to_s
            elsif character == '{'
              depth += 1
            elsif character == '}'
              depth -= 1
            end
            break if depth.zero? || scanner.eos?
          end
        end
      else
        output << scanner.scan(/[^\\]+/).gsub(/[A-Za-z][A-Za-z0-9\/._-]{7,}/) { |word| macro('reviewbreakall', word) }
                       .gsub(/(?<![\s(])\((?=[A-Za-z])/, '\\allowbreak{}(')
      end
    end
    output
  end

  def table(lines, id = nil, caption = nil)
    separator, rows = parse_table_rows(lines)
    count = rows.first.size
    # Explicit author column widths (mm) remain available; if they add up to more than
    # the text area, they shrink in proportion. Otherwise fit the text area.
    fit = nil
    if @latex_tsize || @tsize
      size = @latex_tsize || @tsize
      cells = if /\A[\d., ]+\z/.match?(size)
                widths = size.split(/\s*,\s*/)
                fit = "\\reviewfittsize{#{widths.sum(&:to_f)}}{#{widths.size}}"
                widths.map { |v| "p{#{v}\\reviewtsizeunit}" }
              else
                separate_tsize(size)
              end
    else
      cells = Array.new(count) { "p{\\dimexpr\\linewidth/#{count}-2\\tabcolsep-2\\arrayrulewidth\\relax}" }
    end
    blank
    puts '\\begingroup\\small\\setlength{\\tabcolsep}{3pt}'
    puts fit if fit
    columns = cells.map { |cell| cell.start_with?('p{') ? ">{\\raggedright\\arraybackslash}#{cell}" : cell }
    puts "\\begin{longtable}{|#{columns.join('|')}|}"
    if caption.present?
      puts "\\caption#{id ? '' : '*'}{#{compile_inline(caption)}}#{id ? macro('label', table_label(id)) : ''}\\\\"
    elsif id
      puts "\\label{#{table_label(id)}}"
    end
    puts '\\hline'
    if separator
      rows.shift(separator).each do |row|
        tr(row.each_with_index.map { |cell, index| th(break_table_text(cell), cells[index]) })
      end
      puts '\\endhead'
    end
    rows.each do |row|
      tr(row.each_with_index.map { |cell, index| td(break_table_text(cell), cells[index]) })
    end
    puts '\\end{longtable}'
    puts '\\addtocounter{table}{-1}' unless id
    puts '\\endgroup'
    blank
  ensure
    @tsize = @latex_tsize = @cellwidth = nil
  end
end

ReVIEW::LATEXBuilder.prepend(ManuscriptTableLayout)

# Keep chapter captions, references, and LaTeX numbering in part-article form.
module EditorialChapterNumber
  def format_number(heading = true)
    label = @book.config.fetch('editorial_chapter_numbers', {})[id]
    return super unless label
    heading ? ReVIEW::I18n.t('chapter', label.to_i) : label
  end
end
ReVIEW::Book::Chapter.prepend(EditorialChapterNumber)

module EditorialHeadingNumber
  def headline(level, label, caption)
    number = @book.config.fetch('editorial_chapter_numbers', {})[@chapter.id]
    puts "\\renewcommand{\\thechapter}{#{number}}" if level == 1 && number
    super
  end
end
ReVIEW::LATEXBuilder.prepend(EditorialHeadingNumber)

# Plain-text API names and paths in prose need their own line breaking:
# - "CancellationTokenSource.CreateLinkedTokenSource" or "Library/com.unity.addressables/
#   .../link.xml" have no break opportunity and can be wider than the line, so they ran
#   off the page. Allow breaks after "." and "/" in such words (from 20 characters in
#   headings, 30 elsewhere); shorter words like "Debug.Log" keep their breaks.
# - English hyphenation split names like "Mesh-Skinning" or "Command-Buffer" and printed
#   a hyphen that is not part of the name. Names (camelCase, dotted or slashed words) are not
#   hyphenated ("Vulkan/Metal" printed "Vulka-n/Metal"); they may break between camelCase
#   parts without a hyphen instead.
# Tables keep their own breaking: ManuscriptTableLayout unwraps these words again.
module IdentifierBreaks
  WORD = %r{[A-Za-z][\w<>"()-]*(?:[./][\w<>"()-]+)*}
  CAMEL = /(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])/
  HEADING_MIN = 20
  TEXT_MIN = 30

  def headline(level, label, caption)
    @identifier_heading = true
    super
  ensure
    @identifier_heading = nil
  end

  def nofunc_text(str)
    identifier_text(str)
  end

  def inline_b(str)
    macro('reviewbold', identifier_text(str))
  end

  def inline_strong(str)
    macro('reviewstrong', identifier_text(str))
  end

  private

  def identifier_text(str)
    return escape(str) unless str.match?(WORD)
    str.split(/(#{WORD})/o).map do |part|
      next escape(part) unless part.match?(/\A#{WORD}\z/o)
      long = part.match?(%r{[./]}) && part.length >= (@identifier_heading ? HEADING_MIN : TEXT_MIN)
      next escape(part) unless long || part.match?(CAMEL) || part.match?(%r{\w[./]\w})
      pieces = long ? part.split(%r{(?<=[./])}) : [part]
      body = pieces.map { |piece| piece.split(CAMEL).map { |chunk| escape(chunk) }.join('\\reviewidbreak{}') }
      latex = macro('reviewnohyphen', body.join('\\allowbreak{}'))
      # Bookmarks cannot hold these commands.
      @identifier_heading ? "\\texorpdfstring{#{latex}}{#{escape(part)}}" : latex
    end.join
  end
end
ReVIEW::LATEXBuilder.prepend(IdentifierBreaks)

module EditorialUnnumberedHeadings
  def nonum_begin(level, label, caption)
    return super unless @book.config['editorial_chapter_numbers']
    if @chapter.id == 'editorial-reader-guide' && level == 2
      blank
      puts macro('reviewguidesection*', compile_inline(caption))
      puts macro('addcontentsline', 'toc', 'chapter', '\\hspace{1em}' + compile_inline(caption))
    else
      super
    end
    # Unnumbered front matter must never inherit "chapter 0" or the previous article.
    puts macro('markboth', compile_inline(caption), compile_inline(caption)) if level == 1
  end
end
ReVIEW::LATEXBuilder.prepend(EditorialUnnumberedHeadings)

# A blank line between paragraphs in the manuscript stays a blank line in the PDF.
module ManuscriptParagraphGap
  def paragraph(lines)
    puts '\\vspace{\\baselineskip}' if @paragraph_end_pos && @paragraph_end_pos == @output.pos
    super
    @paragraph_end_pos = @output.pos
  end

  # //noindent prints \noindent before the paragraph, so keep the gap here.
  def noindent
    puts '\\vspace{\\baselineskip}' if @paragraph_end_pos && @paragraph_end_pos == @output.pos
    super
    @paragraph_end_pos = nil
  end
end
ReVIEW::LATEXBuilder.prepend(ManuscriptParagraphGap)

# The web sidebar lists only headings marked by the TOC builder, which Re:VIEW
# does for numbered headings alone; unnumbered front matter would show as "-".
module UnnumberedTocHeadline
  def nonum_begin(level, label, caption)
    print "\x01H#{level}\x01"
    super
  end
end
ReVIEW::PLAINTEXTTocBuilder.prepend(UnnumberedTocHeadline)

# //pagebreak adjusts the printed pages only. The web sidebar is built by the
# plain-text TOC builder, which lacks the command and dropped those chapters.
module PlaintextPagebreak
  def pagebreak(*); end
end
ReVIEW::PLAINTEXTBuilder.prepend(PlaintextPagebreak)

# The web edition shows code listings with line numbers like the original
# Unity Performance Tuning Bible. Only the web configuration enables Rouge, so
# the PDF and EPUB keep plain listings.
module WebListLineNumbers
  def list_body(id, lines, lang)
    highlight? ? listnum_body(lines, lang) : super
  end
end
ReVIEW::HTMLBuilder.prepend(WebListLineNumbers)
