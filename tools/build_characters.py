#!/usr/bin/env python3
"""Append the complete pinned Ironclad/Silent card pools to private NSP2/11.

Only original textures and localization from the owner's verified installation
are read. The entire 0.14 image, card, text, auxiliary, clip and glyph prefix is
preserved. No original assets or localization are stored in public source.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
from PIL import Image, ImageDraw, ImageFont
from build_content import ROOT, atlas, choose_font, image_data, prepared, texture
from build_expansion import rle
from build_tactics import compact_image
from card_banner import material_tint
from character_ui_profile import (BASE_CARDS,BASE_STRINGS,BASE_AUXILIARY,
    CARDS,STRINGS,AUXILIARY,CHARACTER_UI)
from content_pack14 import PackData,LIMIT
from pck import Pack
from reference_inventory import verify_release

LIVE_VARIABLES=frozenset(('Damage','Block','CalculatedDamage','CalculatedBlock',
                         'CalculatedHits','CalculatedShivs'))
KEYWORD_KEYS={'Exhaust':'EXHAUST','Innate':'INNATE','Retain':'RETAIN',
              'Ethereal':'ETHEREAL','Sly':'SLY','Unplayable':'UNPLAYABLE',
              'Eternal':'ETERNAL'}

def models(root:Path=ROOT)->list[dict]:
    rows={}
    for character,expected,start,end in [('ironclad',85,114,170),('silent',86,170,227)]:
        path=root/'config'/f'{character}-cards-0.15.json'
        source=json.loads(path.read_text())
        reference=source.get('reference',{})
        tag=source.get('reference_version',reference.get('tag'))
        commit=source.get('reference_commit',reference.get('commit'))
        if tag!='v0.111.0' or commit!='41cef1ea':
            raise ValueError(f'Unpinned character metadata: {path.name}')
        cards=source['cards']
        scope=source.get('scope',{});existing_tokens=set(scope.get('existing_tokens',[]))
        pool=[row for row in cards if row['model'] not in existing_tokens]
        if len(pool)!=expected or source.get('single_player_count',scope.get('single_player_pool'))!=expected:
            raise ValueError(f'Incomplete {character} single-player registry')
        own=[];token_ids={row['id'] for row in source.get('tokens',[])}
        for row in cards+source.get('tokens',[]):
            row=dict(row);row['key']=row['key'].upper();number=row['id']
            if number<BASE_CARDS:continue
            if number in rows:
                if rows[number]['key']!=row['key']:raise ValueError('Conflicting appended model ID')
                continue
            row['character']=character
            if 'portrait' not in row:
                group='token' if number in token_ids else character
                row['portrait']=f"images/packed/card_portraits/{group}/{row['key'].lower()}.png"
            rows[number]=row
            if start<=number<end:own.append(number)
        if sorted(own)!=list(range(start,end)):raise ValueError(f'Append-only {character} ID range drift')
    if sorted(rows)!=list(range(BASE_CARDS,CARDS)):raise ValueError('Missing or duplicate appended card/token ID')
    if rows[227]['key']!='GIANT_ROCK':raise ValueError('Generated Giant Rock ID drift')
    return [rows[number] for number in sorted(rows)]

def _canonical(row:dict,name:str,upgraded:bool)->int:
    variant='upgraded' if upgraded else 'base'
    variables=row['variables']
    if name in variables:value=variables[name][variant]
    elif name in row.get('dynamic_variables',{}):
        if name in ('CalculatedDamage','CalculatedBlock') and 'CalculationBase' in variables:
            value=variables['CalculationBase'][variant]
        elif name=='CalculatedHits':value=variables.get('Repeat',{}).get(variant,0)
        elif name in ('CalculatedHits','CalculatedShivs','CalculatedBlock'):value=0
        else:raise ValueError(f"Missing catalog value: {row['key']}.{name}")
    else:raise ValueError(f"Unknown canonical variable: {row['key']}.{name}")
    if type(value) is not int or not -999999<=value<=999999:
        raise ValueError(f"Unsupported canonical value: {row['key']}.{name}")
    return value

def _branches(text:str)->list[str]:
    depth=0;at=0;parts=[]
    for i,ch in enumerate(text):
        if ch=='{':depth+=1
        elif ch=='}':
            depth-=1
            if depth<0:raise ValueError('Unbalanced conditional expression')
        elif ch=='|' and depth==0:parts.append(text[at:i]);at=i+1
    if depth:raise ValueError('Unbalanced conditional expression')
    parts.append(text[at:])
    if len(parts)>2:raise ValueError('Unsupported conditional alternatives')
    return parts

def localized(text:str,row:dict,upgraded:bool,keyword_titles:dict)->str:
    """Resolve explicit values and bounded nested reference conditionals.

    Live placeholders retain both their variable name and exact catalog value:
    {Damage=9}. The runtime must resolve each named value in a live combat; an
    unsupported hook remains visibly unresolved and fails the UI regressions.
    """
    def render(source:str,depth:int=0,context:str|None=None)->str:
        if depth>8:raise ValueError('Localization expression nesting exceeds bound')
        result=[];at=0
        while at<len(source):
            if source[at]=='}':raise ValueError(f"Unbalanced description: {row['key']}")
            if source[at]!='{':result.append(source[at]);at+=1;continue
            end=at+1;nested=1
            while end<len(source) and nested:
                if source[end]=='{':nested+=1
                elif source[end]=='}':nested-=1
                end+=1
            if nested:raise ValueError(f"Unclosed description: {row['key']}")
            expression=source[at+1:end-1]
            if expression.startswith('IfUpgraded:show:'):
                choices=_branches(expression[len('IfUpgraded:show:'):])
                selected=choices[0] if upgraded else choices[1] if len(choices)>1 else ''
                result.append(render(selected,depth+1,context))
            elif expression.startswith('InCombat:'):
                choices=_branches(expression[len('InCombat:'):])
                if len(choices)>1 and choices[1]:raise ValueError('Unsupported nonempty noncombat branch')
                result.append('\x1e'+render(choices[0],depth+1,context)+'\x1f')
            elif expression=='energyPrefix:energyIcons(1)':result.append(' 点能量')
            else:
                conditional=re.fullmatch(r'([A-Za-z][A-Za-z0-9]*):(plural|choose\(1\)):(.*)',expression,re.S)
                if conditional:
                    name,style,alternatives=conditional.groups();choices=_branches(alternatives)
                    if len(choices)!=2:raise ValueError(f'Expected two count alternatives: {expression}')
                    if name in LIVE_VARIABLES or name in row.get('dynamic_variables',{}):
                        raise ValueError(f'Unsupported live count conditional: {expression}')
                    value=_canonical(row,name,upgraded)
                    # The pinned pool uses singular/plural and choose(1) with
                    # a single fallback. Empty-selector formats inherit this
                    # exact variable, including through nested conditionals.
                    result.append(render(choices[0 if value==1 else 1],depth+1,name))
                    at=end;continue
                match=re.fullmatch(r'([A-Za-z][A-Za-z0-9]*)?:(diff\(\)|inverseDiff\(\)|energyIcons\(\))',expression)
                if not match:raise ValueError(f"Unsupported expression: {row['key']}: {expression}")
                name,style=match.groups();name=name or context
                if not name:raise ValueError(f'Value format without a selected variable: {expression}')
                # inverseDiff changes comparison highlighting in the original
                # formatter; it never negates the displayed numeric value.
                value=_canonical(row,name,upgraded)
                dynamic=name in LIVE_VARIABLES
                if name in row.get('dynamic_variables',{}) and not dynamic:
                    raise ValueError(f"Unsupported live variable: {row['key']}.{name}")
                result.append('{'+name+'='+str(value)+'}' if dynamic else str(value))
                if style=='energyIcons()':result.append(' 点能量')
            at=end
        return ''.join(result)
    output=render(text.replace('\\n','\n'))
    for name in row['keywords']['upgraded' if upgraded else 'base']:
        if name not in KEYWORD_KEYS:raise ValueError(f"Unsupported card keyword: {row['key']}.{name}")
        key=KEYWORD_KEYS[name]+'.title'
        if key not in keyword_titles:raise ValueError(f'Missing reference keyword title: {key}')
        output+='\n[gold]'+keyword_titles[key]+'。[/gold]'
    tokens=re.findall(r'\{([A-Za-z][A-Za-z0-9]*)=-?[0-9]+\}',output)
    if any(t not in LIVE_VARIABLES for t in tokens) or output.count('{')!=len(tokens) or output.count('}')!=len(tokens):
        raise ValueError(f"Unresolved description: {row['key']}")
    return output

def retained_prefix(source:PackData)->tuple:
    if source.version not in (11,12):raise ValueError('Character conversion requires NSP2/11 or NSP2/12')
    count=len(source.images) if source.version==11 else min(r[0] for r in source.cards[BASE_CARDS:])
    if count<1 or any(r[0]>=count for r in source.cards[:BASE_CARDS]) or any(i>=count for i in source.aux[:BASE_AUXILIARY]):
        raise ValueError('Character input does not preserve the complete earlier image prefix')
    return (source.images[:count],source.cards[:BASE_CARDS],source.strings[:BASE_STRINGS],
            source.aux[:BASE_AUXILIARY],source.clips,source.glyphs.copy())

def assert_retained(pack:PackData,prefix:tuple)->None:
    images,cards,strings,aux,clips,glyphs=prefix
    if (pack.images[:len(images)]!=images or pack.cards[:BASE_CARDS]!=cards or
        pack.strings[:BASE_STRINGS]!=strings or pack.aux[:BASE_AUXILIARY]!=aux or pack.clips!=clips):
        raise ValueError('Historical image/card/text/auxiliary/animation data changed')
    if any(pack.glyphs.get(key)!=value for key,value in glyphs.items()):
        raise ValueError('Historical glyph data changed')

def build(game:Path,source:Path,output:Path,font_path:Path)->dict:
    verify_release(game)
    rows=models()
    pack=PackData(source.read_bytes());prefix=retained_prefix(pack)
    pack.images,pack.cards,pack.strings,pack.aux=([*prefix[0]],[*prefix[1]],[*prefix[2]],[*prefix[3]])
    paths=[];fallbacks=[];cache={}
    def add_image(image):
        alpha=image.getchannel('A').getextrema()[0]<255
        fmt,raw=compact_image(int(alpha),image_data(image,alpha))
        index=len(pack.images);pack.images.append((*image.size,fmt,raw));return index
    pck=Pack(game/'SlayTheSpire2.pck')
    try:
        card_text=json.loads(pck.read('localization/zhs/cards.json'))
        keywords=json.loads(pck.read('localization/zhs/card_keywords.json'))
        for row in rows:
            name=row['portrait']
            if name+'.import' not in pck.entries:
                # The pinned Sidestep model has only the original beta portrait.
                if row['key']!='SIDESTEP':raise ValueError(f'Missing original portrait: {name}')
                fallback='images/packed/card_portraits/silent/beta/sidestep.png'
                if fallback+'.import' not in pck.entries:raise ValueError('Missing original Sidestep beta portrait')
                fallbacks.append({'card':row['key'],'requested':name,'used':fallback,'reason':'No final portrait in the pinned PCK'})
                name=fallback
            image=prepared(texture(pck,name),(82,60),'cover');paths.append(name)
            key=row['key'];first=len(pack.strings)
            pack.strings.extend([card_text[key+'.title'],localized(card_text[key+'.description'],row,False,keywords),
                                localized(card_text[key+'.description'],row,True,keywords)])
            if row['id']!=len(pack.cards):raise ValueError('Character card directory order drift')
            pack.cards.append((add_image(image),first,first+1,first+2))
        header=(ROOT/'include/ui_assets.hpp').read_text()
        if not re.search(r'\bU_Count\s*=\s*'+str(AUXILIARY)+r'\b',header):raise ValueError('Character auxiliary count drift')
        for i,(key,name,size,fit) in enumerate(CHARACTER_UI,BASE_AUXILIARY):
            if not re.search(r'\bU_'+key+r'\s*=\s*'+str(i)+r'\b',header):raise ValueError(f'Character UI index drift: {key}')
            if name=='@curse_frame':
                material='materials/cards/frames/card_frame_curse_mat.tres'
                image=material_tint(pck,atlas(pck,'@card/card_frame_skill_s',cache),material)
                paths.extend(['@card/card_frame_skill_s',material])
            else:image=texture(pck,name);paths.append(name)
            pack.aux.append(add_image(prepared(image,size,fit)))
    finally:pck.close()
    alphabet=set(''.join(pack.strings))
    for folder in ('src','include'):
        for path in (ROOT/folder).rglob('*'):
            if path.suffix in ('.cpp','.hpp','.inc'):alphabet.update(path.read_text())
    for px in (8,10,12,16,20):
        font=ImageFont.truetype(str(font_path),px)
        for ch in sorted(alphabet,key=ord):
            if ord(ch)<32 or (ord(ch),px) in pack.glyphs:continue
            advance=max(1,math.ceil(font.getlength(ch)));w=max(advance,font.getbbox(ch)[2]);h=px+5
            if w>40 or advance>40:raise ValueError('Glyph exceeds runtime bounds')
            bitmap=Image.new('L',(w,h));ImageDraw.Draw(bitmap).text((0,px+1),ch,font=font,fill=255,anchor='ls')
            raw=bitmap.tobytes();runs=rle(raw)
            pack.glyphs[ord(ch),px]=(w,h,advance,len(runs),runs or raw)
    if (len(pack.cards),len(pack.strings),len(pack.aux))!=(CARDS,STRINGS,AUXILIARY):
        raise ValueError('Incomplete character output profile')
    assert_retained(pack,prefix)
    data=pack.encode(12);decoded=PackData(data);assert_retained(decoded,prefix)
    if decoded.encode(12)!=data:raise ValueError('Character pack encoding is not canonical')
    output.parent.mkdir(parents=True,exist_ok=True)
    temp=output.with_name(output.name+'.tmp');temp.write_bytes(data);temp.replace(output)
    manifest={'format':'NSP2/12','reference':'v0.111.0 / 41cef1ea','bytes':len(data),
        'sha256':hashlib.sha256(data).hexdigest(),'cards':CARDS,'ironclad_single_player':85,
        'silent_single_player':86,'single_player_colorless':53,'generated_tokens':4,
        'potions':64,'relics':30,'strings':len(pack.strings),'auxiliary_images':len(pack.aux),
        'images':len(pack.images),'glyphs':len(pack.glyphs),'clips':17,
        'retained_image_payloads':len(prefix[0]),'retained_card_records':BASE_CARDS,
        'retained_strings':BASE_STRINGS,'retained_auxiliary_records':BASE_AUXILIARY,
        'retained_glyph_payloads':len(prefix[5]),'remaining_resident_bytes':LIMIT-len(data),
        'new_card_keys':[row['key'] for row in rows],'source_paths':paths,'portrait_fallbacks':fallbacks,
        'frame_adaptations':{'status':'existing original colorless skill frame',
                             'curse':'original skill frame with original curse material'},
        'excluded':['audio','multiplayer','timeline'],
        'limitations':'Content counts identify models and art; effect and UI tests establish executable behavior separately.'}
    output.with_name('content-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps({k:v for k,v in manifest.items() if k not in ('source_paths','new_card_keys')},ensure_ascii=False),flush=True)
    return manifest

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game',type=Path);parser.add_argument('source',type=Path)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--font')
    args=parser.parse_args();build(args.game,args.source,args.output,choose_font(args.font))
