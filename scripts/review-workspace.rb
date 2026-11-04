# Prepares a Re:VIEW build directory outside articles/ so that builds never
# write into the manuscripts. Shared by the PDF (Docker) and web builds.
require 'fileutils'
require 'json'
require 'review'
require_relative 'review-structure'

module ReviewWorkspace
  # Populates work with the configuration, manuscripts and current image
  # assets, and points the rake tasks at it through the environment.
  # sources selects individual manuscripts; nil builds the whole book.
  def self.prepare(repo:, work:, config_name:, sources: nil)
    articles = File.join(repo, 'articles')
    FileUtils.mkdir_p(work)
    Dir.chdir(work)
    Dir.children(articles).each do |name|
      path = File.join(articles, name)
      if File.file?(path) && (name.match?(/\.(ya?ml|rb|tex|css|erb)\z/) || name == 'Rakefile')
        FileUtils.cp(path, name)
      end
    end
    %w[sty layouts lib fonts].each { |name| FileUtils.ln_s(File.join(articles, name), name) }
    config = ReVIEW::Configure.create(yamlfile: File.join(articles, config_name))
    catalog = YAML.load_file(File.join(articles, config['catalogfile']))
    if sources
      catalog = {'PREDEF' => [], 'CHAPS' => sources.map { |s| File.basename(s, '.*') + '.re' },
                 'APPENDIX' => [], 'POSTDEF' => []}
    end
    structure = nil
    if !sources && config['editorial_structure']
      structure = ReviewStructure.load(File.join(articles, config['editorial_structure']))
      editorial_dir = config['editorial_dir'] || 'editorial'
      ReviewStructure.validate!(structure, catalog, articles, editorial_dir: editorial_dir,
                                manuscript_dir: config.fetch('manuscript_dir', 'text_jp'))
    end
    source_dir = File.join(work, 'sources')
    FileUtils.mkdir_p(source_dir)
    original_contentdir = config['contentdir'] == '_refiles' ? config.fetch('manuscript_dir', 'text_jp') : config['contentdir']
    manuscripts = sources || Dir.glob(File.join(articles, original_contentdir, '*.{md,re}'))
    manuscripts += Dir.glob(File.join(articles, editorial_dir, '*.re')) if structure
    manuscripts.each do |source|
      source = File.expand_path(source, repo)
      FileUtils.ln_s(source, File.join(source_dir, File.basename(source)))
    end
    # Prefer the current manuscript assets to the last Drive synchronization.
    imagedir = config.fetch('imagedir', 'images_jp')
    FileUtils.mkdir_p(imagedir)
    Dir.children(File.join(articles, imagedir)).each do |name|
      FileUtils.ln_s(File.join(articles, imagedir, name), File.join(imagedir, name))
    end
    asset_roots = manuscripts.map { |s| File.join(File.dirname(File.expand_path(s, repo)), 'assets') }.select { |p| Dir.exist?(p) }.uniq
    unless asset_roots.empty?
      target = File.join(imagedir, 'assets')
      FileUtils.rm_f(target) if File.symlink?(target)
      FileUtils.mkdir_p(target)
      asset_roots.each do |root|
        Dir.children(root).each do |name|
          dest = File.join(target, name)
          origin = File.join(root, name)
          raise "conflicting image asset: #{name}" if File.exist?(dest) && File.realpath(dest) != File.realpath(origin)
          FileUtils.ln_s(origin, dest) unless File.exist?(dest)
        end
      end
    end
    overrides = {'inherit' => [File.join(articles, config_name)],
                 'bookname' => 'book', 'catalogfile' => 'catalog.yml', 'contentdir' => '_refiles'}
    # A selected-article preview omits front/back matter, not publication styles.
    if sources
      %w[titlepage titlefile coverfile coverimage backcover colophon toc].each { |k| overrides[k] = nil }
      overrides['pdfmaker'] = {'colophon' => false}
    end
    # The whole book keeps the cover, title page, colophon, and back cover of the
    # selected publication configuration; only the chapter numbering is overridden.
    if structure
      overrides['editorial_chapter_numbers'] = ReviewStructure.chapter_numbers(structure)
      overrides['toclevel'] = 1
    end
    File.write('config.yml', YAML.dump(overrides))
    File.write('catalog.yml', YAML.dump(catalog))
    ENV['REVIEW_CONFIG_FILE'] = 'config.yml'
    ENV['REVIEW_CATALOG_FILE'] = 'catalog.yml'
    ENV['REVIEW_MARKDOWN_SOURCE_DIR'] = source_dir
    ENV['REVIEW_BOOK'] = 'book'
  end
end
