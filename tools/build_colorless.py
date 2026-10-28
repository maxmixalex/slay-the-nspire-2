#!/usr/bin/env python3
"""Extend the owner's NSP2/10 pack with the pinned single-player colorless pool.

The 53-card pool excludes only 12 multiplayer-constrained models. All 64 active
potions are represented; generated/event/other-character acquisition stays
separate. Conversion reads local verified data and preserves existing payloads.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
import re
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from build_content import ROOT, atlas, choose_font, image_data, prepared, texture
from build_events import PackData, LIMIT
from build_expansion import rle
from build_tactics import compact_image
from card_banner import material_tint
from pck import Pack
from reference_inventory import verify_release

POWERS = ['retain_hand','plating','buffer','intangible','ritual','regen','clarity',
          'radiance','gigantification','duplication','automation','calamity','mayhem',
          'nostalgia','panache','prep_time','fasten','rolling_boulder','stratagem',
          'entropy','the_gambit','focus']


def card_records() -> list[tuple[str, int, int]]:
    records = []
    for row in (ROOT/'src/colorless_definitions.inc').read_text().splitlines():
        match = re.match(r'\s*\{"([A-Z_]+)",(.*)\},?', row)
        if match:
            fields = match[2].split(',')
            records.append((match[1], int(fields[-2]), int(fields[-1])))
    if len(records) != 46:
        raise ValueError('Expected 44 colorless additions and two generated tokens')
    return records


def potion_keys() -> list[str]:
    keys = re.findall(r'\{"([A-Z_]+)"', (ROOT/'src/potions.cpp').read_text())
    keys += re.findall(r'\{"([A-Z_]+)"', (ROOT/'src/potions_extra_definitions.inc').read_text())
    if len(keys) != 64 or len(set(keys)) != 64:
        raise ValueError('Potion directory must have exactly 64 distinct models')
    return keys


def localized(key: str, text: str, upgraded: bool, keywords: int) -> str:
    if key == 'SOVEREIGN_BLADE':
        # The generated base token has no enchantment-specific targeting/repeat
        # variants. Those enchantment systems are outside this content stage.
        text = '造成{d}点伤害。'
    text = re.sub(r'\{IfUpgraded:show:([^{}]*)\}', lambda m: m[1] if upgraded else '', text)
    text = text.replace('{InCombat:\n（造成{CalculatedDamage:diff()}点伤害）|}',
                        '\x1e\n（造成{d}点伤害）\x1f')
    text = text.replace('{energyPrefix:energyIcons(1)}', '费')
    text = text.replace('{Energy:energyIcons()}', '[gold]{p}[/gold]点能量')
    variables = {'Damage':'d', 'CalculatedDamage':'d', 'Block':'b', 'Cards':'n',
                 'StrengthLoss':'p', 'PlatingPower':'p', 'ExtraBlock':'p', 'Replay':'p',
                 'PanacheDamage':'p', 'PrepTimePower':'p', 'StrengthPower':'p',
                 'DexterityPower':'p', 'ExtraDamage':'p', 'RollingBoulderPower':'p',
                 'BombDamage':'p'}
    for source, target in variables.items():
        text = text.replace('{'+source+':diff()}', '[blue]{'+target+'}[/blue]')
    text = text.replace('{IncrementAmount:diff()}', '[blue]5[/blue]')
    text = text.replace('{Turns:diff()}', '[blue]3[/blue]')
    for bit, name in ((1,'消耗'), (2,'固有'), (4,'保留')):
        if keywords & bit:
            text += '\n[gold]'+name+'。[/gold]'
    if any(token not in ('{d}','{b}','{n}','{p}') for token in re.findall(r'\{[^{}]*\}', text)):
        raise ValueError('Unresolved card localization: '+key+' '+text)
    return text


def build(game: Path, source: Path, output: Path, font_path: Path) -> dict:
    verify_release(game)
    data = source.read_bytes()
    pack = PackData(data)
    if pack.version != 10:
        raise ValueError('Use an NSP2/10 source; do not append to an already-expanded pack')
    old_images, old_clips, old_cards = pack.images[:], pack.clips, pack.cards[:]
    old_glyphs = pack.glyphs.copy()
    pck = Pack(game/'SlayTheSpire2.pck')
    def add_image(image: Image.Image) -> int:
        alpha = image.getchannel('A').getextrema()[0] < 255
        fmt, pixels = compact_image(int(alpha), image_data(image, alpha))
        index = len(pack.images)
        pack.images.append((*image.size, fmt, pixels))
        return index
    def ui(path: str, size: tuple[int,int], fit: str='contain') -> None:
        pack.aux.append(add_image(prepared(texture(pck, path), size, fit)))
    try:
        texts = json.loads(pck.read('localization/zhs/cards.json'))
        records = card_records()
        for key, base, upgraded in records:
            portrait = 'token' if key in ('SOUL','SOVEREIGN_BLADE') else 'colorless'
            image = prepared(texture(pck, f'images/packed/card_portraits/{portrait}/{key.lower()}.png'), (82,60), 'cover')
            start = len(pack.strings)
            pack.cards.append((add_image(image),start,start+1,start+2))
            pack.strings += [texts[key+'.title'],
                             localized(key,texts[key+'.description'],False,base),
                             localized(key,texts[key+'.description'],True,upgraded)]
        for key in potion_keys()[16:]:
            ui('images/potions/large/'+key.lower()+'.png', (26,32))
        for key in POWERS+['demise','shrink','doom','the_bomb']:
            ui('images/powers/'+key+'_power.png', (24,24))
        ui('images/packed/sprite_fonts/star_icon.png', (24,24))
        ui('images/orbs/dark_orb.png', (24,24))
        cache = {}
        for color, kind in (('pink','skill'),('orange','attack')):
            image = material_tint(pck, atlas(pck, '@card/card_frame_'+kind+'_s', cache),
                                  'materials/cards/frames/card_frame_'+color+'_mat.tres')
            pack.aux.append(add_image(prepared(image,(80,112),'stretch')))
        for color in ('necrobinder','regent'):
            pack.aux.append(add_image(prepared(atlas(pck,'@card/energy_'+color,cache),(26,26),'contain')))
    finally:
        pck.close()
    chars = set(''.join(pack.strings)) | {chr(cp) for cp, _ in pack.glyphs}
    for folder in ('src','include'):
        for file in (ROOT/folder).rglob('*'):
            if file.suffix in ('.cpp','.hpp','.inc'):
                chars.update(file.read_text())
    chars = {c for c in chars if ord(c)>=32}
    for px in (8,10,12,16,20):
        font = ImageFont.truetype(str(font_path),px)
        for ch in sorted(chars,key=ord):
            if (ord(ch),px) in pack.glyphs:
                continue
            advance=max(1,math.ceil(font.getlength(ch)));w=max(advance,font.getbbox(ch)[2]);h=px+5
            if w>40 or advance>40:
                raise ValueError('Glyph exceeds runtime dimensions')
            image=Image.new('L',(w,h))
            ImageDraw.Draw(image).text((0,px+1),ch,font=font,fill=255,anchor='ls')
            raw=image.tobytes();encoded=rle(raw)
            pack.glyphs[ord(ch),px]=(w,h,advance,len(encoded),encoded or raw)
    if pack.images[:len(old_images)]!=old_images or pack.clips!=old_clips or pack.cards[:68]!=old_cards:
        raise ValueError('An existing asset or clip changed')
    if any(pack.glyphs[k]!=v for k,v in old_glyphs.items()):
        raise ValueError('Existing glyph payload changed')
    result=pack.encode(version=11)
    check=PackData(result)
    if len(check.cards)!=114 or len(check.aux)!=185 or len(check.strings)!=407:
        raise ValueError('Incomplete output profile')
    output.parent.mkdir(parents=True,exist_ok=True)
    temporary=output.with_name(output.name+'.tmp');temporary.write_bytes(result);temporary.replace(output)
    report={'format':'NSP2/11','reference':'v0.111.0 / 41cef1ea','bytes':len(result),
            'sha256':hashlib.sha256(result).hexdigest(),'source_sha256':hashlib.sha256(data).hexdigest(),
            'card_models':114,'single_player_colorless':53,'generated_tokens':3,'potions':64,
            'relics':30,'characters':2,'events':5,'images':len(pack.images),'glyphs':len(pack.glyphs),
            'strings':407,'ui_entries':185,'preserved_images':len(old_images),'clips':17,
            'excluded':['audio','multiplayer','timeline'],
            'scope':'Content profiles are not per-model gameplay verification or full original-game parity.'}
    output.with_name('content-manifest.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps(report,ensure_ascii=False),flush=True)
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game',type=Path);parser.add_argument('source',type=Path)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--font')
    args=parser.parse_args()
    build(args.game,args.source,args.output,choose_font(args.font))
