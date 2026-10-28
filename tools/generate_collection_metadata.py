#!/usr/bin/env python3
"""Emit numeric locale order and category membership for original collections.

Private source and localized strings remain inputs only. ICU's platform
collator is checked against the existing .NET-produced 2,232 card ranks before
it is used, so a changed locale installation cannot silently reorder metadata.
"""
from __future__ import annotations
import argparse
import ctypes
import ctypes.util
from functools import cmp_to_key
import json
from pathlib import Path
import re
from locale_cards import model_rows, ROOT
from pck import Pack

ANCIENTS=('Neow','Orobas','Pael','Tezcatara','Nonupeipe','Tanx','Vakuu','Darv')

def collate(names,locale):
    name=ctypes.util.find_library('icui18n')
    if not name:raise ValueError('ICU is required')
    lib=ctypes.CDLL(name);suffix='_'+name.rsplit('.',1)[1]
    op=getattr(lib,'ucol_open'+suffix);op.argtypes=[ctypes.c_char_p,ctypes.POINTER(ctypes.c_int)];op.restype=ctypes.c_void_p
    compare=getattr(lib,'ucol_strcollUTF8'+suffix);compare.argtypes=[ctypes.c_void_p,ctypes.c_char_p,ctypes.c_int,ctypes.c_char_p,ctypes.c_int,ctypes.POINTER(ctypes.c_int)];compare.restype=ctypes.c_int
    close=getattr(lib,'ucol_close'+suffix);close.argtypes=[ctypes.c_void_p]
    status=ctypes.c_int(0);handle=op(locale.encode(),ctypes.byref(status))
    if not handle or status.value>0:raise ValueError('ICU locale open failed')
    encoded=[x.encode() for x in names]
    def cmp(a,b):
        status.value=0;result=compare(handle,encoded[a],len(encoded[a]),encoded[b],len(encoded[b]),ctypes.byref(status))
        if status.value>0:raise ValueError('ICU comparison failed')
        return result
    try:
        order=sorted(range(len(names)),key=cmp_to_key(cmp));ranks=[0]*len(names);rank=0
        for i,index in enumerate(order):
            if i and cmp(order[i-1],index):rank+=1
            ranks[index]=rank
        return ranks
    finally:close(handle)

def generate(reference:Path,pck:Path):
    relics=json.loads((ROOT/'config/world-relics-0.17.json').read_text())['relics']
    relics=sorted(relics,key=lambda r:r['id'])
    potion_text=(ROOT/'src/potions.cpp').read_text().split('};',1)[0]+(ROOT/'src/potions_extra_definitions.inc').read_text()
    potions=re.findall(r'\{"([A-Z_]+)"',potion_text)
    if len(relics)!=297 or len(potions)!=64:raise ValueError('Collection registry changed')
    source_ranks=[int(x) for x in re.findall(r'\d+',(ROOT/'src/card_title_sort_rank.inc').read_text().split('={',1)[1])]
    pack=Pack(pck);ranks=[]
    for lang,locale in [('eng','en-US'),('zhs','zh-CN')]:
        cards=json.loads(pack.read(f'localization/{lang}/cards.json'))
        titles=[cards[r['key']+'.title'] for r in model_rows()]
        verified=collate(titles+[n+'+' for n in titles],locale)
        expected=source_ranks[(0 if lang=='eng' else 1116):(1116 if lang=='eng' else 2232)]
        if verified!=expected:raise ValueError('ICU differs from the pinned .NET card ordering')
        relic_table=json.loads(pack.read(f'localization/{lang}/relics.json'))
        potion_table=json.loads(pack.read(f'localization/{lang}/potions.json'))
        names=[relic_table[r['key']+'.title'] for r in relics]+[potion_table[k+'.title'] for k in potions]
        if any('{' in n for n in names):raise ValueError('Audit dynamic collection title')
        ranks.append(collate(names,locale))
    pack.close()
    groups=[0]*len(relics)
    for i,name in enumerate(ANCIENTS):
        source=(reference/'MegaCrit.Sts2.Core.Models.Events'/f'{name}.cs').read_text()
        members=set(re.findall(r'(?:RelicOption|ModelDb\.Relic)<([A-Za-z0-9]+)>',source))
        for row in relics:
            if row['model'] in members and row['rarity']==7:
                if groups[row['id']]:raise ValueError('Duplicate ancient category '+row['model'])
                groups[row['id']]=i+1
    missing=[r['model'] for r in relics if r['rarity']==7 and not groups[r['id']]]
    if missing:raise ValueError('Unassigned ancient relics '+repr(missing))
    potion_character=[0]*len(potions)
    for character,name in enumerate(('Ironclad','Silent','Regent','Necrobinder','Defect'),1):
        source=(reference/'MegaCrit.Sts2.Core.Timeline.Epochs'/f'{name}4Epoch.cs').read_text()
        members={n.upper() for n in re.findall(r'ModelDb\.Potion<([A-Za-z0-9]+)>',source)}
        for i,key in enumerate(potions):
            if key.replace('_','') in members:potion_character[i]=character
    return ranks,groups,potion_character

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--pck',type=Path,required=True)
    p.add_argument('--output',type=Path,default=Path('src/collection_metadata.inc'));a=p.parse_args();ranks,groups,potion_character=generate(a.reference,a.pck)
    lines=['// Numeric collection metadata. Pinned v0.111.0 / 41cef1ea.',
           '// Locale order was cross-checked against both complete .NET card rank tables.',
           'static constexpr uint16_t collection_title_rank[2][WorldRelicKinds+PotionKinds]={']
    for rank in ranks:
        lines.append(' {')
        for i in range(0,len(rank),24):lines.append('  '+','.join(map(str,rank[i:i+24]))+',')
        lines.append(' },')
    lines.append('};');lines.append('static constexpr uint8_t collection_ancient[WorldRelicKinds]={')
    for i in range(0,len(groups),24):lines.append(' '+','.join(map(str,groups[i:i+24]))+',')
    lines.append('};');lines.append('static constexpr uint8_t collection_potion_character[PotionKinds]={'+','.join(map(str,potion_character))+'};');a.output.write_text('\n'.join(lines)+'\n');print('Generated 297 relics, 64 potions, two locale orders and 8 ancient categories.')
if __name__=='__main__':main()
