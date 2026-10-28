#!/usr/bin/env python3
"""Emit numeric original CardLibrary pool/cost predicates; no private text."""
from __future__ import annotations
import argparse
from pathlib import Path
import re
from locale_cards import model_rows

POOLS=('Ironclad','Silent','Regent','Necrobinder','Defect','Colorless')
RARITIES=('None','Basic','Common','Uncommon','Rare','Ancient','Event','Token','Status','Curse','Quest')

def extract(reference: Path):
    cards=reference/'MegaCrit.Sts2.Core.Models.Cards'
    paths={p.stem.casefold():p for p in cards.glob('*.cs')}
    pools=[]
    for pool in POOLS:
        source=(reference/'MegaCrit.Sts2.Core.Models.CardPools'/f'{pool}CardPool.cs').read_text()
        pools.append({x.casefold() for x in re.findall(r'ModelDb\.Card<([A-Za-z0-9]+)>',source)})
    output=[]
    for row in model_rows():
        key=(row.get('model') or row['key'].replace('_','')).casefold()
        path=paths.get(key)
        if path is None:raise ValueError('Missing model '+row['key'])
        source=path.read_text()
        rarity=re.findall(r':\s*base\([^;{}]*?CardRarity\.([A-Za-z]+)',source)
        if len(rarity)!=1:raise ValueError('Missing constructor rarity '+row['key'])
        kind=RARITIES.index(rarity[0]);mask=sum(1<<i for i,pool in enumerate(pools) if key in pool)
        if kind==5:mask|=64
        if 6<=kind<=10:mask|=128
        if 'ShouldShowInCardLibrary' in source:raise ValueError('Audit custom library visibility '+row['key'])
        # Base cost -1 is the canonical energy-X sentinel in the audited port.
        star_x=bool(re.search(r'HasStarCostX\s*=>\s*true',source))
        output.append((mask,int(star_x)))
    return output

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True)
    p.add_argument('--output',type=Path,default=Path('src/card_library_metadata.inc'));a=p.parse_args()
    values=extract(a.reference)
    lines=['// Numeric CardLibrary predicates, v0.111.0 / 41cef1ea.',
           '// Pool bits follow the original 4x2 sidebar; then HasStarCostX.',
           'static constexpr uint8_t library_card_metadata[CardKinds][2]={']
    for i in range(0,len(values),12):lines.append(' '+','.join('{%d,%d}'%v for v in values[i:i+12])+',')
    lines.append('};');a.output.write_text('\n'.join(lines)+'\n')
    print('Wrote numeric library metadata for',len(values),'models')
if __name__=='__main__':main()
