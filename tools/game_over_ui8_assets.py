"""Pinned score, solo badges and discoveries; only descriptors are public."""
import json
from PIL import Image
from content_tiles import require
from patch_feedback_assets import write_image
from world_scene_assets import load_scene_texture

BASE_AUX, BASE_TEXT = 2013, 8654
BADGES = ('CCCCOMBO','CURSES','DOUBLE_SNECKO','ELITE','FAMISHED','GLUTTON',
          'HIGHLANDER','HONED','BIG_DECK','ILIKESHINY','KACHING','MONEY_MONEY',
          'MYSTERY_MACHINE','PERFECT','RESTFUL','RESTLESS','SPEEDY','TABLET','TINY_DECK')
TIERED = {'ELITE','GLUTTON','BIG_DECK','MONEY_MONEY','PERFECT','SPEEDY','TINY_DECK'}
IMAGE_SPECS = tuple((name,(16,18)) for name in ('score_floor','score_gold','score_elite','score_boss','score_ascension')) + tuple(
    (name,(24,24)) for name in ('badge_bronze','badge_silver','badge_gold')) + tuple(
    ('badge_'+name.lower(),(24,24)) for name in BADGES) + tuple(
    ('discovery_'+name,(24,24)) for name in ('card','relic','potion','monster'))
TEXTS=[]
BADGE_SLOTS=[]
for badge in BADGES:
    start=BASE_TEXT+len(TEXTS)
    tiers=('bronze','silver','gold') if badge in TIERED else ('',)
    for tier in tiers:TEXTS.append((BASE_TEXT+len(TEXTS),'game_over_badge.'+badge+'.'+(tier or 'default')))
    BADGE_SLOTS.append((start,len(tiers)))
PORT_TEXTS={
    'eng':{'VIEW_RUN':'View Run','PARTIAL_HISTORY':'Earlier run data is unavailable.'},
    'zhs':{'VIEW_RUN':'查看本局','PARTIAL_HISTORY':'较早的本局记录不可用。'},
}
PORT_FIRST=BASE_TEXT+len(TEXTS)
TEXTS+=tuple((PORT_FIRST+i,'game_over_ui.'+key) for i,key in enumerate(PORT_TEXTS['eng']))
TEXTS=tuple(TEXTS)
require(BASE_TEXT+len(TEXTS)<=8704,'Game-over text exceeds reserved string directory')

def build_game_over_images(pack,cache,base=BASE_AUX):
    require(base==BASE_AUX,'Game-over source image order changed')
    scene=pack.read('scenes/screens/game_over_screen/score_line.tscn').decode()
    require('custom_minimum_size = Vector2(520, 48)' in scene,'Changed original score-line geometry')
    rows=[]
    for name,size in IMAGE_SPECS:
        source='images/ui/game_over_screen/'+name+'.png'
        original=load_scene_texture(pack,source).convert('RGBA')
        image=original.resize(size,Image.Resampling.LANCZOS)
        key='game-over-'+name.replace('_','-');row=write_image(cache,key,image)
        row.update(key=key,source=source,source_size=list(original.size),auxiliary=base+len(rows));rows.append(row)
    return rows

def build_game_over_text(pack):
    result={}
    for language in ('eng','zhs'):
        table=json.loads(pack.read('localization/'+language+'/badges.json'));localized={}
        for _,qualified in TEXTS:
            if qualified.startswith('game_over_ui.'):
                localized[qualified]=PORT_TEXTS[language][qualified.split('.',1)[1]];continue
            badge,tier=qualified.split('.')[1:]
            prefix='' if tier=='default' else tier
            title=table[badge+'.'+(prefix+'Title' if prefix else 'title')]
            description=table[badge+'.'+(prefix+'Description' if prefix else 'description')]
            # Two original fields share one resource string, retaining both
            # exactly. The first newline separates title from description.
            require('\n' not in title and '\0' not in title+description,'Invalid original badge text')
            localized[qualified]=title+'\n'+description
        result[language]=localized
    return result
