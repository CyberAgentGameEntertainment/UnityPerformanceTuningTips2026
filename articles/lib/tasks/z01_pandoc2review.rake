# Copyright (c) 2020 Kenshi Muto
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

require 'fileutils'
require 'review'

def make_mdre(ch, path, source_dir)
  review_source = File.join(source_dir, ch)
  markdown_source = review_source.sub(/\.re\Z/, '.md')
  output = File.join(path, ch)

  FileUtils.mkdir_p(File.dirname(output))
  converter = File.expand_path('../../../scripts/prepare-review-source.py', File.dirname(File.realpath(__FILE__)))
  if File.exist?(review_source) # re file (the converter applies only the PDF glyph swap)
    success = system('python3', converter, review_source, output)
    raise "Re:VIEW source copy failed: #{review_source}" unless success
  elsif File.exist?(markdown_source) # md file
    success = system('python3', converter, markdown_source, output)
    raise "Markdown to Re:VIEW failed: #{markdown_source}" unless success
  else
    raise "manuscript not found: #{review_source} or #{markdown_source}"
  end
end

desc 'run pandoc2review'
task :pandoc2review do
  config = ReVIEW::Configure.create(yamlfile: CONFIG_FILE)
  if config['contentdir'] == '_refiles'
    path = '_refiles'
    source_dir = ENV.fetch('REVIEW_MARKDOWN_SOURCE_DIR', 'text_jp')

    unless File.exist?(path)
      Dir.mkdir(path)
      File.write("#{path}/THIS_FOLDER_IS_TEMPORARY", '')
    end

    catalog = YAML.load_file(CATALOG_FILE)
    %w(PREDEF CHAPS APPENDIX POSTDEF).each do |block|
      if catalog[block].kind_of?(Array)
        catalog[block].each do |ch|
          if ch.kind_of?(Hash) # Parts
            ch.each_pair do |k, v|
              make_mdre(k, path, source_dir)
              # Chapters
              v.each {|subch| make_mdre(subch, path, source_dir) }
            end
          elsif ch.kind_of?(String) # Chapters
            make_mdre(ch, path, source_dir)
          end
        end
      end
    end
  end
end

CLEAN.include('_refiles')
Rake::Task[BOOK_PDF].enhance([:pandoc2review])
Rake::Task[BOOK_EPUB].enhance([:pandoc2review])
Rake::Task[WEBROOT].enhance([:pandoc2review])
Rake::Task[TEXTROOT].enhance([:pandoc2review])
Rake::Task[TOPROOT].enhance([:pandoc2review])
Rake::Task[IDGXMLROOT].enhance([:pandoc2review])
