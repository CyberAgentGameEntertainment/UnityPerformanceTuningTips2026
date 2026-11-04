# Runs inside the build container; all generated files live in /work.
require 'json'
require_relative 'review-workspace'

request = JSON.parse(File.read('/work/request.json'))
ReviewWorkspace.prepare(repo: '/book', work: '/work', config_name: request.fetch('config'), sources: request['sources'])
exec('bundle', 'exec', 'rake', 'pdf')
