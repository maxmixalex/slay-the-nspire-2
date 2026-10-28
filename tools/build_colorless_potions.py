#!/usr/bin/env python3
"""Append the fixed single-player colorless registry, potions and status art.

Read only the owner's verified local v0.111.0 / 41cef1ea installation. Preserve
all earlier image/animation/glyph payloads. Never publish the generated pack.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
from PIL import Image, ImageDraw, ImageFont
from build_content import ROOT, choose_font, texture, prepared, image_data, atlas
from build_expansion import rle
from build_tactics import compact_image
from card_banner import material_tint
from content_pack14 import PackData, LIMIT
from pck import Pack
from reference_inventory import verify_release

POWER_ART=['retain_hand','plating','buffer','intangible','ritual','regen','clarity','radiance',
 'gigantification','duplication','automation','calamity','mayhem','nostalgia','panache',
 'prep_time','fasten','rolling_boulder','stratagem','entropy','the_gambit','focus']

def models():
    source=(ROOT/'src/colorless_definitions.inc').read_text()
    rows=[]
    for line in source.splitlines():
        if not re.match(r'\s*\{"',line):continue
        fields=line.strip().strip(',').strip('{}').split(',')
        rows.append((fields[0].strip('"'),int(fields[-2]),int(fields[-1])))
    if len(rows)!=46 or rows[-2][0]!='SOUL' or rows[-1][0]!='SOVEREIGN_BLADE':
        raise ValueError('Append-only card registry drift')
    potions=re.findall(r'\{"([A-Z_]+)"',(ROOT/'src/potions_extra_definitions.inc').read_text())
    if len(potions)!=48:raise ValueError('Potion registry drift')
    return rows,potions

def localized(text:str,key:str,upgraded:bool,keywords:int)->str:
    # Only the supported standalone Blade is synthesized from this expression;
    # enchantments that alter target/repeat/block remain outside this card model.
    if key=='SOVEREIGN_BLADE':text='造成{Damage:diff()}点伤害。'
    text=text.replace('{InCombat:\n（造成{CalculatedDamage:diff()}点伤害）|}', '\x1e\n（造成{d}点伤害）\x1f')
    text=re.sub(r'\{IfUpgraded:show:([^{}]*)\}',lambda m:m[1] if upgraded else '',text)
    text=text.replace('{energyPrefix:energyIcons(1)}',' 点能量')
    text=text.replace('{Energy:energyIcons()}','{p} 点能量')
    mapping={'Damage':'d','CalculatedDamage':'d','Block':'b','Cards':'n',
      'StrengthLoss':'p','PlatingPower':'p','ExtraBlock':'p','Replay':'p','PanacheDamage':'p',
      'PrepTimePower':'p','StrengthPower':'p','DexterityPower':'p','ExtraDamage':'p',
      'RollingBoulderPower':'p','BombDamage':'p'}
    for name,token in mapping.items():text=text.replace('{'+name+':diff()}','{'+token+'}')
    text=text.replace('{Turns:diff()}','3').replace('{IncrementAmount:diff()}','5')
    for bit,label in [(1,'消耗'),(2,'固有'),(4,'保留')]:
        if keywords&bit:text+='\n[gold]'+label+'。[/gold]'
    unresolved=set(re.findall(r'\{[^{}]*\}',text))-{'{d}','{b}','{n}','{p}'}
    if unresolved or text.count('{')!=len(re.findall(r'\{[dbnp]\}',text)):
        raise ValueError(f'Unresolved expression for {key}: {text}')
    return text

def build(game:Path,source:Path,output:Path,font_path:Path)->dict:
    verify_release(game)
    old=PackData(source.read_bytes())
    if old.version not in (10,11):raise ValueError('Colorless conversion requires NSP2/10 or NSP2/11; character packs must use build_characters.py')
    # Repeated conversion uses the exact old prefix, not a growing duplicate tail.
    rows,potions=models();pack=PackData(source.read_bytes())
    retained_images=old.images[:];retained_glyphs=old.glyphs.copy();retained_clips=old.clips
    old_image_count=len(old.images) if old.version==10 else min(r[0] for r in old.cards[68:])
    pack.images=pack.images[:old_image_count];pack.cards=pack.cards[:68];pack.strings=pack.strings[:269];pack.aux=pack.aux[:105]
    pck=Pack(game/'SlayTheSpire2.pck');paths=[];cache={}
    try:
        card_text=json.loads(pck.read('localization/zhs/cards.json'))
        def add_image(image):
            alpha=image.getchannel('A').getextrema()[0]<255
            fmt,raw=compact_image(int(alpha),image_data(image,alpha))
            index=len(pack.images);pack.images.append((*image.size,fmt,raw));return index
        for key,base_kw,up_kw in rows:
            name='images/packed/card_portraits/'+('token/' if key in ('SOUL','SOVEREIGN_BLADE') else 'colorless/')+key.lower()+'.png'
            image=prepared(texture(pck,name),(82,60),'cover');paths.append(name)
            strings=[card_text[key+'.title'],localized(card_text[key+'.description'],key,False,base_kw),localized(card_text[key+'.description'],key,True,up_kw)]
            start=len(pack.strings);pack.strings.extend(strings);pack.cards.append((add_image(image),start,start+1,start+2))
        for key in potions:
            name='images/potions/large/'+key.lower()+'.png';paths.append(name)
            pack.aux.append(add_image(prepared(texture(pck,name),(25,30),'contain')))
        for key in POWER_ART+['demise','shrink','doom','the_bomb']:
            name='images/powers/'+key+'_power.png';paths.append(name)
            pack.aux.append(add_image(prepared(texture(pck,name),(18,18),'contain')))
        for name in ['images/ui/combat/energy_star.png','images/orbs/dark_orb.png']:
            paths.append(name);pack.aux.append(add_image(prepared(texture(pck,name),(18,18),'contain')))
        for color,kind in [('pink','skill'),('orange','attack')]:
            name=f'materials/cards/frames/card_frame_{color}_mat.tres';paths.append(name)
            image=material_tint(pck,atlas(pck,f'@card/card_frame_{kind}_s',cache),name)
            pack.aux.append(add_image(prepared(image,(80,112),'stretch')))
        for name in ['@card/energy_necrobinder','@card/energy_regent']:
            paths.append(name);pack.aux.append(add_image(prepared(atlas(pck,name,cache),(26,26),'contain')))
    finally:pck.close()
    alphabet=set(''.join(pack.strings))
    for folder in ('src','include'):
        for p in (ROOT/folder).rglob('*'):
            if p.suffix in ('.cpp','.hpp','.inc'):alphabet.update(p.read_text())
    for px in (8,10,12,16,20):
        font=ImageFont.truetype(str(font_path),px)
        for ch in sorted(alphabet,key=ord):
            if ord(ch)<32 or (ord(ch),px) in pack.glyphs:continue
            advance=max(1,math.ceil(font.getlength(ch)));w=max(advance,font.getbbox(ch)[2]);h=px+5
            if w>40 or advance>40:raise ValueError('Glyph exceeds runtime bounds')
            image=Image.new('L',(w,h));ImageDraw.Draw(image).text((0,px+1),ch,font=font,fill=255,anchor='ls')
            raw=image.tobytes();runs=rle(raw);pack.glyphs[ord(ch),px]=(w,h,advance,len(runs),runs or raw)
    if pack.images[:old_image_count]!=retained_images[:old_image_count] or pack.clips!=retained_clips:
        raise ValueError('Historical image or animation payload changed')
    if any(pack.glyphs[k]!=v for k,v in retained_glyphs.items()):raise ValueError('Retained glyph changed')
    data=pack.encode();PackData(data)
    output.parent.mkdir(parents=True,exist_ok=True)
    temp=output.with_name(output.name+'.tmp');temp.write_bytes(data);temp.replace(output)
    manifest={'format':'NSP2/11','reference':'v0.111.0 / 41cef1ea','bytes':len(data),
      'sha256':hashlib.sha256(data).hexdigest(),'cards':114,'single_player_colorless':53,'potions':64,'relics':30,
      'strings':len(pack.strings),'auxiliary_images':len(pack.aux),'images':len(pack.images),'glyphs':len(pack.glyphs),'clips':17,
      'retained_image_payloads':old_image_count,'remaining_resident_bytes':LIMIT-len(data),
      'new_card_keys':[r[0] for r in rows],'new_potion_keys':potions,'source_paths':paths,
      'excluded':['audio','multiplayer','timeline'],
      'limitations':'Other-character potions are available in practice; this does not add full characters or original generation pools.'}
    output.with_name('content-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps({k:v for k,v in manifest.items() if k not in ('source_paths','new_card_keys','new_potion_keys')},ensure_ascii=False),flush=True)
    return manifest

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('game',type=Path);ap.add_argument('source',type=Path);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--font')
    args=ap.parse_args();build(args.game,args.source,args.output,choose_font(args.font))
