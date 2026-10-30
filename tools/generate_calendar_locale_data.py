#!/usr/bin/env python3
"""Generate bounded calendar culture facts from calendar_reference_oracle.

Input is its cultures.json metadata, not an assembly or localization payload.
The Unicode-derived output is distributed with third_party/unicode/LICENSE.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import unicodedata
import regex

CODES = ('eng', 'zhs', 'zht', 'deu', 'esp', 'fra', 'ind', 'ita',
         'jpn', 'kor', 'pol', 'ptb', 'rus', 'spa', 'tha', 'tur')


def generate(cultures):
    if tuple(row['code'] for row in cultures) != CODES:
        raise ValueError('Expected the exact original sixteen-language order')
    sets, rows = {}, []
    def literal(text):
        return json.dumps(text, ensure_ascii=False).replace('\\u0000', '\\0')
    def months(values):
        if len(values) != 13 or values[12] or not all(values[:12]):
            raise ValueError('Expected twelve nonempty month names')
        values = tuple(values[:12])
        return sets.setdefault(values, 'Months'+str(len(sets)))
    for row in cultures:
        if row['calendar'] not in ('GregorianCalendar', 'ThaiBuddhistCalendar'):
            raise ValueError('An unsupported calendar requires an implementation')
        offset = 543 if row['calendar'] == 'ThaiBuddhistCalendar' else 0
        if row['year'] != 2026+offset or row['time_separator'] not in (':', '.'):
            raise ValueError('Unexpected culture year or time separator')
        normal = months(row['months'])
        genitive = 'nullptr' if row['months'] == row['genitive'] else months(row['genitive'])
        fields = [normal, genitive, literal(row['am']), literal(row['pm']),
                  literal(row['date_separator']), str(offset), repr(row['time_separator'])]
        rows.append('    {'+', '.join(fields)+'}, // '+row['code']+' / '+row['culture'])
    text = ('// Generated culture facts for the v0.111.0 language map.\n'
            '// .NET 9.0.22 / ICU 74.2; see tools/calendar_reference_oracle and\n'
            '// docs/calendar-locales-ui18-0.17.md. Unicode License V3: third_party/unicode/LICENSE.\n'
            '// Month strings are NUL-separated to avoid 12 pointers per language.\n')
    for values, name in sets.items():
        text += 'static constexpr char '+name+'[]=\n    '+'\n    '.join(literal(value+'\0') for value in values)+';\n'
    text += 'static constexpr Locale Locales[]={\n'+'\n'.join(rows)+'\n};\n'
    clusters = {}
    for word in cultures[14]['months'][:12]:
        for cluster in regex.findall(r'\X', word):
            if len(cluster) > 1 and any(unicodedata.category(c).startswith('M') or c == '\u0e33' for c in cluster):
                clusters.setdefault(cluster, 0x10f000+len(clusters))
    if len(clusters) != 11 or any(len(word.encode()) <= 4 for word in clusters):
        raise ValueError('Unexpected Thai calendar graphemes')
    text += ('\n// Fixed display-only Thai month graphemes. Private original font rasters\n'
             '// are supplied by calendar_fonts_ui18.py; gameplay/text values stay Unicode.\n'
             'struct Cluster {const char* text;uint32_t codepoint;};\n'
             'static constexpr Cluster ThaiMonthClusters[]={\n')
    text += '\n'.join('    {'+literal(word)+', '+hex(cp)+'},' for word, cp in clusters.items())+'\n};\n'
    return text


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.write_text(generate(json.loads(args.input.read_text())))
