"""Original offline credits, patch notes and feedback art; payloads stay private."""
from pathlib import Path
import json
from PIL import Image
from content_tiles import require
from patch_feedback_assets import write_image
from world_scene_assets import load_scene_texture

BASE_TEXT = 8507
LABEL_KEYS = tuple('settings_ui.'+key for key in (
    'SEND_FEEDBACK','SEND_FEEDBACK_BUTTON_LABEL','CREDITS','CREDITS_BUTTON_LABEL',
    'FEEDBACK_DESCRIPTION_PLACEHOLDER','FEEDBACK_CATEGORY_LABEL','FEEDBACK_EMOJI_LABEL',
    'FEEDBACK_CATEGORY.bug','FEEDBACK_CATEGORY.balance','FEEDBACK_CATEGORY.feedback','FEEDBACK_CATEGORY.translation'))
IMAGE_SPECS = (
    ('patch-notes-icon','images/ui/main_menu/patch_notes_icon.png',(12,12)),
    ('credits-logo','images/ui/sts2_logo_static.png',(124,63)),
    ('credits-fmod','images/ui/credits/fmod_logo.png',(100,27)),
    ('credits-spine','images/ui/credits/spine_logo.png',(95,29)),
    ('credits-godot','images/ui/credits/godot_logo.png',(45,42)),
    ('feedback-happy','images/ui/emote/happy_cultist.png',(12,12)),
    ('feedback-heart','images/ui/emote/heart.png',(12,12)),
    ('feedback-sad','images/ui/emote/slime_sad.png',(12,12)),
    ('feedback-exclaim','images/ui/emote/exclaim.png',(12,12)),
    ('feedback-question','images/ui/emote/question.png',(12,12)),
    ('feedback-sun','images/atlases/compressed.sprites/feedback/the_sun.tres',(49,49)),
    ('feedback-cupcake','images/atlases/compressed.sprites/feedback/cupcake.tres',(59,59)),
    ('feedback-flower','images/atlases/compressed.sprites/feedback/flower.tres',(69,68)),
    ('feedback-flower-anticipation','images/atlases/compressed.sprites/feedback/flower_anticipation.tres',(69,68)),
    ('feedback-flower-happy','images/atlases/compressed.sprites/feedback/flower_happy.tres',(69,68)),
    ('patch-notes-arrow','images/packed/common_ui/settings_tiny_left_arrow.png',(24,24)),
    ('keyboard-keycap','images/ui/keyboard_icon_ninepatch.png',(16,16)),
)


def split_note(text, limit=15000):
    """Preserve every UTF-8 byte; prefer complete paragraphs between chunks."""
    raw=text.encode('utf-8');parts=[]
    while len(raw)>limit:
        stop=raw.rfind(b'\n\n',0,limit+1)
        if stop<limit//2:stop=raw.rfind(b'\n',0,limit+1)
        if stop<limit//2:
            stop=limit
            while raw[stop]&0xc0==0x80:stop-=1
        else:stop+=1
        parts.append(raw[:stop].decode('utf-8'));raw=raw[stop:]
    parts.append(raw.decode('utf-8'))
    require(''.join(parts)==text and all(len(p.encode('utf-8'))<16383 for p in parts),'Patch-note split lost text')
    return parts


# Port-only local export labels are ordinary localized records as well. This
# lets the real font converter discover every visible character (for example
# Chinese 草稿), instead of relying on an incidental older glyph alphabet.
PORT_TEXTS={
 'eng':{'FEEDBACK_TITLE':'Local feedback report','FEEDBACK_EXPORT':'Export',
        'FEEDBACK_EXPORTED':'Report exported locally.',
        'FEEDBACK_FAILED':'Export failed. Your draft is retained.',
        'FEEDBACK_EXPORT_INFO':'Saves a text file on this device.'},
 'zhs':{'FEEDBACK_TITLE':'本地反馈报告','FEEDBACK_EXPORT':'导出',
        'FEEDBACK_EXPORTED':'报告已导出到本地。',
        'FEEDBACK_FAILED':'导出失败，草稿已保留。',
        'FEEDBACK_EXPORT_INFO':'将文本报告保存到此设备。'},
}


NOTE_SPECS=json.loads((Path(__file__).resolve().parents[1]/'config/auxiliary-ui8-0.17.json').read_text())['patch_notes']
TEXTS=[(BASE_TEXT,'auxiliary.CREDITS')]+list(enumerate(LABEL_KEYS,BASE_TEXT+1))
NOTES=[]
for entry in NOTE_SPECS:
    first=BASE_TEXT+len(TEXTS)
    year,month,day=map(int,entry['file'][:-3].split('_'))
    NOTES.append((year*10000+month*100+day,first,entry['chunks']))
    TEXTS.extend((first+i,'patch_notes.'+entry['file'][:-3]+'.'+str(i)) for i in range(entry['chunks']))
PORT_FIRST=BASE_TEXT+len(TEXTS)
TEXTS.extend((PORT_FIRST+i,'auxiliary.'+key) for i,key in enumerate(PORT_TEXTS['eng']))
TEXTS=tuple(TEXTS)
require(BASE_TEXT+len(TEXTS)<=8704,'Auxiliary text exceeds existing reserved directory')


