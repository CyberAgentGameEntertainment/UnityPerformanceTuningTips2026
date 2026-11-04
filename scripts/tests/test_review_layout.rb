require_relative '../../articles/review-ext'

builder = ReVIEW::LATEXBuilder.allocate
cases = {
  'Serialization/Asset' => '\\reviewbreakall{Serialization/Asset}',
  '\\href{https://example.com/longidentifier}{\\reviewcode{LongIdentifier}}' => '\\href{https://example.com/longidentifier}{\\reviewcode{LongIdentifier}}',
  '関数数 1,419 → 1,290 +49.66%' => '関数数 1,419 → 1,290 +49.66%'
}
cases.each do |input, expected|
  actual = builder.break_table_text(input)
  raise "Expected #{expected.inspect}, got #{actual.inspect}" unless actual == expected
end
puts "#{cases.size} Re:VIEW table wrapping checks passed"
