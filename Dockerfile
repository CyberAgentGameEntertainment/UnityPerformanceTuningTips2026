# Pinned by digest (ruby 3.4.13 / Debian 12.15). To update: pull ruby:3.4-bookworm,
# copy its digest here, rebuild, and refresh docker/tool-versions.txt with scripts/check-docker-tools.sh --write.
FROM ruby:3.4.13-bookworm@sha256:9d047dfbfd4d94305e66bcb5317d58af0ac6b8b77f892d497c728e45c0e0afba

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