def generated_index():
    return '// Original auxiliary-menu keys; no localized payloads.\n'+''.join(
        ' {"'+key+'",'+str(slot)+'},\n' for slot,key in sorted((row for row in TEXTS if not row[1].startswith('patch_notes.')),key=lambda row:row[1]))


def generated_notes():
    return '// Original note dates and bounded string slices; latest first.\n'+''.join(
        ' {'+str(date)+','+str(slot)+','+str(count)+'},\n' for date,slot,count in NOTES)


def credits_text(table):
    rows=['I0'];used=set()
    def value(key):
        used.add(key);require(key in table,'Missing credit key: '+key);return table[key]
    def lines(kind,key):
        for line in value(key).splitlines():rows.append(kind+line)
    def header(key,major=False):
        kind='H' if major else 'B' if key in ('LOC.header','TRAILER.header') else 't' if key in ('LOC_PTB.header','TRAILER_ANIMATION.header') else 's' if key.startswith('LOC_') or key=='TRAILER_EDITOR.header' else 'h'
        lines(kind,key)
    def centered(key):lines('C',key)
    def roles(key,multi=False):
        for raw in value(key).splitlines():
            parts=[p.strip() for p in raw.split('||')]
            # SplitTwoColumn discards malformed rows in the pinned locale.
            if len(parts)!=2 or not all(parts):continue
            names=[p.strip() for p in parts[0].split(',')] if multi else [parts[0]]
            for i,name in enumerate(names):rows.append('R'+name+'\t'+(parts[1] if i==0 else ''))
            if len(names)>1:rows.append('S8')
    header('MEGA_CRIT.header',True);centered('MEGA_CRIT.names');rows.append('S8');roles('MEGA_CRIT_TEAM.names')
    for key in ('COMPOSER','ADDITIONAL_PROGRAMMING','ADDITIONAL_VFX','MARKETING_SUPPORT','CONSULTANTS'):
        header(key+'.header');centered(key+'.names')
    header('VOICES.header');roles('VOICES.names',True);header('LOC.header')
    for lang in ('PTB','ZHS','ZHT','FRA','DEU','ITA','IND','JPN','KOR','POL','RUS','SPA','ESP','THA','TUR'):
        key='LOC_'+lang;header(key+'.header')
        if key+'.team' in table:centered(key+'.team')
        if lang in ('ZHS','FRA','ITA','RUS','ESP','TUR'):roles(key+'.names')
        else:centered(key+'.names')
    header('TWITCH.header');roles('TWITCH.names');header('MODDING_SUPPORT.header')
    for name in value('MODDING_SUPPORT.names').split('||'):rows.append('M'+name.strip())
    header('PLAYTESTERS.header')
    for name in value('PLAYTESTERS.names').split('||'):rows.append('P'+name.strip())
    header('TRAILER.header');header('TRAILER_ANIMATION.header');centered('TRAILER_ANIMATION.team');roles('TRAILER_ANIMATION.names')
    header('TRAILER_EDITOR.header');roles('TRAILER_EDITOR.names')
    for image,key in enumerate(('FMOD','SPINE','GODOT'),1):
        rows.append('S'+str(20 if key=='GODOT' else 23));rows.append('I'+str(image));centered(key)
    exit_lines=value('EXIT_MESSAGE').splitlines();rows.append('S'+str(max(0,76-len(exit_lines)*11)))
    rows.extend('C'+line for line in exit_lines);rows.extend(('S476','I4'))
    require(used==set(table),'Credits omitted localized keys: '+repr(set(table)-used))
    require(all(len(line[1:].encode('utf-8'))<192 for line in rows if line[:1] in ('M','P')), 'Credits group name exceeds display buffer')
    text='\n'.join(rows)
    require(len(text.encode('utf-8'))<16383 and all(len(line.encode('utf-8'))<1024 for line in rows),'Credits exceed line/string bounds')
    return text


def build_auxiliary_text(pack):
    result={}
    for lang in ('eng','zhs'):
        settings=json.loads(pack.read('localization/'+lang+'/settings_ui.json'))
        texts={'auxiliary.CREDITS':credits_text(json.loads(pack.read('localization/'+lang+'/credits.json')))}
        for key in LABEL_KEYS:texts[key]=settings[key.split('.',1)[1]]
        texts.update(('auxiliary.'+key,value) for key,value in PORT_TEXTS[lang].items())
        for entry in NOTE_SPECS:
            path='localization/'+lang+'/patch_notes/'+entry['file']
            if path not in pack.entries:path='localization/eng/patch_notes/'+entry['file']
            original=pack.read(path).decode('utf-8');parts=split_note(original)
            require(len(parts)<=entry['chunks'],'Pinned patch note gained chunks: '+path)
            for i in range(entry['chunks']):texts['patch_notes.'+entry['file'][:-3]+'.'+str(i)]=parts[i] if i<len(parts) else '\n'
        result[lang]=texts
    return result


def build_auxiliary_images(pack,cache,first):
    rows=[]
    for key,path,size in IMAGE_SPECS:
        image=load_scene_texture(pack,path).convert('RGBA').resize(size,Image.Resampling.LANCZOS)
        row=write_image(cache,key,image);row.update(key=key,source=path,auxiliary=first+len(rows));rows.append(row)
    return rows
