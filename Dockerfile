# Pinned by digest (ruby 3.4.12 / Debian 12.15, built 2026-10-28). To update: pull ruby:3.4-bookworm,
# copy its digest here, rebuild, and refresh docker/tool-versions.txt with scripts/check-docker-tools.sh --write.
FROM ruby:3.4.12-bookworm@sha256:8853c6f24866429483fce3a869e61a6a13916beb2720468c6d95c78f41c0f564

# TeX Live + Japanese support for Re:VIEW PDF builds
RUN apt-get update && apt-get install -y --no-install-recommends \
    texlive-luatex \
    texlive-lang-japanese \
    texlive-lang-cjk \
    texlive-fonts-recommended \
    texlive-fonts-extra \
    texlive-latex-recommended \
    texlive-latex-extra \
    texlive-plain-generic \
    texlive-extra-utils \
    lmodern \
    pandoc \
    nodejs \
    npm \
    ghostscript \
    && rm -rf /var/lib/apt/lists/*

RUN gem install bundler:4.0.3

WORKDIR /book
