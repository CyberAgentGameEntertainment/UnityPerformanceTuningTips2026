#!/usr/bin/env python3
"""Author profile data and the Re:VIEW blocks that typeset it.

The author of each article is recorded in articles/authors.json instead of
in the manuscript itself.
"""
from __future__ import annotations

import json
import re
from pathlib import Path

PROFILES = Path(__file__).resolve().parents[1] / 'articles' / 'authors.json'

LATEX_ESCAPES = {
    '\\': r'\textbackslash{}',
    '{': r'\{',
    '}': r'\}',
    '$': r'\$',
    '&': r'\&',
    '#': r'\#',
    '%': r'\%',
    '_': r'\_',
    '^': r'\textasciicircum{}',
    '~': r'\textasciitilde{}',
}

HTML_ESCAPES = {'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;'}

# HTML and EPUB have no icon font, so the same three marks travel as inline SVG.
SVG = ('<svg class="author-profile-icon" viewBox="0 0 16 16" width="15" height="15"'
       ' aria-hidden="true" fill="currentColor">{}</svg>')
ICONS = {
    'github': SVG.format(
        '<path d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38'
        ' 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15'
        '-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07'
        '-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21'
        ' 2.2.82a7.4 7.4 0 0 1 2-.27c.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16'
        ' 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0'
        ' 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.01 8.01 0 0 0 16 8c0-4.42-3.58-8-8-8Z"/>'),
    'x': SVG.format(
        '<path d="M9.52 6.78 15.48 0h-1.41L8.89 5.89 4.76 0H0l6.25 8.9L0 16h1.41l5.46-6.22L11.24'
        ' 16H16L9.52 6.78Zm-1.93 2.2-.63-.89L1.92 1.04h2.17l4.06 5.7.63.89 5.28 7.41h-2.17L7.59'
        ' 8.98Z"/>'),
    'web': SVG.format(
        '<path d="M8 0a8 8 0 1 0 0 16A8 8 0 0 0 8 0Zm5.68 4.8h-2.3a12.4 12.4 0 0 0-1.1-2.86'
        ' 6.53 6.53 0 0 1 3.4 2.86ZM8 1.54c.6.87 1.07 1.96 1.37 3.26H6.63C6.93 3.5 7.4 2.41 8'
        ' 1.54ZM1.7 9.6A6.5 6.5 0 0 1 1.5 8c0-.55.07-1.09.2-1.6h2.6a13.6 13.6 0 0 0 0 3.2H1.7Zm.62'
        ' 1.6h2.3c.26 1.04.63 2 1.1 2.86a6.53 6.53 0 0 1-3.4-2.86Zm2.3-6.4h-2.3a6.53 6.53 0 0 1'
        ' 3.4-2.86c-.47.86-.84 1.82-1.1 2.86ZM8 14.46c-.6-.87-1.07-1.96-1.37-3.26h2.74c-.3 1.3-.77'
        ' 2.39-1.37 3.26ZM9.68 9.6H6.32a12.3 12.3 0 0 1 0-3.2h3.36a12.3 12.3 0 0 1 0 3.2Zm.6 4.46c.47'
        '-.86.84-1.82 1.1-2.86h2.3a6.53 6.53 0 0 1-3.4 2.86Zm1.42-4.46a13.6 13.6 0 0 0 0-3.2h2.6c.13'
        '.51.2 1.05.2 1.6 0 .55-.07 1.09-.2 1.6h-2.6Z"/>'),
}


def load(path=PROFILES):
    """Return the article id to profile mapping."""
    data = json.loads(Path(path).read_text(encoding='utf-8'))
    return data.get('articles', {})


def latex_text(value):
    return ''.join(LATEX_ESCAPES.get(character, character) for character in value)


def latex_url(value):
    # hyperref keeps most characters verbatim; only these break its argument.
    return value.replace('\\', r'\\').replace('%', r'\%').replace('#', r'\#')


def html_text(value):
    return ''.join(HTML_ESCAPES.get(character, character) for character in value)


def account(value, host):
    """Return (url, label) for an account given as a handle or as a URL."""
    value = value.strip()
    if re.match(r'^https?://', value):
        url = value.rstrip('/')
        handle = url.rsplit('/', 1)[-1]
    else:
        handle = value.lstrip('@')
        url = f'https://{host}/{handle}'
    return url, handle


def links(profile):
    """Return the icon, URL, and label of every link the author supplied."""
    result = []
    if profile.get('github'):
        url, handle = account(profile['github'], 'github.com')
        result.append(('github', url, handle))
    if profile.get('x'):
        url, handle = account(profile['x'], 'x.com')
        result.append(('x', url, '@' + handle))
    if profile.get('web'):
        url = profile['web'].strip()
        if not re.match(r'^https?://', url):
            url = 'https://' + url
        result.append(('web', url, re.sub(r'^https?://', '', url).rstrip('/')))
    return result


def latex(profile):
    """Return the LaTeX call that typesets one author profile box."""
    body = [r'\reviewauthorname{' + latex_text(profile['name']) + '}']
    if profile.get('romaji'):
        body.append(r'\reviewauthorromaji{' + latex_text(profile['romaji']) + '}')
    if profile.get('affiliation'):
        body.append(r'\reviewauthoraffiliation{' + latex_text(profile['affiliation']) + '}')
    entries = [
        r'\reviewauthorlink{\reviewauthoricon' + kind + '}{' + latex_url(url) + '}{' + latex_text(label) + '}'
        for kind, url, label in links(profile)
    ]
    if entries:
        body.append(r'\reviewauthorlinks{' + r'\reviewauthorsep '.join(entries) + '}')
    return r'\reviewauthorprofile{' + ''.join(body) + '}'


def html(profile):
    """Return the markup that carries the same profile into HTML and EPUB."""
    parts = ['<div class="author-profile">',
             '<p class="author-profile-name">' + html_text(profile['name'])]
    if profile.get('romaji'):
        parts.append('<span class="author-profile-romaji">' + html_text(profile['romaji']) + '</span>')
    parts.append('</p>')
    if profile.get('affiliation'):
        parts.append('<p class="author-profile-affiliation">'
                     + html_text(profile['affiliation']) + '</p>')
    entries = [
        f'<a class="author-profile-link author-profile-{kind}" href="{html_text(url)}">'
        f'{ICONS[kind]}{html_text(label)}</a>'
        for kind, url, label in links(profile)
    ]
    if entries:
        parts.append('<p class="author-profile-links">' + ''.join(entries) + '</p>')
    parts.append('</div>')
    return ''.join(parts)


def review_block(profile):
    """Return the Re:VIEW lines that render the profile in every target."""
    return '\n'.join(('//embed[latex]{', latex(profile), '//}',
                      '//embed[html]{', html(profile), '//}', ''))
