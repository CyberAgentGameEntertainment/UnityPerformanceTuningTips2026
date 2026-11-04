# Shared full-book structure contract. No Re:VIEW dependency is needed to test it.
require 'json'
require 'yaml'
module ReviewStructure
  # A translated structure only gives part_titles; everything else comes from the file it extends.
  def self.load(path)
    structure = JSON.parse(File.read(path))
    base = structure['extends']
    return structure unless base
    titles = structure.fetch('part_titles')
    merged = load(File.join(File.dirname(path), base))
    unless titles.size == merged.fetch('parts').size
      raise "part_titles has #{titles.size} entries, the base structure has #{merged['parts'].size} parts"
    end
    merged['parts'].each_with_index { |part, i| part['title'] = titles[i] }
    merged
  end

  def self.catalog(structure)
    {'PREDEF' => structure.fetch('predef'),
     'CHAPS' => structure.fetch('before_parts') + structure.fetch('parts').map do |part|
       {part.fetch('file') => [part['intro']].compact + part.fetch('articles').map { |id| id + '.re' }}
     end,
     'APPENDIX' => [], 'POSTDEF' => structure.fetch('postdef')}
  end

  # 章番号は部ごとに 1 から振り直す。本文の相互参照は「第N部M章」と書き下す。
  def self.chapter_numbers(structure)
    structure.fetch('parts').each_with_object({}) do |part, result|
      part.fetch('articles').each_with_index { |id, j| result[id] = (j + 1).to_s }
    end
  end

  def self.validate!(structure, actual_catalog, articles, editorial_dir: 'editorial', manuscript_dir: 'text_jp')
    raise 'catalog.yml differs from editorial-structure.json (order/front matter/parts)' unless actual_catalog == catalog(structure)
    ids = structure.fetch('parts').flat_map { |part| part.fetch('articles') }
    raise 'duplicate manuscript in editorial structure' unless ids.uniq == ids
    available = Dir.glob(File.join(articles, manuscript_dir, '*.md')).map { |file| File.basename(file, '.md') }
    unless ids.sort == available.sort
      raise "editorial manuscript coverage differs: missing=#{available - ids}, unknown=#{ids - available}"
    end
    auxiliary = structure.fetch('predef') + structure.fetch('before_parts') + structure.fetch('postdef') +
                structure.fetch('parts').flat_map { |part| [part.fetch('file'), part['intro']].compact }
    auxiliary.each do |name|
      raise "missing editorial manuscript: #{name}" unless File.file?(File.join(articles, editorial_dir, name))
    end
    structure.fetch('parts').each do |part|
      expected = '= ' + part.fetch('title')
      actual = File.readlines(File.join(articles, editorial_dir, part.fetch('file'))).first.to_s.strip
      raise "part title differs: #{part['file']}" unless actual == expected
    end
    true
  end
end
