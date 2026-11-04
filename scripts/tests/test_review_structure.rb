require_relative '../review-structure'
articles = File.expand_path('../../articles', __dir__)
structure = JSON.parse(File.read(File.join(articles, 'editorial-structure.json')))
catalog = YAML.load_file(File.join(articles, 'catalog.yml'))
ReviewStructure.validate!(structure, catalog, articles)
expected = [%w[kiyohara-takayuki-1 zhang-yubing-1 zhang-yubing-2 nagatome-ryouhei-1 seto-noboru-1 amano-takuro-1],
            %w[saito-shunsuke-1 shiotsuka-yuki-1 tatami-yuuki-1 kato-yuya-1 inada-taketo-1 zaitsu-hiromu-2],
            %w[inada-taketo-2 zaitsu-hiromu-1 sakuma-naoki-1 mukai-yuichiro-2 kim-minhyuk-1 kim-minhyuk-2],
            %w[yamazaki-masahiro-1 kim-minhyuk-3 nakamura-teruhisa-1]]
raise 'reference PDF article order changed' unless structure['parts'].map { |part| part['articles'] } == expected
raise 'part numbering changed' unless ReviewStructure.chapter_numbers(structure)['kim-minhyuk-1'] == '5'
mutations = [
  ->(s, c) { c['CHAPS'].reverse! },
  ->(s, c) { c['PREDEF'] = [] },
  ->(s, c) { s['parts'][0]['articles'] << s['parts'][0]['articles'][0]; c.replace(ReviewStructure.catalog(s)) },
  ->(s, c) { s['parts'][0]['articles'].pop; c.replace(ReviewStructure.catalog(s)) },
  ->(s, c) { s['parts'][0]['title'] = 'unexpected title' }
]
mutations.each do |mutate|
  s, c = Marshal.load(Marshal.dump([structure, catalog]))
  mutate.call(s, c)
  rejected = false
  begin
    ReviewStructure.validate!(s, c, articles)
  rescue RuntimeError
    rejected = true
  end
  raise 'invalid structure was accepted' unless rejected
end
Dir.glob(File.join(articles, 'editorial-structure_*_2026.json')).each do |path|
  translated = ReviewStructure.load(path)
  base = structure
  %w[predef before_parts postdef].each { |k| raise "#{path}: #{k} differs from the base" unless translated[k] == base[k] }
  raise "#{path}: articles differ from the base" unless translated['parts'].map { |x| x['articles'] } == base['parts'].map { |x| x['articles'] }
  raise "#{path}: part titles were not applied" if translated['parts'].map { |x| x['title'] } == base['parts'].map { |x| x['title'] }
end
puts '8 editorial structure checks passed, translated structures resolve'
