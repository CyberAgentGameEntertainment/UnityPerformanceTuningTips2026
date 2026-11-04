# Builds the web edition in a work directory; the HTML lands in <work>/webroot.
# Usage: ruby scripts/build-review-web.rb <config under articles/> <work dir>
require 'fileutils'
require_relative 'review-workspace'

config_name, work = ARGV
abort 'usage: build-review-web.rb CONFIG WORK_DIR' unless config_name && work
repo = File.expand_path('..', __dir__)
work = File.expand_path(work)
ENV['BUNDLE_GEMFILE'] ||= File.join(repo, 'Gemfile')
# The Markdown converter runs from scripts/; keep its bytecode out of the tree.
ENV['PYTHONDONTWRITEBYTECODE'] = '1'
FileUtils.rm_rf(work)
ReviewWorkspace.prepare(repo: repo, work: work, config_name: config_name)
exec('bundle', 'exec', 'rake', 'web')
